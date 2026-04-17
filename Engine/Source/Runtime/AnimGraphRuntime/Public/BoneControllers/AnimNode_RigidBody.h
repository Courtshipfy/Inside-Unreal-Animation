// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Physics/ImmediatePhysics/ImmediatePhysicsDeclares.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsProxy/PerSolverFieldSystem.h"
#include "Tasks/Task.h"
#include "AnimNode_RigidBody.generated.h"

struct FBodyInstance;
struct FConstraintInstance;
class FEvent;

extern ANIMGRAPHRUNTIME_API bool bEnableRigidBodyNode;
extern ANIMGRAPHRUNTIME_API FAutoConsoleVariableRef CVarEnableRigidBodyNode;
extern ANIMGRAPHRUNTIME_API TAutoConsoleVariable<int32> CVarEnableRigidBodyNodeSimulation;
extern ANIMGRAPHRUNTIME_API TAutoConsoleVariable<int32> CVarRigidBodyLODThreshold;

/** Determines in what space the simulation should run */
/** 确定模拟应该在什么空间中运行 */
UENUM()
enum class ESimulationSpace : uint8
{
	/** Simulate in component space. Moving the entire skeletal mesh will have no affect on velocities */
	/** 在组件空间中进行模拟。移动整个骨架网格物体不会影响速度 */
	ComponentSpace,
	/** Simulate in world space. Moving the skeletal mesh will generate velocity changes */
	/** 在世界空间中进行模拟。移动骨架网格物体会产生速度变化 */
	WorldSpace,
	/** Simulate in another bone space. Moving the entire skeletal mesh and individually modifying the base bone will have no affect on velocities */
	/** 在另一个骨骼空间中进行模拟。移动整个骨架网格物体并单独修改基骨不会对速度产生影响 */
	BaseBoneSpace,
};

/** Determines behaviour regarding deferral of simulation tasks. */
/** 确定有关延迟模拟任务的行为。 */
UENUM()
enum class ESimulationTiming : uint8
{
	/** Use the default project setting as defined by p.RigidBodyNode.DeferredSimulationDefault. */
	/** 使用 p.RigidBodyNode.DeferredSimulationDefault 定义的默认项目设置。 */
	Default,
	/** Always run the simulation to completion during animation evaluation. */
	/** 在动画评估期间始终运行模拟直至完成。 */
	Synchronous,
	/** Always run the simulation in the background and retrieve the result on the next animation evaluation. */
	/** 始终在后台运行模拟并在下一次动画评估时检索结果。 */
	Deferred
};

/**
 * Settings for the system which passes motion of the simulation's space into the simulation. This allows the simulation to pass a 
 * fraction of the world space motion onto the bodies which allows Bone-Space and Component-Space simulations to react to world-space 
 * movement in a controllable way.
 */
template <> struct TIsPODType<FSimSpaceSettings> { enum { Value = true }; };

USTRUCT(BlueprintType)
struct FSimSpaceSettings
{
	GENERATED_USTRUCT_BODY()

	ANIMGRAPHRUNTIME_API FSimSpaceSettings();

	// Disable deprecation errors by providing defaults wrapped with pragma disable
	// 通过提供用 pragma disable 包装的默认值来禁用弃用错误
	PRAGMA_DISABLE_DEPRECATION_WARNINGS
	~FSimSpaceSettings() = default;
	FSimSpaceSettings(FSimSpaceSettings const&) = default;
	FSimSpaceSettings& operator=(const FSimSpaceSettings &) = default;
	PRAGMA_ENABLE_DEPRECATION_WARNINGS

	// Global multipler on the effects of simulation space movement. Must be in range [0, 1]. If WorldAlpha = 0.0, the system is disabled and the simulation will
	// 模拟空间运动影响的全局倍数。必须在 [0, 1] 范围内。如果 WorldAlpha = 0.0，系统被禁用，模拟将
	// be fully local (i.e., world-space actor movement and rotation does not affect the simulation). When WorldAlpha = 1.0 the simulation effectively acts as a 
	// 完全本地化（即世界空间演员的移动和旋转不会影响模拟）。当 WorldAlpha = 1.0 时，模拟有效地充当
	// world-space sim, but with the ability to apply limits using the other parameters.
	// 世界空间模拟，但能够使用其他参数应用限制。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float WorldAlpha;

#if WITH_EDITORONLY_DATA
	UE_DEPRECATED(5.1, "This property has been deprecated. Please, use WorldAlpha.")
	float MasterAlpha = 0.f;
#endif // WITH_EDITORONLY_DATA

