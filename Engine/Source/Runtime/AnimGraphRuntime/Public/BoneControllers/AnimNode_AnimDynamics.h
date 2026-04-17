// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Stats/Stats.h"
#include "UObject/ObjectMacros.h"
#include "BoneContainer.h"
#include "Animation/AnimPhysicsSolver.h"
#include "BonePose.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "Animation/AnimInstanceProxy.h"
#include "CommonAnimationTypes.h"
#include "AnimNode_AnimDynamics.generated.h"

class UAnimInstance;
class USkeletalMeshComponent;
template <class T> class TAutoConsoleVariable;

extern TAutoConsoleVariable<int32> CVarEnableDynamics;
extern ANIMGRAPHRUNTIME_API TAutoConsoleVariable<int32> CVarLODThreshold;
extern TAutoConsoleVariable<int32> CVarEnableWind;

#if ENABLE_ANIM_DRAW_DEBUG

extern TAutoConsoleVariable<int32> CVarShowDebug;
extern TAutoConsoleVariable<FString> CVarDebugBone;

#endif

DECLARE_CYCLE_STAT_EXTERN(TEXT("Anim Dynamics Overall"), STAT_AnimDynamicsOverall, STATGROUP_Physics, ANIMGRAPHRUNTIME_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Anim Dynamics Wind Data Update"), STAT_AnimDynamicsWindData, STATGROUP_Physics, ANIMGRAPHRUNTIME_API);
DECLARE_CYCLE_STAT_EXTERN(TEXT("Anim Dynamics Bone Evaluation"), STAT_AnimDynamicsBoneEval, STATGROUP_Physics, ANIMGRAPHRUNTIME_API);
DECLARE_DWORD_COUNTER_STAT_EXTERN(TEXT("Anim Dynamics Sub-Steps"), STAT_AnimDynamicsSubSteps, STATGROUP_Physics, ANIMGRAPHRUNTIME_API);

/** Supported angular constraint types */
/** 支持的角度约束类型 */
/** 支持的角度约束类型 */
/** 支持的角度约束类型 */
UENUM()
enum class AnimPhysAngularConstraintType : uint8
{
	Angular,
	Cone
};
/** 支持的线性轴约束 */

/** 支持的线性轴约束 */
/** Supported linear axis constraints */
/** 支持的线性轴约束 */
UENUM()
enum class AnimPhysLinearConstraintType : uint8
{
	Free,
	Limited,
};

UENUM(BlueprintType)
enum class AnimPhysSimSpaceType : uint8
{
	Component UMETA(ToolTip = "Sim origin is the location/orientation of the skeletal mesh component."),
	Actor UMETA(ToolTip = "Sim origin is the location/orientation of the actor containing the skeletal mesh component."),
	World UMETA(ToolTip = "Sim origin is the world origin. Teleporting characters is not recommended in this mode."),
	RootRelative UMETA(ToolTip = "Sim origin is the location/orientation of the root bone."),
/** 将刚体映射到骨骼参考的帮助器 */
	BoneRelative UMETA(ToolTip = "Sim origin is the location/orientation of the bone specified in RelativeSpaceBone"),
};
/** 将刚体映射到骨骼参考的帮助器 */

/** Helper mapping a rigid body to a bone reference */
/** 将刚体映射到骨骼参考的帮助器 */
struct FAnimPhysBoneRigidBody
{
	FAnimPhysBoneRigidBody(TArray<FAnimPhysShape>& Shapes, const FVector& Position, const FBoneReference& LinkedBone)
	: PhysBody(Shapes, Position)
	, BoundBone(LinkedBone)
	{}
/** 描述链接到可选父级的主体的帮助器（可以是 nullptr） */

	FAnimPhysRigidBody PhysBody;
	FBoneReference BoundBone;
/** 描述链接到可选父级的主体的帮助器（可以是 nullptr） */
};

/** Helper describing a body linked to an optional parent (can be nullptr) */
/** 描述链接到可选父级的主体的帮助器（可以是 nullptr） */
struct FAnimPhysLinkedBody
{
	FAnimPhysLinkedBody(TArray<FAnimPhysShape>& Shapes, const FVector& Position, const FBoneReference& LinkedBone)
	: RigidBody(Shapes, Position, LinkedBone)
/** 约束设置结构，保存构建物理约束所需的数据 */
	, ParentBody(nullptr)
	{}

	FAnimPhysBoneRigidBody RigidBody;
/** 约束设置结构，保存构建物理约束所需的数据 */
	FAnimPhysBoneRigidBody* ParentBody;
};

/** Constraint setup struct, holds data required to build a physics constraint */
/** 约束设置结构，保存构建物理约束所需的数据 */
USTRUCT()
struct FAnimPhysConstraintSetup
{
	GENERATED_BODY()

	FAnimPhysConstraintSetup()
	: LinearXLimitType(AnimPhysLinearConstraintType::Limited)
	, LinearYLimitType(AnimPhysLinearConstraintType::Limited)
	, LinearZLimitType(AnimPhysLinearConstraintType::Limited)
	, bLinearFullyLocked(false)
	, LinearAxesMin(ForceInitToZero)
	, LinearAxesMax(ForceInitToZero)
	, AngularConstraintType(AnimPhysAngularConstraintType::Angular)
	, TwistAxis(AnimPhysTwistAxis::AxisX)
	, AngularTargetAxis(AnimPhysTwistAxis::AxisX)
	, ConeAngle(0.0f)
#if WITH_EDITORONLY_DATA
	/** 是否限制直线X轴 */
	, AngularXAngle_DEPRECATED(0.0f)
	, AngularYAngle_DEPRECATED(0.0f)
	, AngularZAngle_DEPRECATED(0.0f)
#endif
	/** 是否限制直线Y轴 */
	, AngularLimitsMin(ForceInitToZero)
	/** 是否限制直线X轴 */
	, AngularLimitsMax(ForceInitToZero)
	, AngularTarget(ForceInitToZero)
	/** 是否限制直线Z轴 */
	{}

	/** 是否限制直线Y轴 */
	/** Whether to limit the linear X axis */
	/** 如果所有轴都被锁定，我们可以使用 3 个线性限制，而不是限制轴所需的 6 个 */
	/** 是否限制直线X轴 */
	UPROPERTY(EditAnywhere, Category = Linear)
	AnimPhysLinearConstraintType LinearXLimitType;
	/** 每轴最小线性移动（此处设置零并在最大限制内锁定） */
	/** 是否限制直线Z轴 */

	/** Whether to limit the linear Y axis */
	/** 是否限制直线Y轴 */
	/** 每轴最大线性运动（此处设置零并在最小限制内锁定） */
	UPROPERTY(EditAnywhere, Category = Linear)
	/** 如果所有轴都被锁定，我们可以使用 3 个线性限制，而不是限制轴所需的 6 个 */
	AnimPhysLinearConstraintType LinearYLimitType;

	/** 约束角运动时使用的方法 */
	/** Whether to limit the linear Z axis */
	/** 每轴最小线性移动（此处设置零并在最大限制内锁定） */
	/** 是否限制直线Z轴 */
	UPROPERTY(EditAnywhere, Category = Linear)
	/** 限制角运动时要考虑扭转的轴（前向轴） */
	AnimPhysLinearConstraintType LinearZLimitType;

	/** 每轴最大线性运动（此处设置零并在最小限制内锁定） */
	/** If all axes are locked we can use 3 linear limits instead of the 6 needed for limited axes */
	/** 如果所有轴都被锁定，我们可以使用 3 个线性限制，而不是限制轴所需的 6 个 */
	bool bLinearFullyLocked;

	/** 约束角运动时使用的方法 */
	/** Minimum linear movement per-axis (Set zero here and in the max limit to lock) */
	/** 每轴最小线性移动（此处设置零并在最大限制内锁定） */
	UPROPERTY(EditAnywhere, Category = Linear, meta = (UIMax = "0", ClampMax = "0"))
	FVector LinearAxesMin;
	/** 使用圆锥体进行约束时使用的角度 */
	/** 限制角运动时要考虑扭转的轴（前向轴） */

	/** Maximum linear movement per-axis (Set zero here and in the min limit to lock) */
	/** 每轴最大线性运动（此处设置零并在最小限制内锁定） */
	UPROPERTY(EditAnywhere, Category = Linear, meta = (UIMin = "0", ClampMin = "0"))
	/** 使用“角度”约束类型时角度运动的 X 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	FVector LinearAxesMax;

	/** Method to use when constraining angular motion */
	/** 约束角运动时使用的方法 */
	/** 使用“角度”约束类型时的 Y 轴角运动限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	UPROPERTY(EditAnywhere, Category = Angular)
	AnimPhysAngularConstraintType AngularConstraintType;

	/** Axis to consider for twist when constraining angular motion (forward axis) */
	/** 使用“角度”约束类型时角度运动的 Z 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	/** 使用圆锥体进行约束时使用的角度 */
	/** 限制角运动时要考虑扭转的轴（前向轴） */
	UPROPERTY(EditAnywhere, Category = Angular)
	AnimPhysTwistAxis TwistAxis;

	/**
	/** 使用“角度”约束类型时角度运动的 X 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	 * The axis in the simulation pose to align to the Angular Target.
	 * This is typically the axis pointing along the bone.
	 * Note: This is affected by the Angular Spring Constant.
	 */
	/** 使用“角度”约束类型时的 Y 轴角运动限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	UPROPERTY(EditAnywhere, Category = Angular, meta=(DisplayAfter=AngularLimitsMax))
	AnimPhysTwistAxis AngularTargetAxis;

	/** Angle to use when constraining using a cone */
	/** 使用“角度”约束类型时角度运动的 Z 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	/** 使用圆锥体进行约束时使用的角度 */
	UPROPERTY(EditAnywhere, Category = Angular, meta = (UIMin = "0", UIMax = "90", ClampMin = "0", ClampMax = "90"))
	float ConeAngle;

#if WITH_EDITORONLY_DATA
	/** X-axis limit for angular motion when using the "Angular" constraint type (Set to 0 to lock, or 180 to remain free) */
	/** 使用“角度”约束类型时角度运动的 X 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	UPROPERTY()
	float AngularXAngle_DEPRECATED;
	/** 当使用驱动骨骼时，平面变换将相对于骨骼变换 */

	/** Y-axis limit for angular motion when using the "Angular" constraint type (Set to 0 to lock, or 180 to remain free) */
	/** 使用“角度”约束类型时的 Y 轴角运动限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	UPROPERTY()
	float AngularYAngle_DEPRECATED;

	/** Z-axis limit for angular motion when using the "Angular" constraint type (Set to 0 to lock, or 180 to remain free) */
	/** 使用“角度”约束类型时角度运动的 Z 轴限制（设置为 0 进行锁定，或设置为 180 保持自由） */
	UPROPERTY()
	float AngularZAngle_DEPRECATED;
#endif
/** 球体是否将物体保持在其形状内部或外部 */

	UPROPERTY(EditAnywhere, Category = Angular, meta = (UIMin = "-180", UIMax = "180", ClampMin = "-180", ClampMax = "180"))
	FVector AngularLimitsMin;

	UPROPERTY(EditAnywhere, Category = Angular, meta = (UIMin = "-180", UIMax = "180", ClampMin = "-180", ClampMax = "180"))
	FVector AngularLimitsMax;
	/** 当使用驱动骨骼时，平面变换将相对于骨骼变换 */

	/**
	 * The axis to align the angular spring constraint to in the animation pose.
	 * This typically points down the bone - so values of (1.0, 0.0, 0.0) are common,
	 * but you can pick other values to align the spring to a different direction.
	 * Note: This is affected by the Angular Spring Constant.
	 */
	UPROPERTY(EditAnywhere, Category = Angular)
	FVector AngularTarget;
};

/** 球体是否将物体保持在其形状内部或外部 */
	/** 用于连接球体的骨骼 */
USTRUCT()
struct FAnimPhysPlanarLimit
{
	GENERATED_BODY();
	/** 球体的局部偏移，如果没有设置驱动骨骼，则在节点空间中，否则在骨骼空间中 */

	/** When using a driving bone, the plane transform will be relative to the bone transform */
	/** 当使用驱动骨骼时，平面变换将相对于骨骼变换 */
	UPROPERTY(EditAnywhere, Category=PlanarLimit)
	/** 球体半径 */
	FBoneReference DrivingBone;

	/** Transform of the plane, this is either in component-space if no DrivinBone is specified
	 *  or in bone-space if a driving bone is present.
	/** 是否将物体锁定在球体内部或外部 */
	 */
	UPROPERTY(EditAnywhere, Category=PlanarLimit)
	FTransform PlaneTransform;
};

/** Whether spheres keep bodies inside, or outside of their shape */
/** 球体是否将物体保持在其形状内部或外部 */
	/** 用于连接球体的骨骼 */
UENUM()
enum class ESphericalLimitType : uint8
{
	Inner,
	/** 球体的局部偏移，如果没有设置驱动骨骼，则在节点空间中，否则在骨骼空间中 */
	Outer
};

USTRUCT()
	/** 球体半径 */
struct FAnimPhysSphericalLimit
{
	GENERATED_BODY();

	/** 是否将物体锁定在球体内部或外部 */
	FAnimPhysSphericalLimit()
		: SphereLocalOffset(FVector::ZeroVector)
		, LimitRadius(0.0f)
		, LimitType(ESphericalLimitType::Outer)
	{}

	/** Bone to attach the sphere to */
	/** 用于连接球体的骨骼 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit)
	/** 用于模拟的盒子的范围 */
	FBoneReference DrivingBone;

	/** Local offset for the sphere, if no driving bone is set this is in node space, otherwise bone space */
	/** 球体的局部偏移，如果没有设置驱动骨骼，则在节点空间中，否则在骨骼空间中 */
	/** 相对于被模拟的身体以附加约束的向量 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit)
	FVector SphereLocalOffset;

	/** Radius of the sphere */
	/** 描述我们将应用于身体的约束的数据 */
	/** 球体半径 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit)
	float LimitRadius;

	/** 平面极限的解析方法 */
	/** Whether to lock bodies inside or outside of the sphere */
	/** 是否将物体锁定在球体内部或外部 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit)
	ESphericalLimitType LimitType;
	/** CollisionType 设置为 CustomSphere 时使用的半径 */
};

USTRUCT()
struct FAnimPhysBodyDefinition
{
	GENERATED_BODY();

	/** 用于模拟的盒子的范围 */
	FAnimPhysBodyDefinition()
	: BoxExtents(10.0f, 10.0f, 10.0f)
	, LocalJointOffset(FVector::ZeroVector)
	, CollisionType(AnimPhysCollisionType::CoM)
	/** 相对于被模拟的身体以附加约束的向量 */
	, SphereCollisionRadius(10.0f)
	{}

	FAnimPhysBodyDefinition& operator=(const FAnimPhysBodyDefinition& Other)
	/** 描述我们将应用于身体的约束的数据 */
	{
		this->BoundBone = Other.BoundBone;
		this->BoxExtents = Other.BoxExtents;
		this->LocalJointOffset = Other.LocalJointOffset;
	/** 平面极限的解析方法 */
		this->ConstraintSetup = Other.ConstraintSetup;
		this->CollisionType = Other.CollisionType;
		this->SphereCollisionRadius = Other.SphereCollisionRadius;

	/** CollisionType 设置为 CustomSphere 时使用的半径 */
		return *this;
	}

	UPROPERTY(VisibleAnywhere, Category = PhysicsBodyDefinition, meta = (EditCondition = "false"))
	FBoneReference BoundBone;

	/** Extents of the box to use for simulation */
	/** 用于模拟的盒子的范围 */
	UPROPERTY(EditAnywhere, Category = PhysicsBodyDefinition, meta = (UIMin = "1", ClampMin = "1"))
	FVector BoxExtents;

	/** Vector relative to the body being simulated to attach the constraint to */
	/** 相对于被模拟的身体以附加约束的向量 */
	UPROPERTY(EditAnywhere, Category = PhysicsBodyDefinition)
	FVector LocalJointOffset;

	/** Data describing the constraints we will apply to the body */
	/** 描述我们将应用于身体的约束的数据 */
	UPROPERTY(EditAnywhere, Category = Constraint)
	FAnimPhysConstraintSetup ConstraintSetup;

	/** Resolution method for planar limits */
	/** 平面极限的解析方法 */
	UPROPERTY(EditAnywhere, Category = Collision)
	AnimPhysCollisionType CollisionType;

	/** Radius to use if CollisionType is set to CustomSphere */
	/** CollisionType 设置为 CustomSphere 时使用的半径 */
	UPROPERTY(EditAnywhere, Category = PhysicsBodyDefinition, meta = (UIMin = "1", ClampMin = "1", EditCondition = "CollisionType == AnimPhysCollisionType::CustomSphere"))
	float SphereCollisionRadius;
};

struct FAnimConstraintOffsetPair
{
	FAnimConstraintOffsetPair(const FVector& InBody0Offset, const FVector InBody1Offset)
	: Body0Offset(InBody0Offset)
	, Body1Offset(InBody1Offset)
	{}

	FVector Body0Offset;
	FVector Body1Offset;
};

/**
 * Settings for the system which passes rotational motion of the simulation's space into the simulation. This allows the simulation to pass a 
 * fraction of the world space motion onto the bodies which allows Bone-Space and Component-Space simulations to react to world-space 
 * movement in a controllable way.
 */
template <> struct TIsPODType<FAnimPhysSimSpaceSettings> { enum { Value = true }; };

USTRUCT(BlueprintType)
struct FAnimPhysSimSpaceSettings
{
	GENERATED_BODY()

	ANIMGRAPHRUNTIME_API FAnimPhysSimSpaceSettings();

	~FAnimPhysSimSpaceSettings() = default;
	FAnimPhysSimSpaceSettings(FAnimPhysSimSpaceSettings const&) = default;
	FAnimPhysSimSpaceSettings& operator=(const FAnimPhysSimSpaceSettings&) = default;

	/**
	 * Global multipler on the effects of simulation space rotational movement. Must be in range[0, 1].If SimSpaceAngularAlpha = 0.0, the system is disabled and the simulation will
	 * be fully local (i.e., world-space actor movement and rotation does not affect the simulation).
	/** 当在 BoneRelative sim 空间中时，模拟将使用该骨骼作为原点 */
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SimSpaceAngularAlpha;

	/** 附加物理体的骨骼，如果 bChain 为 true，则这是链的顶部 */
	/**
	 * A clamp on the effective world-space angular velocity that is passed to the simulation. Units are radian/s, so a value of about 6.0 is one rotation per second.
	 * The default value effectively means "unlimited". You would reduce this (and MaxAngularAcceleration) to limit how much bodies "fly out" when the actor spins on the spot.
	 * This is especially useful if you have characters than can rotate very quickly and you would probably want values around or less than 10 in this case.
	/** 如果 bChain 为 true，则这是链的底部，否则忽略 */
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxAngularVelocity;
	
	/**
	 * A clamp on the effective world-space angular accleration that is passed to the simulation. Units are radian/s/s. The default value effectively means "unlimited".
	 * This has a similar effect to MaxAngularVelocity, except that it is related to the flying out of bodies when the rotation speed suddenly changes. Typical limits for
	/** 重力比例，较高的值会增加重力造成的力 */
	 * a character might be around 100.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings, meta = (ClampMin = "0.0"))
	float MaxAngularAcceleration;
	/** 重力覆盖值 */

	/**
	 * Additional angular velocity that is added to the component angular velocity. This can be used to make the simulation act as if the actor is rotating
	 * even when it is not. E.g., to apply physics to a character on a podium as the camera rotates around it, to emulate the podium itself rotating.
	 * Vector is in world space. Units are rad/s.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Settings)
	FVector ExternalAngularVelocity;

	/** 当在 BoneRelative sim 空间中时，模拟将使用该骨骼作为原点 */
};