	// Multiplier on the Z-component of velocity and acceleration that is passed to the simulation. Usually from 0.0 to 1.0 to 
	// 传递到模拟的速度和加速度 Z 分量的乘数。通常从 0.0 到 1.0 到
	// reduce the effects of jumping and crouching on the simulation, but it can be higher than 1.0 if you need to exaggerate this motion for some reason.
	// 减少跳跃和蹲伏对模拟的影响，但如果您出于某种原因需要夸大此运动，则该值可以高于 1.0。
	// Should probably have been called "WorldAlphaScaleZ".
	// 也许应该被称为“WorldAlphaScaleZ”。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float VelocityScaleZ;

	// A muliplier to control how much of the simulation space movement is used to calculate the drag forces from Linear/Angular Damping in the Physics Asset.
	// 用于控制模拟空间运动的乘数用于计算物理资源中线性/角度阻尼的阻力。
	// When DampingAlpha=1.0, Damping drag forces are equivalent to a world-space simulation. This is similar to air resistance.
	// 当 DampingAlpha=1.0 时，阻尼阻力相当于世界空间模拟。这与空气阻力类似。
	// When DampingAlpha=0.0, Damping drag forces depend only on local-space body velocity and not on the simulation space velocity.
	// 当 DampingAlpha=0.0 时，阻尼阻力仅取决于局部空间体速度，而不取决于模拟空间速度。
	// It can be useful to set this to zero so that the Linear/Angular Damping settings on the BodyInstances do not contribute to air resistance. 
	// 将其设置为零会很有用，这样 BodyInstance 上的线性/角度阻尼设置不会增加空气阻力。
	// Air resistance can be re-added in a controlled way using the ExternalLinearDrag setting below.
	// 可以使用下面的ExternalLinearDrag 设置以受控方式重新添加空气阻力。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float DampingAlpha;

	// A clamp on the effective world-space velocity that is passed to the simulation. Units are cm/s. The default value effectively means "unlimited". It is not usually required to
	// 对传递到模拟的有效世界空间速度的限制。单位为厘米/秒。默认值实际上意味着“无限制”。通常不需要
	// change this but you would reduce this to limit the effects of drag on the bodies in the simulation (if you have bodies that have LinearDrag set to non-zero in the physics asset). 
	// 更改此设置，但您可以减少此设置以限制模拟中拖动对主体的影响（如果您的主体在物理资源中将 LinearDrag 设置为非零）。
	// Expected values in this case would be somewhat less than the usual velocities of your object which is commonly a few hundred for a character.
	// 在这种情况下，预期值会比对象的通常速度稍低，通常一个角色的速度为几百。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxLinearVelocity;

	// A clamp on the effective world-space angular velocity that is passed to the simulation. Units are radian/s, so a value of about 6.0 is one rotation per second.
	// 对传递到模拟的有效世界空间角速度的限制。单位为弧度/秒，因此大约 6.0 的值是每秒旋转一圈。
	// The default value effectively means "unlimited". You would reduce this (and MaxAngularAcceleration) to limit how much bodies "fly out" when the actor spins on the spot. 
	// 默认值实际上意味着“无限制”。您可以减少此值（以及 MaxAngularAcceleration），以限制演员在原地旋转时“飞出”的物体数量。
	// This is especially useful if you have characters than can rotate very quickly and you would probably want values around or less than 10 in this case.
	// 如果您的角色旋转速度非常快，并且在这种情况下您可能需要 10 左右或小于 10 的值，那么这尤其有用。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxAngularVelocity;
	
	// A clamp on the effective world-space acceleration that is passed to the simulation. Units are cm/s/s. The default value effectively means "unlimited". 
	// 对传递到模拟的有效世界空间加速度的限制。单位为厘米/秒/秒。默认值实际上意味着“无限制”。
	// This property is used to stop the bodies of the simulation flying out when suddenly changing linear speed. It is useful when you have characters than can 
	// 该属性用于在突然改变线速度时阻止模拟物体飞出。当你的角色数量多于可容纳的数量时，它很有用
	// changes from stationary to running very quickly such as in an FPS. A common value for a character might be in the few hundreds.
	// 从静止状态快速转变为运行状态，例如在 FPS 中。一个字符的常见值可能是几百个。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxLinearAcceleration;
	
	// A clamp on the effective world-space angular accleration that is passed to the simulation. Units are radian/s/s. The default value effectively means "unlimited". 
	// 对传递到模拟的有效世界空间角加速度的限制。单位为弧度/秒/秒。默认值实际上意味着“无限制”。
	// This has a similar effect to MaxAngularVelocity, except that it is related to the flying out of bodies when the rotation speed suddenly changes. Typical limist for
	// 这与MaxAngularVelocity有类似的效果，只不过它与旋转速度突然变化时物体飞出有关。典型的限制主义者
	// a character might be around 100.
	// 一个字符可能有100个左右。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxAngularAcceleration;

#if WITH_EDITORONLY_DATA
	UPROPERTY(meta = (DeprecatedProperty, DeprecationMessage = "ExternalLinearDrag is deprecated. Please use ExternalLinearDragV instead."))
	float ExternalLinearDrag_DEPRECATED;
#endif

	// Additional linear drag applied to each body based on total body velocity. This is in addition to per-body linear damping in the physics asset (but see DampingAlpha to control that).
	// 根据总体速度向每个物体施加额外的线性阻力。这是物理资源中每个物体线性阻尼的补充（但请参阅 DampingAlpha 来控制它）。
	// (NOTE: The "V" suffix is to differentiate from the deprecated float property of the same name. It means "Vector" and not "Velocity").
	// （注意：“V”后缀是为了区别已弃用的同名浮点属性。它的意思是“矢量”而不是“速度”）。
	//
	// NOTE: ExternalLinearDragV is in simulation space, so if the RB AnimNode is set to Bone Space the ExternalLinearDragV.Z will be the drag in the
	// 注意：ExternalLinearDragV 在模拟空间中，因此如果 RB AnimNode 设置为 Bone Space，ExternalLinearDragV.Z 将是
	// Up direction of the selected bone.
	// 所选骨骼的向上方向。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector ExternalLinearDragV;

	// Additional velocity that is added to the component velocity so the simulation acts as if the actor is moving at speed, even when stationary. 
	// 添加到分量速度的附加速度使得模拟的行为就好像演员正在高速移动，即使是静止的。
	// Vector is in world space. Units are cm/s. Could be used for a wind effects etc. Typical values are similar to the velocity of the object or effect, 
	// Vector is in world space.单位为厘米/秒。可用于风效果等。典型值类似于对象或效果的速度，
	// and usually around or less than 1000 for characters/wind.
	// 通常字符/风大约或小于 1000 个。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector ExternalLinearVelocity;

	// Additional angular velocity that is added to the component angular velocity. This can be used to make the simulation act as if the actor is rotating
	// 添加到分量角速度的附加角速度。这可用于使模拟表现得就像演员在旋转一样
	// even when it is not. E.g., to apply physics to a character on a podium as the camera rotates around it, to emulate the podium itself rotating.
	// 即使事实并非如此。例如，当摄像机围绕讲台上的角色旋转时，将物理应用到讲台上的角色，以模拟讲台本身的旋转。
	// Vector is in world space. Units are rad/s.
	// 向量位于世界空间中。单位为弧度/秒。
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector ExternalAngularVelocity;

	ANIMGRAPHRUNTIME_API void PostSerialize(const FArchive& Ar);
};

#if WITH_EDITORONLY_DATA
template<>
struct TStructOpsTypeTraits<FSimSpaceSettings> : public TStructOpsTypeTraitsBase2<FSimSpaceSettings>
{
	enum
	{
		WithPostSerialize = true
	};
};
#endif


/**
 *	Controller that simulates physics based on the physics asset of the skeletal mesh component
 */
USTRUCT()
struct FAnimNode_RigidBody : public FAnimNode_SkeletalControlBase
{
	GENERATED_USTRUCT_BODY()

	ANIMGRAPHRUNTIME_API FAnimNode_RigidBody();
	ANIMGRAPHRUNTIME_API ~FAnimNode_RigidBody();