USTRUCT(BlueprintInternalUseOnly)
	/** 附加物理体的骨骼，如果 bChain 为 true，则这是链的顶部 */
struct FAnimNode_AnimDynamics : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY();

	/** 适用于求解器中计算的风速的比例 */
	/** 如果 bChain 为 true，则这是链的底部，否则忽略 */
	ANIMGRAPHRUNTIME_API FAnimNode_AnimDynamics();

	/**
	/** 当使用非世界空间模拟时，这控制有多少组件世界空间加速度传递到局部空间模拟。 */
	* Overridden linear damping value. The default is 0.7. Values below 0.7 won't have an effect.
	*/
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "NumSolverIterationsPostUpdate", EditCondition = bOverrideLinearDamping))
	float LinearDampingOverride;
	/** 当使用非世界空间模拟时，这会根据组件世界空间速度对局部空间模拟中的物体施加“阻力”。 */
	/** 重力比例，较高的值会增加重力造成的力 */

	/**
	 * Overridden angular damping value. The default is 0.7. Values below 0.7 won't have an effect.
	/** 当使用非世界空间模拟时，这是对源自 ComponentLinearAccScale 和 ComponentLinearVelScale 的加速度的总体限制，以确保它不会太大。 */
	 */
	/** 重力覆盖值 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "LinearDampingOverride", EditCondition = bOverrideAngularDamping))
	float AngularDampingOverride;

	// Previous component & actor transforms, used to account for teleports
 // 之前的组件和 actor 变换，用于考虑传送
	FTransform PreviousCompWorldSpaceTM;
	FTransform PreviousActorWorldSpaceTM;
	FTransform PreviousSimSpaceTM;

	/** When in BoneRelative sim space, the simulation will use this bone as the origin */
	/** 当在 BoneRelative sim 空间中时，模拟将使用该骨骼作为原点 */
	UPROPERTY(EditAnywhere, Category = Setup, meta=(DisplayAfter="SimulationSpace", EditCondition = "SimulationSpace == AnimPhysSimSpaceType::BoneRelative"))
	FBoneReference RelativeSpaceBone;

	/** The bone to attach the physics body to, if bChain is true this is the top of the chain */
	/** 附加物理体的骨骼，如果 bChain 为 true，则这是链的顶部 */
	UPROPERTY(EditAnywhere, Category = Setup)
	FBoneReference BoundBone;

	/** 适用于求解器中计算的风速的比例 */
	/** If bChain is true this is the bottom of the chain, otherwise ignored */
	/** 如果 bChain 为 true，则这是链的底部，否则忽略 */
	UPROPERTY(EditAnywhere, Category = Setup, meta = (EditCondition = bChain, DisplayAfter = "BoundBone"))
	FBoneReference ChainEnd;
	/** 在我们求解物体位置之前，线性和角度限制上的更新次数建议为 NumSolverIterationsPostUpdate 值的四倍 */
	/** 当使用非世界空间模拟时，这控制有多少组件世界空间加速度传递到局部空间模拟。 */

	UPROPERTY(EditAnywhere, EditFixedSize, Category = PhysicsParameters, meta = (DisplayName = "Body Definitions", EditFixedOrder, DisplayAfter = "ChainEnd"))
	TArray< FAnimPhysBodyDefinition > PhysicsBodyDefinitions;
	/** 求解物体位置后，线性和角度限制上的更新次数，建议约为 NumSolverIterationsPreUpdate 的四分之一 */

	/** 当使用非世界空间模拟时，这会根据组件世界空间速度对局部空间模拟中的物体施加“阻力”。 */
	/** Scale for gravity, higher values increase forces due to gravity */
	/** 重力比例，较高的值会增加重力造成的力 */
	/** 该节点的可用球形限制列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "bGravityOverrideInSimSpace", EditCondition = "!bUseGravityOverride"))
	float GravityScale;
	/** 当使用非世界空间模拟时，这是对源自 ComponentLinearAccScale 和 ComponentLinearVelScale 的加速度的总体限制，以确保它不会太大。 */

	/** 勾选后应用于模拟中所有物体的外力，在世界空间中指定 */
	/** Gravity Override Value */
	/** 重力覆盖值 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "bUseGravityOverride", EditCondition = "bUseGravityOverride"))
	FVector GravityOverride;
	/** 该节点的可用平面限制列表 */

	/** 
	 * Spring constant to use when calculating linear springs, higher values mean a stronger spring.
	 * You need to enable the Linear Spring checkbox for this to have an effect.
	/** 用于运行模拟的空间 */
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsParameters, meta = (EditCondition = bLinearSpring, PinHiddenByDefault, DisplayAfter = "AngularSpringConstant"))
	float LinearSpringConstant;

	/** 
	 * Spring constant to use when calculating angular springs, higher values mean a stronger spring.
	 * You need to enable the Angular Spring checkbox for this to have an effect.
	 * Note: Make sure to also set the Angular Target Axis and Angular Target in the Constraint Setup for this to have an effect.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsParameters, meta = (DisplayAfter = "PhysicsBodyDefinitions", EditCondition = bAngularSpring, PinHiddenByDefault))
	float AngularSpringConstant;
	/** 是否评估球面极限 */

	/** Scale to apply to calculated wind velocities in the solver */
	/** 适用于求解器中计算的风速的比例 */
	UPROPERTY(EditAnywhere, Category = Wind, meta=(DisplayAfter="bEnableWind"))
	/** 是否评估平面限制 */
	float WindScale;
	/** 在我们求解物体位置之前，线性和角度限制上的更新次数建议为 NumSolverIterationsPostUpdate 值的四倍 */

	/** When using non-world-space sim, this controls how much of the components world-space acceleration is passed on to the local-space simulation. */
	/** 如果为真，我们将执行物理更新，否则跳过 - 允许物体初始状态的可视化 */
	/** 当使用非世界空间模拟时，这控制有多少组件世界空间加速度传递到局部空间模拟。 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	/** 求解物体位置后，线性和角度限制上的更新次数，建议约为 NumSolverIterationsPreUpdate 的四分之一 */
	FVector ComponentLinearAccScale;
	/** 如果为真，我们将执行骨骼变换评估，否则跳过 - 允许与物理模拟相比的初始动画状态的可视化 */

	/** When using non-world-space sim, this applies a 'drag' to the bodies in the local space simulation, based on the components world-space velocity. */
	/** 当使用非世界空间模拟时，这会根据组件世界空间速度对局部空间模拟中的物体施加“阻力”。 */
	/** 该节点的可用球形限制列表 */
	/** 如果为 true，则覆盖值将用于线性阻尼 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	FVector ComponentLinearVelScale;

	/** When using non-world-space sim, this is an overall clamp on acceleration derived from ComponentLinearAccScale and ComponentLinearVelScale, to ensure it is not too large. */
	/** 勾选后应用于模拟中所有物体的外力，在世界空间中指定 */
	/** 当使用非世界空间模拟时，这是对源自 ComponentLinearAccScale 和 ComponentLinearVelScale 的加速度的总体限制，以确保它不会太大。 */
	UPROPERTY(EditAnywhere, Category = Settings)
	FVector	ComponentAppliedLinearAccClamp;

	/** 该节点的可用平面限制列表 */
	/**
	 * Settings for the system which passes motion of the simulation's space
	/** 如果为 true，则覆盖值将用于角度阻尼 */
	 * into the simulation. This allows the simulation to pass a
	 * fraction of the world space motion onto the bodies which allows Bone-Space
	/** 用于运行模拟的空间 */
	 * and Component-Space simulations to react to world-space movement in a
	/** 在此模拟中是否为物体启用风 */
	 * controllable way.
	 * This system is a superset of the functionality provided by ComponentLinearAccScale,
	 * ComponentLinearVelScale, and ComponentAppliedLinearAccClamp. In general
	 * you should not have both systems enabled.
	 */
	UPROPERTY(EditAnywhere, Category = Settings, meta = (PinHiddenByDefault))
	/** 使用重力覆盖值与重力比例 */
	FAnimPhysSimSpaceSettings SimSpaceSettings;

	/** Overridden angular bias value
	 *  Angular bias is essentially a twist reduction for chain forces and defaults to a value to keep chains stability
	/** 如果为 true，则重力覆盖值在模拟空间中定义，默认情况下它在世界空间中 */
	/** 是否评估球面极限 */
	*  in check. When using single-body systems sometimes angular forces will look like they are "catching-up" with
	*  the mesh, if that's the case override this and push it towards 1.0f until it settles correctly
	*/
	/** 如果为真，身体将尝试弹回其初始位置 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, BlueprintReadWrite, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "AngularDampingOverride", EditCondition = bOverrideAngularBias))
	/** 是否评估平面限制 */
	float AngularBiasOverride;

	/** 如果为 true，则主体将尝试将自身与指定的角度目标对齐 */
	/** Number of update passes on the linear and angular limits before we solve the position of the bodies recommended to be four times the value of NumSolverIterationsPostUpdate */
	/** 在我们求解物体位置之前，线性和角度限制上的更新次数建议为 NumSolverIterationsPostUpdate 值的四倍 */
	/** 如果为真，我们将执行物理更新，否则跳过 - 允许物体初始状态的可视化 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters)
	/** 设置为 true 以使用求解器模拟连接链 */
	int32 NumSolverIterationsPreUpdate;

	/** Number of update passes on the linear and angular limits after we solve the position of the bodies, recommended to be around a quarter of NumSolverIterationsPreUpdate */
	/** 如果为真，我们将执行骨骼变换评估，否则跳过 - 允许与物理模拟相比的初始动画状态的可视化 */
	/** 旋转重定向的设置 */
	/** 求解物体位置后，线性和角度限制上的更新次数，建议约为 NumSolverIterationsPreUpdate 的四分之一 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta = (PinHiddenByDefault, DisplayAfter = "NumSolverIterationsPreUpdate"))
	int32 NumSolverIterationsPostUpdate;

	/** 如果为 true，则覆盖值将用于线性阻尼 */
	/** List of available spherical limits for this node */
	/** 该节点的可用球形限制列表 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit, meta=(DisplayAfter="bUseSphericalLimits"))
	TArray<FAnimPhysSphericalLimit> SphericalLimits;

	/** An external force to apply to all bodies in the simulation when ticked, specified in world space */
	/** 勾选后应用于模拟中所有物体的外力，在世界空间中指定 */
	UPROPERTY(EditAnywhere, Category = Forces, meta = (PinShownByDefault))
	FVector ExternalForce;

	/** List of available planar limits for this node */
	/** 该节点的可用平面限制列表 */
	/** 如果为 true，则覆盖值将用于角度阻尼 */
	UPROPERTY(EditAnywhere, Category=PlanarLimit, meta=(DisplayAfter="bUsePlanarLimit"))
	TArray<FAnimPhysPlanarLimit> PlanarLimits;

	/** The space used to run the simulation */
	/** 在此模拟中是否为物体启用风 */
	/** 用于运行模拟的空间 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Setup, meta = (PinHiddenByDefault, DisplayPriority=0))
	AnimPhysSimSpaceType SimulationSpace;

	// Cached sim space that we last used
 // 我们上次使用的缓存 sim 空间
	/** 使用重力覆盖值与重力比例 */
	AnimPhysSimSpaceType LastSimSpace;

	// We can't get clean bone positions unless we are in the evaluate step.
 // 除非我们处于评估步骤，否则我们无法获得干净的骨骼位置。
	/** 如果为 true，则重力覆盖值在模拟空间中定义，默认情况下它在世界空间中 */
	// Requesting an init or reinit sets this flag for us to pick up during evaluate
 // 请求 init 或 reinit 会设置此标志，以便我们在评估期间拾取
	ETeleportType InitTeleportType;

	/** 如果为真，身体将尝试弹回其初始位置 */
	/** Whether to evaluate spherical limits */
	/** 是否评估球面极限 */
	UPROPERTY(EditAnywhere, Category = SphericalLimit)
	uint8 bUseSphericalLimits:1;
	/** 如果为 true，则主体将尝试将自身与指定的角度目标对齐 */

	/** Whether to evaluate planar limits */
	/** 是否评估平面限制 */
	UPROPERTY(EditAnywhere, Category=PlanarLimit)
	/** 设置为 true 以使用求解器模拟连接链 */
	uint8 bUsePlanarLimit:1;

	/** If true we will perform physics update, otherwise skip - allows visualization of the initial state of the bodies */
	/** 如果为真，我们将执行物理更新，否则跳过 - 允许物体初始状态的可视化 */
	/** 旋转重定向的设置 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta = (DisplayAfter = "bDoEval"))
	uint8 bDoUpdate : 1;

	/** If true we will perform bone transform evaluation, otherwise skip - allows visualization of the initial anim state compared to the physics sim */
	/** 如果为真，我们将执行骨骼变换评估，否则跳过 - 允许与物理模拟相比的初始动画状态的可视化 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta = (DisplayAfter = "AngularBiasOverride"))
	uint8 bDoEval : 1;

	/** If true, the override value will be used for linear damping */
	/** 如果为 true，则覆盖值将用于线性阻尼 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta=(InlineEditConditionToggle, DisplayAfter="AngularSpringConstraint"))
	uint8 bOverrideLinearDamping:1;

	/** If true, the override value will be used for the angular bias for bodies in this node. 
	 *  Angular bias is essentially a twist reduction for chain forces and defaults to a value to keep chains stability
	 *  in check. When using single-body systems sometimes angular forces will look like they are "catching-up" with
	 *  the mesh, if that's the case override this and push it towards 1.0f until it settles correctly
	 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta=(InlineEditConditionToggle, DisplayAfter="AngularDampingOverride"))
	uint8 bOverrideAngularBias:1;

	/** If true, the override value will be used for angular damping */
	/** 如果为 true，则覆盖值将用于角度阻尼 */
	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = PhysicsParameters, meta=(InlineEditConditionToggle, DisplayAfter="LinearDampingOverride"))
	uint8 bOverrideAngularDamping:1;

	/** Whether or not wind is enabled for the bodies in this simulation */
	/** 在此模拟中是否为物体启用风 */
	UPROPERTY(EditAnywhere, Category = Wind)
	uint8 bEnableWind:1;

	uint8 bWindWasEnabled:1;

	/** Use gravity override value vs gravity scale */
	/** 使用重力覆盖值与重力比例 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = PhysicsParameters, meta = (DisplayAfter = "LinearSpringConstant"))
	uint8 bUseGravityOverride:1;

	/** If true the gravity override value is defined in simulation space, by default it is in world space */
	/** 如果为 true，则重力覆盖值在模拟空间中定义，默认情况下它在世界空间中 */
	UPROPERTY(EditAnywhere, Category = PhysicsParameters, meta=(DisplayAfter = "GravityOverride", DisplayName = "Gravity Override In Sim Space", EditCondition = "bUseGravityOverride"))
	uint8 bGravityOverrideInSimSpace : 1;

	/** If true the body will attempt to spring back to its initial position */
	/** 如果为真，身体将尝试弹回其初始位置 */
	UPROPERTY(EditAnywhere, Category = PhysicsParameters, meta = (InlineEditConditionToggle))
	uint8 bLinearSpring:1;

	/** If true the body will attempt to align itself with the specified angular target */
	/** 如果为 true，则主体将尝试将自身与指定的角度目标对齐 */
	UPROPERTY(EditAnywhere, Category = PhysicsParameters, meta = (InlineEditConditionToggle))
	uint8 bAngularSpring:1;

	/** Set to true to use the solver to simulate a connected chain */
	/** 设置为 true 以使用求解器模拟连接链 */
	UPROPERTY(EditAnywhere, Category = Setup, meta=(InlineEditConditionToggle))
	uint8 bChain:1;

	/** The settings for rotation retargeting */
	/** 旋转重定向的设置 */
	UPROPERTY(EditAnywhere, Category = Retargeting)
	FRotationRetargetingInfo RetargetingSettings;