	// FAnimNode_Base interface
	// FAnimNode_Base接口
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	// End of FAnimNode_Base interface
	// FAnimNode_Base接口结束

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void UpdateComponentPose_AnyThread(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void EvaluateComponentPose_AnyThread(FComponentSpacePoseContext& Output) override;
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual void OnInitializeAnimInstance(const FAnimInstanceProxy* InProxy, const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsOnInitializeAnimInstance() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual bool HasPreUpdate() const override { return true; }
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	ANIMGRAPHRUNTIME_API virtual bool NeedsDynamicReset() const override;
	ANIMGRAPHRUNTIME_API virtual void ResetDynamics(ETeleportType InTeleportType) override;
	ANIMGRAPHRUNTIME_API virtual int32 GetLODThreshold() const override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	ANIMGRAPHRUNTIME_API virtual void AddImpulseAtLocation(FVector Impulse, FVector Location, FName BoneName = NAME_None);

	// TEMP: Exposed for use in PhAt as a quick way to get drag handles working with Chaos
	// TEMP：在 PhAt 中公开使用，作为让拖动手柄与 Chaos 配合使用的快速方法
	virtual ImmediatePhysics::FSimulation* GetSimulation() { return PhysicsSimulation; }

	/**
	 * Set the override physics asset. This will automatically trigger a physics re-init in case the override physics asset changes. 
	 * Users can get access to this in the Animation Blueprint via the Animation Node Functions.
	 */
	void SetOverridePhysicsAsset(UPhysicsAsset* PhysicsAsset);

	UPhysicsAsset* GetPhysicsAsset() const { return UsePhysicsAsset; }

public:
	/** Physics asset to use. If empty use the skeletal mesh's default physics asset in case Default To Skeletal Mesh Physics Asset is set to True. */
	/** 要使用的物理资源。如果为空，则使用骨架网格物体的默认物理资源，以防默认骨架网格物体物理资源设置为 True。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	TObjectPtr<UPhysicsAsset> OverridePhysicsAsset;

	/** Use the skeletal mesh physics asset as default in case set to True. The Override Physics Asset will always have priority over this. */
	/** 如果设置为 True，则使用骨架网格物体物理资源作为默认值。覆盖物理资源将始终优先于此。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bDefaultToSkeletalMeshPhysicsAsset = true;

	/** Treat parts in the Physics Asset with PhysicsType Default as Simulated for RBAN. */
	/** 将物理资源中物理类型默认的部件视为 RBAN 模拟。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bUseDefaultAsSimulated = false;

private:
	/** Get the physics asset candidate to be used while respecting the bDefaultToSkeletalMeshPhysicsAsset and the priority to the override physics asset. */
	/** 获取要使用的候选物理资源，同时尊重 bDefaultToSkeletalMeshPhysicsAsset 和覆盖物理资源的优先级。 */
	UPhysicsAsset* GetPhysicsAssetToBeUsed(const UAnimInstance* InAnimInstance) const;

	FTransform PreviousCompWorldSpaceTM;
	FTransform CurrentTransform;
	FTransform PreviousTransform;

	UPhysicsAsset* UsePhysicsAsset;

public:
	/** Enable if you want to ignore the p.RigidBodyLODThreshold CVAR and force the node to solely use the LOD threshold. */
	/** 如果您想要忽略 p.RigidBodyLODThreshold CVAR 并强制节点仅使用 LOD 阈值，请启用。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Performance, meta = (PinHiddenByDefault))
	bool bUseLocalLODThresholdOnly = false;

	/** Override gravity*/
	/** 超越重力*/
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault, editcondition = "bOverrideWorldGravity"))
	FVector OverrideWorldGravity;

	/** Applies a uniform external force in world space. This allows for easily faking inertia of movement while still simulating in component space for example */
	/** 在世界空间中施加均匀的外力。例如，这可以轻松伪造运动惯性，同时仍然在组件空间中进行模拟 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinShownByDefault))
	FVector ExternalForce;

	/** When using non-world-space sim, this controls how much of the components world-space acceleration is passed on to the local-space simulation. */
	/** 当使用非世界空间模拟时，这控制有多少组件世界空间加速度传递到局部空间模拟。 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FVector ComponentLinearAccScale;

	/** When using non-world-space sim, this applies a 'drag' to the bodies in the local space simulation, based on the components world-space velocity. */
	/** 当使用非世界空间模拟时，这会根据组件世界空间速度对局部空间模拟中的物体施加“阻力”。 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FVector ComponentLinearVelScale;

	/** When using non-world-space sim, this is an overall clamp on acceleration derived from ComponentLinearAccScale and ComponentLinearVelScale, to ensure it is not too large. */
	/** 当使用非世界空间模拟时，这是对源自 ComponentLinearAccScale 和 ComponentLinearVelScale 的加速度的总体限制，以确保它不会太大。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	FVector	ComponentAppliedLinearAccClamp;

	/**
	 * Settings for the system which passes motion of the simulation's space
	 * into the simulation. This allows the simulation to pass a
	 * fraction of the world space motion onto the bodies which allows Bone-Space
	 * and Component-Space simulations to react to world-space movement in a
	 * controllable way.
	 * This system is a superset of the functionality provided by ComponentLinearAccScale,
	 * ComponentLinearVelScale, and ComponentAppliedLinearAccClamp. In general
	 * you should not have both systems enabled.
	 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FSimSpaceSettings SimSpaceSettings;


	/**
	 * Scale of cached bounds (vs. actual bounds).
	 * Increasing this may improve performance, but overlaps may not work as well.
	 * (A value of 1.0 effectively disables cached bounds).
	 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (ClampMin="1.0", ClampMax="2.0"))
	float CachedBoundsScale;

	/** Matters if SimulationSpace is BaseBone */
	/** 如果SimulationSpace 是BaseBone 则很重要 */
	UPROPERTY(EditAnywhere, Category = Settings)
	FBoneReference BaseBoneRef;