#if WITH_EDITORONLY_DATA
	UPROPERTY()
		FVector BoxExtents_DEPRECATED;
	UPROPERTY()
		FVector LocalJointOffset_DEPRECATED;
	UPROPERTY()
		FAnimPhysConstraintSetup ConstraintSetup_DEPRECATED;
	UPROPERTY()
		AnimPhysCollisionType CollisionType_DEPRECATED;
	UPROPERTY()
		float SphereCollisionRadius_DEPRECATED;
#endif

	// FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口
	ANIMGRAPHRUNTIME_API virtual void Initialize_AnyThread(const FAnimationInitializeContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	ANIMGRAPHRUNTIME_API virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms) override;
	ANIMGRAPHRUNTIME_API virtual void GatherDebugData(FNodeDebugData& DebugData) override;
	ANIMGRAPHRUNTIME_API virtual bool HasPreUpdate() const override;
	ANIMGRAPHRUNTIME_API virtual void PreUpdate(const UAnimInstance* InAnimInstance) override;
	virtual bool NeedsDynamicReset() const override { return true; }
	virtual void ResetDynamics(ETeleportType InTeleportType) override { RequestInitialise(InTeleportType); }
	ANIMGRAPHRUNTIME_API virtual int32 GetLODThreshold() const override;
	// End of FAnimNode_SkeletalControlBase interface
 // FAnimNode_SkeletalControlBase接口结束

	ANIMGRAPHRUNTIME_API void RequestInitialise(ETeleportType InTeleportType);
	ANIMGRAPHRUNTIME_API void InitPhysics(FComponentSpacePoseContext& Output);
	ANIMGRAPHRUNTIME_API void TermPhysics();

	ANIMGRAPHRUNTIME_API void UpdateChainPhysicsBodyDefinitions(const FReferenceSkeleton& ReferenceSkeleton);
	ANIMGRAPHRUNTIME_API void ValidateChainPhysicsBodyDefinitions(const FReferenceSkeleton& ReferenceSkeleton);
	ANIMGRAPHRUNTIME_API void FindChainBoneNames(const FReferenceSkeleton& ReferenceSkeleton, TArray<FName>& ChainBoneNames);
	ANIMGRAPHRUNTIME_API void UpdateLimits(FComponentSpacePoseContext& Output);

	ANIMGRAPHRUNTIME_API int32 GetNumBodies() const;
	ANIMGRAPHRUNTIME_API const FAnimPhysRigidBody& GetPhysBody(int32 BodyIndex) const;

	ANIMGRAPHRUNTIME_API FTransform GetBodyComponentSpaceTransform(const FAnimPhysRigidBody& Body, const USkeletalMeshComponent* const SkelComp) const;