	/** The channel we use to find static geometry to collide with */
	/** 我们用来寻找静态几何体进行碰撞的通道 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (editcondition = "bEnableWorldGeometry"))
	TEnumAsByte<ECollisionChannel> OverlapChannel;

	/** What space to simulate the bodies in. This affects how velocities are generated */
	/** 模拟身体的空间。这会影响速度的生成方式 */
	UPROPERTY(EditAnywhere, Category = Settings)
	ESimulationSpace SimulationSpace;

	/** Whether to allow collisions between two bodies joined by a constraint  */
	/** 是否允许通过约束连接的两个实体之间发生碰撞  */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bForceDisableCollisionBetweenConstraintBodies;

	/** If true, kinematic objects will be added to the simulation at runtime to represent any cloth colliders defined for the parent object. */
	/** 如果为 true，则运动对象将在运行时添加到模拟中，以表示为父对象定义的任何布料碰撞器。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	bool bUseExternalClothCollision;

private:
	ETeleportType ResetSimulatedTeleportType;

public:
	UPROPERTY(EditAnywhere, Category = Settings, meta = (InlineEditConditionToggle))
	uint8 bEnableWorldGeometry : 1;

	UPROPERTY(EditAnywhere, Category = Settings, meta = (InlineEditConditionToggle))
	uint8 bOverrideWorldGravity : 1;

	/** 
		When simulation starts, transfer previous bone velocities (from animation)
		to make transition into simulation seamless.
	*/
	UPROPERTY(EditAnywhere, Category = Settings, meta=(PinHiddenByDefault))
	uint8 bTransferBoneVelocities : 1;

	/**
		When simulation starts, freeze incoming pose.
		This is useful for ragdolls, when we want the simulation to take over.
		It prevents non simulated bones from animating.
	*/
	UPROPERTY(EditAnywhere, Category = Settings)
	uint8 bFreezeIncomingPoseOnStart : 1;

	/**
		Correct for linear tearing on bodies with all axes Locked.
		This only works if all axes linear translation are locked
	*/
	UPROPERTY(EditAnywhere, Category = Settings)
	uint8 bClampLinearTranslationLimitToRefPose : 1;

	/**
		For world-space simulations, if the magnitude of the component's 3D scale is less than WorldSpaceMinimumScale, do not update the node.
	*/
	UPROPERTY(EditAnywhere, Category = Settings)
	float WorldSpaceMinimumScale;

	/**
		If the node is not evaluated for this amount of time (seconds), either because a lower LOD was in use for a while or the component was
		not visible, reset the simulation to the default pose on the next evaluation. Set to 0 to disable time-based reset.
	*/
	UPROPERTY(EditAnywhere, Category = Settings)
	float EvaluationResetTime;

private:
	uint8 bEnabled : 1;
	uint8 bSimulationStarted : 1;
	uint8 bCheckForBodyTransformInit : 1;

public:
	ANIMGRAPHRUNTIME_API void PostSerialize(const FArchive& Ar);

private:

#if WITH_EDITORONLY_DATA
	UPROPERTY()
	bool bComponentSpaceSimulation_DEPRECATED;	//use SimulationSpace
#endif

	// FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	// End of FAnimNode_SkeletalControlBase interface
	// FAnimNode_SkeletalControlBase接口结束

	ANIMGRAPHRUNTIME_API void InitPhysics(const UAnimInstance* InAnimInstance);
	ANIMGRAPHRUNTIME_API void UpdateWorldGeometry(const UWorld& World, const USkeletalMeshComponent& SKC);
	ANIMGRAPHRUNTIME_API void UpdateWorldForces(const FTransform& ComponentToWorld, const FTransform& RootBoneTM, const float DeltaSeconds);

	ANIMGRAPHRUNTIME_API void InitializeNewBodyTransformsDuringSimulation(FComponentSpacePoseContext& Output, const FTransform& ComponentTransform, const FTransform& BaseBoneTM);

	ANIMGRAPHRUNTIME_API void InitSimulationSpace(
		const FTransform& ComponentToWorld,
		const FTransform& BoneToComponent);

	// Calculate simulation space transform, velocity etc to pass into the solver
	// 计算模拟空间变换、速度等以传递给求解器
	ANIMGRAPHRUNTIME_API void CalculateSimulationSpaceMotion(
		ESimulationSpace Space,
		const FTransform& SpaceTransform,
		const FTransform& ComponentToWorld,
		const float Dt,
		const FSimSpaceSettings& Settings,
		FVector& SpaceLinearVel,
		FVector& SpaceAngularVel,
		FVector& SpaceLinearAcc,
		FVector& SpaceAngularAcc);

	// Gather cloth collision sources from the supplied Skeltal Mesh and add a kinematic actor representing each one of them to the sim.
	// 从提供的骨架网格体中收集布料碰撞源，并将代表每个碰撞源的运动学演员添加到模拟中。
	ANIMGRAPHRUNTIME_API void CollectClothColliderObjects(const USkeletalMeshComponent* SkeletalMeshComp);
	
	// Remove all cloth collider objects from the sim.
	// 从 sim 中移除所有布料碰撞对象。
	ANIMGRAPHRUNTIME_API void RemoveClothColliderObjects();

	// Update the sim-space transforms of all cloth collider objects.
	// 更新所有布料碰撞对象的模拟空间变换。
	ANIMGRAPHRUNTIME_API void UpdateClothColliderObjects(const FTransform& SpaceTransform);

	// Gather nearby world objects and add them to the sim
	// 收集附近的世界物体并将它们添加到 sim 中
	ANIMGRAPHRUNTIME_API void CollectWorldObjects();

	// Flag invalid world objects to be removed from the sim
	// 标记要从 sim 中删除的无效世界对象
	ANIMGRAPHRUNTIME_API void ExpireWorldObjects();

	// Remove simulation objects that are flagged as expired
	// 删除标记为过期的模拟对象
	ANIMGRAPHRUNTIME_API void PurgeExpiredWorldObjects();

	// Update sim-space transforms of world objects
	// 更新世界对象的模拟空间变换
	ANIMGRAPHRUNTIME_API void UpdateWorldObjects(const FTransform& SpaceTransform);

	// Advances the simulation by a given timestep
	// 按给定的时间步长推进模拟
	ANIMGRAPHRUNTIME_API void RunPhysicsSimulation(float DeltaSeconds, const FVector& SimSpaceGravity);

	// Waits for the deferred simulation task to complete if it's not already finished
	// 如果延迟的模拟任务尚未完成，则等待其完成
	ANIMGRAPHRUNTIME_API void FlushDeferredSimulationTask();

	// Destroy the simulation and free related structures
	// 销毁模拟和自由相关结构
	ANIMGRAPHRUNTIME_API void DestroyPhysicsSimulation();

public:

	/* Whether the physics simulation runs synchronously with the node's evaluation or is run in the background until the next frame. */
	/* 物理模拟是与节点的评估同步运行还是在后台运行直到下一帧。 */
	UPROPERTY(EditAnywhere, Category=Settings, AdvancedDisplay)
	ESimulationTiming SimulationTiming;

private:

	double WorldTimeSeconds;
	double LastEvalTimeSeconds;

	float AccumulatedDeltaTime;
	float AnimPhysicsMinDeltaTime;
	bool bSimulateAnimPhysicsAfterReset;
	/** This should only be used for removing the delegate during termination. Do NOT use this for any per frame work */
	/** 这应该仅用于在终止期间删除委托。不要将其用于任何每框架工作 */
	TWeakObjectPtr<USkeletalMeshComponent> SkelMeshCompWeakPtr;