#if WITH_EDITOR

	// Accessors for editor code (mainly for visualization functions)
 // 编辑器代码的访问器（主要用于可视化功能）
	ANIMGRAPHRUNTIME_API FVector GetBodyLocalJointOffset(const int32 BodyIndex) const;

	// True by default, if false physics simulation will not update this frame. Used to prevent the rig moving whilst interactively editing parameters with a widget in the viewport.
 // 默认情况下为 true，如果 false 物理模拟将不会更新此帧。用于防止装备移动，同时使用视口中的小部件交互式编辑参数。
	bool bDoPhysicsUpdateInEditor;

#endif

	ANIMGRAPHRUNTIME_API bool ShouldDoPhysicsUpdate() const;

protected:

	// FAnimNode_SkeletalControlBase protected interface
 // FAnimNode_SkeletalControlBase 受保护接口
	ANIMGRAPHRUNTIME_API virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	ANIMGRAPHRUNTIME_API virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones);
	// End of FAnimNode_SkeletalControlBase protected interface
 // FAnimNode_SkeletalControlBase 受保护接口结束

private:
	// Given a bone index, get it's transform in the currently selected simulation space
 // 给定一个骨骼索引，获取它在当前选择的模拟空间中的变换
	ANIMGRAPHRUNTIME_API FTransform GetBoneTransformInSimSpace(FComponentSpacePoseContext& Output, const FCompactPoseBoneIndex& BoneIndex) const;

	// Given a transform in simulation space, convert it back to component space
 // 给定模拟空间中的变换，将其转换回组件空间
	ANIMGRAPHRUNTIME_API FTransform GetComponentSpaceTransformFromSimSpace(AnimPhysSimSpaceType SimSpace, FComponentSpacePoseContext& Output, const FTransform& InSimTransform) const;
	ANIMGRAPHRUNTIME_API FTransform GetComponentSpaceTransformFromSimSpace(AnimPhysSimSpaceType SimSpace, FComponentSpacePoseContext& Output, const FTransform& InSimTransform, const FTransform& InCompWorldSpaceTM, const FTransform& InActorWorldSpaceTM) const;
	ANIMGRAPHRUNTIME_API FTransform GetComponentSpaceTransformFromSimSpace(AnimPhysSimSpaceType SimSpace, const USkeletalMeshComponent* const SkelComp, const FTransform& InSimTransform) const;

	FTransform GetSimToWorldSpaceTransform(AnimPhysSimSpaceType SimSpace, FComponentSpacePoseContext& Output) const;

	// Given a transform in component space, convert it to the current sim space
 // 给定组件空间中的变换，将其转换为当前的模拟空间
	ANIMGRAPHRUNTIME_API FTransform GetSimSpaceTransformFromComponentSpace(AnimPhysSimSpaceType SimSpace, FComponentSpacePoseContext& Output, const FTransform& InComponentTransform) const;

	// Given a world-space vector, convert it into the current simulation space
 // 给定一个世界空间向量，将其转换为当前的模拟空间
	ANIMGRAPHRUNTIME_API FVector TransformWorldVectorToSimSpace(FComponentSpacePoseContext& Output, const FVector& InVec) const;

	ANIMGRAPHRUNTIME_API FVector TransformWorldVectorToSimSpaceScaled(FComponentSpacePoseContext& Output, const FVector& InVec) const;

	ANIMGRAPHRUNTIME_API void ConvertSimulationSpace(FComponentSpacePoseContext& Output, AnimPhysSimSpaceType From, AnimPhysSimSpaceType To) const;

	void CalculateSimulationSpaceMotion(AnimPhysSimSpaceType Space, FAnimPhysSimSpaceSettings Settings, FComponentSpacePoseContext& Output, const float Dt,
		FVector& SpaceAngularVel, FVector& SpaceAngularAcc);

	// Maximum time to consider when accumulating time debt to avoid spiraling
 // 累积时间债时要考虑的最长时间以避免螺旋式上升
	static ANIMGRAPHRUNTIME_API const float MaxTimeDebt;

	// Cached timestep from the update phase (needed in evaluate phase)
 // 更新阶段的缓存时间步长（评估阶段需要）
	float NextTimeStep;

	// Current amount of time debt
 // 当前时间债务金额
	float TimeDebt;

	// Cached physics settings. We cache these on initialise to avoid the cost of accessing UPhysicsSettings a lot each frame
 // 缓存物理设置。我们在初始化时缓存这些内容，以避免每帧大量访问 UPhysicsSettings 的成本
	float AnimPhysicsMinDeltaTime;
	float MaxPhysicsDeltaTime;
	float MaxSubstepDeltaTime;
	int32 MaxSubsteps;
	//////////////////////////////////////////////////////////////////////////

	// Active body list
 // 活跃身体清单
	TArray<FAnimPhysLinkedBody> Bodies;

	// Pointers to bodies that need to be reset to their bound bone.
 // 指向需要重置为其绑定骨骼的实体的指针。
	// This happens on LOD change so we don't make the simulation unstable
 // 这种情况发生在 LOD 更改时，因此我们不会使模拟变得不稳定
	TArray<FAnimPhysLinkedBody*> BodiesToReset;

	// Pointers back to the base bodies to pass to the simulation
 // 返回基体的指针以传递给模拟
	TArray<FAnimPhysRigidBody*> BaseBodyPtrs;

	// List of current linear limits built for the current frame
 // 为当前帧构建的当前线性限制列表
	TArray<FAnimPhysLinearLimit> LinearLimits;

	// List of current angular limits built for the current frame
 // 为当前帧构建的当前角度限制列表
	TArray<FAnimPhysAngularLimit> AngularLimits;

	// List of spring force generators created for this frame
 // 为此框架创建的弹簧力生成器列表
	TArray<FAnimPhysSpring> Springs;

	// Position of the physics object relative to the transform if its bound bone.
 // 物理对象相对于其绑定骨骼的变换的位置。
	TArray<FVector> PhysicsBodyJointOffsets;

	// A pair of positions (relative to their associated physics bodies) for each pair of bodies in a chain. These positions should be driven to match each other in sim space by the physics contstraints - See UpdateLimits() fns.
 // 链中每对物体的一对位置（相对于其关联的物理物体）。应通过物理约束驱动这些位置在模拟空间中相互匹配 - 请参阅 UpdateLimits() fns。
	TArray<FAnimConstraintOffsetPair> ConstraintOffsets;
	
	// Depending on the LOD we might not be running all of the bound bodies (for chains)
 // 根据 LOD，我们可能不会运行所有绑定体（对于链）
	// this tracks the active bodies.
 // 这会跟踪活动的物体。
	TArray<int32> ActiveBoneIndices;

	// Gravity direction in sim space
 // 模拟空间中的重力方向
	FVector SimSpaceGravityDirection;

	// Previous linear velocity to resolve world accelerations when not using world space simulation
 // 以前的线速度在不使用世界空间模拟时解析世界加速度
	FVector PreviousComponentLinearVelocity;

	// Previous angular velocity to resolve world angular acceleration when not using world space simulation
 // 不使用世界空间模拟时用于解析世界角加速度的先前角速度
	FVector PreviousSimSpaceAngularVelocity;

	//////////////////////////////////////////////////////////////////////////
	// Live debug
 // 实时调试
	//////////////////////////////////////////////////////////////////////////
#if ENABLE_ANIM_DRAW_DEBUG
	ANIMGRAPHRUNTIME_API void DrawBodies(FComponentSpacePoseContext& InContext, const TArray<FAnimPhysRigidBody*>& InBodies);

	int32 FilteredBoneIndex;
#endif
public: 
	static ANIMGRAPHRUNTIME_API bool IsAnimDynamicsSystemEnabledFor(int32 InLOD);
};