	ImmediatePhysics::FSimulation* PhysicsSimulation;
	FPhysicsAssetSolverSettings SolverSettings;
	FSolverIterations SolverIterations;	// to be deprecated

	friend class FRigidBodyNodeSimulationTask;
	UE::Tasks::FTask SimulationTask;

	struct FOutputBoneData
	{
		FOutputBoneData()
			: CompactPoseBoneIndex(INDEX_NONE)
		{}

		TArray<FCompactPoseBoneIndex> BoneIndicesToParentBody;
		FCompactPoseBoneIndex CompactPoseBoneIndex;
		int32 BodyIndex;
		int32 ParentBodyIndex;
	};

	struct FBodyAnimData
	{
		FBodyAnimData()
			: TransferedBoneAngularVelocity(ForceInit)
			, TransferedBoneLinearVelocity(ForceInitToZero)
			, LinearXMotion(ELinearConstraintMotion::LCM_Locked)
			, LinearYMotion(ELinearConstraintMotion::LCM_Locked)
			, LinearZMotion(ELinearConstraintMotion::LCM_Locked)
			, LinearLimit(0.0f)
			, RefPoseLength (0.f)
			, bIsSimulated(false)
			, bBodyTransformInitialized(false)
		{}

		FQuat TransferedBoneAngularVelocity;
		FVector TransferedBoneLinearVelocity;

		ELinearConstraintMotion LinearXMotion;
		ELinearConstraintMotion LinearYMotion;
		ELinearConstraintMotion LinearZMotion;
		float LinearLimit;
		// we don't use linear limit but use default length to limit the bodies
		// 我们不使用线性限制，而是使用默认长度来限制主体
		// linear limits are defined per constraint - it can be any two joints that can limit
		// 线性限制是根据每个约束定义的 - 它可以是任何两个可以限制的关节
		// this is just default length of the local space from parent, and we use that info to limit
		// 这只是父级本地空间的默认长度，我们使用该信息来限制
		// the translation
		// 翻译
		float RefPoseLength;

		bool bIsSimulated : 1;
		bool bBodyTransformInitialized : 1;
	};

	struct FWorldObject
	{
		FWorldObject() : ActorHandle(nullptr), LastSeenTick(0), bExpired(false), bNew(true) {}
		FWorldObject(ImmediatePhysics::FActorHandle* InActorHandle, int32 InLastSeenTick) : ActorHandle(InActorHandle), LastSeenTick(InLastSeenTick), bExpired(false), bNew(true) {}

		ImmediatePhysics::FActorHandle* ActorHandle;
		int32 LastSeenTick;
		uint8 bExpired : 1;
		uint8 bNew : 1;
	};

	TArray<FOutputBoneData> OutputBoneData;
	TArray<ImmediatePhysics::FActorHandle*> Bodies;
	TArray<int32> SkeletonBoneIndexToBodyIndex;
	TArray<FBodyAnimData> BodyAnimData;

	TArray<FPhysicsConstraintHandle*> Constraints;
	TArray<USkeletalMeshComponent::FPendingRadialForces> PendingRadialForces;

	FPerSolverFieldSystem PerSolverField;

	// Information required to identify and update a kinematic object representing a cloth collision source in the sim.
	// 识别和更新代表 sim 中布料碰撞源的运动对象所需的信息。
	struct FClothCollider
	{
		FClothCollider(ImmediatePhysics::FActorHandle* const InActorHandle, const USkeletalMeshComponent* const InSkeletalMeshComponent, const uint32 InBoneIndex)
			: ActorHandle(InActorHandle)
			, SkeletalMeshComponent(InSkeletalMeshComponent)
			, BoneIndex(InBoneIndex)
		{}

		ImmediatePhysics::FActorHandle* ActorHandle; // Identifies the physics actor in the sim.
		const USkeletalMeshComponent* SkeletalMeshComponent; // Parent skeleton.
		uint32 BoneIndex; // Bone within parent skeleton that drives physics actors transform.
	};

	// List of actors in the sim that represent objects collected from other parts of this character.
	// 模拟中的演员列表，代表从该角色的其他部分收集的对象。
	TArray<FClothCollider> ClothColliders; 
	
	TMap<const UPrimitiveComponent*, FWorldObject> ComponentsInSim;
	int32 ComponentsInSimTick;

	FVector WorldSpaceGravity;

	double TotalMass;

	// Bounds used to gather world objects copied into the simulation
	// 用于收集复制到模拟中的世界对象的边界
	FSphere CachedBounds;

	FCollisionQueryParams QueryParams;

	FPhysScene* PhysScene;

	// Used by CollectWorldObjects and UpdateWorldGeometry in Task Thread
	// 由任务线程中的 CollectWorldObjects 和 UpdateWorldGeometry 使用
	// Typically, World should never be accessed off the Game Thread.
	// 通常，永远不应该从游戏线程中访问世界。
	// However, since we're just doing overlaps this should be OK.
	// 然而，由于我们只是做重叠，所以应该没问题。
	const UWorld* UnsafeWorld;

	// Used by CollectWorldObjects and UpdateWorldGeometry in Task Thread
	// 由任务线程中的 CollectWorldObjects 和 UpdateWorldGeometry 使用
	// Only used for a pointer comparison.
	// 仅用于指针比较。
	const AActor* UnsafeOwner;

	FBoneContainer CapturedBoneVelocityBoneContainer;
	FCSPose<FCompactHeapPose> CapturedBoneVelocityPose;
	FCSPose<FCompactHeapPose> CapturedFrozenPose;
	FBlendedHeapCurve CapturedFrozenCurves;

	FVector PreviousComponentLinearVelocity;

	// Used by the world-space to simulation-space motion transfer system in Component- or Bone-Space sims
	// 由组件空间或骨骼空间模拟中的世界空间到模拟空间运动传递系统使用
	FTransform PreviousSimulationSpaceTransform;
	FTransform PreviousPreviousSimulationSpaceTransform;
	float PreviousDt;

#if ENABLE_LOW_LEVEL_MEM_TRACKER
	FName OwningAssetPackageName;
	FName OwningAssetName;
#endif
};

#if WITH_EDITORONLY_DATA
template<>
struct TStructOpsTypeTraits<FAnimNode_RigidBody> : public TStructOpsTypeTraitsBase2<FAnimNode_RigidBody>
{
	enum
	{
		WithPostSerialize = true,
	};
};
#endif

inline FTransform SpaceToWorldTransform(
	ESimulationSpace Space, const FTransform& ComponentToWorld, const FTransform& BaseBoneTM)
{
	switch (Space)
	{
	case ESimulationSpace::ComponentSpace: return ComponentToWorld;
	case ESimulationSpace::WorldSpace: return FTransform::Identity;
	case ESimulationSpace::BaseBoneSpace: return BaseBoneTM * ComponentToWorld;
	default: return FTransform::Identity;
	}
}

inline FVector WorldVectorToSpaceNoScale(
	ESimulationSpace Space, const FVector& WorldDir, const FTransform& ComponentToWorld, const FTransform& BaseBoneTM)
{
	switch (Space)
	{
	case ESimulationSpace::ComponentSpace: return ComponentToWorld.InverseTransformVectorNoScale(WorldDir);
	case ESimulationSpace::WorldSpace: return WorldDir;
	case ESimulationSpace::BaseBoneSpace:
		return BaseBoneTM.InverseTransformVectorNoScale(ComponentToWorld.InverseTransformVectorNoScale(WorldDir));
	default: return FVector::ZeroVector;
	}
}

inline FVector WorldPositionToSpace(
	ESimulationSpace Space, const FVector& WorldPoint, const FTransform& ComponentToWorld, const FTransform& BaseBoneTM)
{
	switch (Space)
	{
	case ESimulationSpace::ComponentSpace: return ComponentToWorld.InverseTransformPosition(WorldPoint);
	case ESimulationSpace::WorldSpace: return WorldPoint;
	case ESimulationSpace::BaseBoneSpace:
		return BaseBoneTM.InverseTransformPosition(ComponentToWorld.InverseTransformPosition(WorldPoint));
	default: return FVector::ZeroVector;
	}
}

inline FTransform ConvertCSTransformToSimSpace(
	ESimulationSpace Space, const FTransform& InCSTransform, const FTransform& ComponentToWorld, const FTransform& BaseBoneTM)
{
	switch (Space)
	{
	case ESimulationSpace::ComponentSpace: return InCSTransform;
	case ESimulationSpace::WorldSpace:  return InCSTransform * ComponentToWorld;
	case ESimulationSpace::BaseBoneSpace: return InCSTransform.GetRelativeTransform(BaseBoneTM); break;
	default: ensureMsgf(false, TEXT("Unsupported Simulation Space")); return InCSTransform;
	}
}
