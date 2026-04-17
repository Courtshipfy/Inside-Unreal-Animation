// Copyright Epic Games, Inc. All Rights Reserved.

#include "BoneControllers/AnimNode_SpringBone.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimStats.h"
#include "Animation/AnimTrace.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNode_SpringBone)

/////////////////////////////////////////////////////
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone
// FAnimNode_SpringBone

FAnimNode_SpringBone::FAnimNode_SpringBone()
	: MaxDisplacement(0.0)
	, SpringStiffness(50.0)
	, SpringDamping(4.0)
	, ErrorResetThresh(256.0)
	, BoneLocation(FVector::ZeroVector)
	, BoneVelocity(FVector::ZeroVector)
	, OwnerVelocity(FVector::ZeroVector)
	, RemainingTime(0.f)
#if WITH_EDITORONLY_DATA
	, bNoZSpring_DEPRECATED(false)
#endif
	, bLimitDisplacement(false)
	, bTranslateX(true)
	, bTranslateY(true)
	, bTranslateZ(true)
	, bRotateX(false)
	, bRotateY(false)
	, bRotateZ(false)
	, bHadValidStrength(false)
	, bOverrideOwnerVelocity(false)
	, OwnerVelocityOverride(FVector::ZeroVector)
{
}

void FAnimNode_SpringBone::Initialize_AnyThread(const FAnimationInitializeContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(Initialize_AnyThread)
	FAnimNode_SkeletalControlBase::Initialize_AnyThread(Context);

	RemainingTime = 0.0f;
}

void FAnimNode_SpringBone::CacheBones_AnyThread(const FAnimationCacheBonesContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(CacheBones_AnyThread)
	FAnimNode_SkeletalControlBase::CacheBones_AnyThread(Context);
}

void FAnimNode_SpringBone::UpdateInternal(const FAnimationUpdateContext& Context)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(UpdateInternal)
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);

	RemainingTime += Context.GetDeltaTime();

	// Fixed step simulation at 120hz
 // 120hz 下的固定步进模拟
	FixedTimeStep = (1.f / 120.f) * TimeDilation;
}

void FAnimNode_SpringBone::GatherDebugData(FNodeDebugData& DebugData)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(GatherDebugData)
	const float ActualBiasedAlpha = AlphaScaleBias.ApplyTo(Alpha);

	//MDW_TODO Add more output info?
 // MDW_TODO 添加更多输出信息？
	FString DebugLine = DebugData.GetNodeName(this);
	DebugLine += FString::Printf(TEXT("(Alpha: %.1f%% RemainingTime: %.3f)"), ActualBiasedAlpha*100.f, RemainingTime);

	DebugData.AddDebugItem(DebugLine);
	ComponentPose.GatherDebugData(DebugData);
}

FORCEINLINE void CopyToVectorByFlags(FVector& DestVec, const FVector& SrcVec, bool bX, bool bY, bool bZ)
{
	if (bX)
	{
		DestVec.X = SrcVec.X;
	}
	if (bY)
	{
		DestVec.Y = SrcVec.Y;
	}
	if (bZ)
	{
		DestVec.Z = SrcVec.Z;
	}
}

void FAnimNode_SpringBone::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output, TArray<FBoneTransform>& OutBoneTransforms)
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(EvaluateSkeletalControl_AnyThread)
	ANIM_MT_SCOPE_CYCLE_COUNTER_VERBOSE(SpringBone, !IsInGameThread());

	check(OutBoneTransforms.Num() == 0);

	const bool bNoOffset = !bTranslateX && !bTranslateY && !bTranslateZ;
	if (bNoOffset)
	{
		return;
	}

	// Location of our bone in world space
 // 我们的骨头在世界空间中的位置
	const FBoneContainer& BoneContainer = Output.Pose.GetPose().GetBoneContainer();

	const FCompactPoseBoneIndex SpringBoneIndex = SpringBone.GetCompactPoseIndex(BoneContainer);
	const FTransform& SpaceBase = Output.Pose.GetComponentSpaceTransform(SpringBoneIndex);
	FTransform BoneTransformInWorldSpace = SpaceBase * Output.AnimInstanceProxy->GetComponentTransform();

	FVector const TargetPos = BoneTransformInWorldSpace.GetLocation();

	// Init values first time
 // 第一次初始化值
	if (RemainingTime == 0.0f)
	{
		BoneLocation = TargetPos;
		BoneVelocity = FVector::ZeroVector;
	}

	if(!FMath::IsNearlyZero(FixedTimeStep, KINDA_SMALL_NUMBER))
	{
		while (RemainingTime > FixedTimeStep)
		{
			// Update location of our base by how much our base moved this frame.
   // 通过我们的基地移动此框架的距离来更新我们基地的位置。
			FVector const BaseTranslation = (OwnerVelocity * FixedTimeStep);
			BoneLocation += BaseTranslation;

			// Reinit values if outside reset threshold
   // 如果超出重置阈值，则重新初始化值
			if (((TargetPos - BoneLocation).SizeSquared() > (ErrorResetThresh*ErrorResetThresh)))
			{
				BoneLocation = TargetPos;
				BoneVelocity = FVector::ZeroVector;
			}

			// Calculate error vector.
   // 计算误差向量。
			FVector const Error = (TargetPos - BoneLocation);
			FVector const DampingForce = SpringDamping * BoneVelocity;
			FVector const SpringForce = SpringStiffness * Error;

			// Calculate force based on error and vel
   // 根据误差和速度计算力
			FVector const Acceleration = SpringForce - DampingForce;

			// Integrate velocity
   // 积分速度
			// Make sure damping with variable frame rate actually dampens velocity. Otherwise Spring will go nuts.
   // 确保可变帧速率的阻尼实际上会抑制速度。否则Spring会发疯的。
			double const CutOffDampingValue = 1.0 / FixedTimeStep;
			if (SpringDamping > CutOffDampingValue)
			{
				double const SafetyScale = CutOffDampingValue / SpringDamping;
				BoneVelocity += SafetyScale * (Acceleration * FixedTimeStep);
			}
			else
			{
				BoneVelocity += (Acceleration * FixedTimeStep);
			}

			// Clamp velocity to something sane (|dX/dt| <= ErrorResetThresh)
   // 将速度限制为正常值 (|dX/dt| <= ErrorResetThresh)
			double const BoneVelocityMagnitude = BoneVelocity.Size();
			if (BoneVelocityMagnitude * FixedTimeStep > ErrorResetThresh)
			{
				BoneVelocity *= (ErrorResetThresh / (BoneVelocityMagnitude * FixedTimeStep));
			}

			// Integrate position
   // 整合位置
			FVector const OldBoneLocation = BoneLocation;
			FVector const DeltaMove = (BoneVelocity * FixedTimeStep);
			BoneLocation += DeltaMove;

			// Filter out spring translation based on our filter properties
   // 根据我们的过滤器属性过滤掉 spring 翻译
			CopyToVectorByFlags(BoneLocation, TargetPos, !bTranslateX, !bTranslateY, !bTranslateZ);


			// If desired, limit error
   // 如果需要，限制误差
			if (bLimitDisplacement)
			{
				FVector CurrentDisp = BoneLocation - TargetPos;
				// Too far away - project back onto sphere around target.
    // 距离太远 - 投影回到目标周围的球体上。
				if (CurrentDisp.SizeSquared() > FMath::Square(MaxDisplacement))
				{
					FVector DispDir = CurrentDisp.GetSafeNormal();
					BoneLocation = TargetPos + (MaxDisplacement * DispDir);
				}
			}

			// Update velocity to reflect post processing done to bone location.
   // 更新速度以反映对骨骼位置进行的后处理。
			BoneVelocity = (BoneLocation - OldBoneLocation) / FixedTimeStep;

			check(!BoneLocation.ContainsNaN());
			check(!BoneVelocity.ContainsNaN());

			RemainingTime -= FixedTimeStep;
		}
		LocalBoneTransform = Output.AnimInstanceProxy->GetComponentTransform().InverseTransformPosition(BoneLocation);
	}
	else
	{
		BoneLocation = Output.AnimInstanceProxy->GetComponentTransform().TransformPosition(LocalBoneTransform);
	}
	// Now convert back into component space and output - rotation is unchanged.
 // 现在转换回组件空间并输出 - 旋转不变。
	FTransform OutBoneTM = SpaceBase;
	OutBoneTM.SetLocation(LocalBoneTransform);

	const bool bUseRotation = bRotateX || bRotateY || bRotateZ;
	if (bUseRotation)
	{
		FCompactPoseBoneIndex ParentBoneIndex = Output.Pose.GetPose().GetParentBoneIndex(SpringBoneIndex);
		const FTransform& ParentSpaceBase = Output.Pose.GetComponentSpaceTransform(ParentBoneIndex);

		FVector ParentToTarget = (TargetPos - ParentSpaceBase.GetLocation()).GetSafeNormal();
		FVector ParentToCurrent = (BoneLocation - ParentSpaceBase.GetLocation()).GetSafeNormal();

		FQuat AdditionalRotation = FQuat::FindBetweenNormals(ParentToTarget, ParentToCurrent);

		// Filter rotation based on our filter properties
  // 根据我们的过滤器属性进行过滤器旋转
		FVector EularRot = AdditionalRotation.Euler();
		CopyToVectorByFlags(EularRot, FVector::ZeroVector, !bRotateX, !bRotateY, !bRotateZ);

		OutBoneTM.SetRotation(FQuat::MakeFromEuler(EularRot) * OutBoneTM.GetRotation());
	}

	// Output new transform for current bone.
 // 输出当前骨骼的新变换。
	OutBoneTransforms.Add(FBoneTransform(SpringBoneIndex, OutBoneTM));

	TRACE_ANIM_NODE_VALUE(Output, TEXT("Remaining Time"), RemainingTime);
}


bool FAnimNode_SpringBone::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) 
{
	return (SpringBone.IsValidToEvaluate(RequiredBones));
}

void FAnimNode_SpringBone::InitializeBoneReferences(const FBoneContainer& RequiredBones) 
{
	DECLARE_SCOPE_HIERARCHICAL_COUNTER_ANIMNODE(InitializeBoneReferences)
	SpringBone.Initialize(RequiredBones);
}

void FAnimNode_SpringBone::PreUpdate(const UAnimInstance* InAnimInstance)
{
	if (const USkeletalMeshComponent* SkelComp = InAnimInstance->GetSkelMeshComponent())
	{
		if (const UWorld* World = SkelComp->GetWorld())
		{
			check(World->GetWorldSettings());
			TimeDilation = World->GetWorldSettings()->GetEffectiveTimeDilation();

			if (bOverrideOwnerVelocity)
			{
				OwnerVelocity = OwnerVelocityOverride;
			}
			else
			{
				AActor* SkelOwner = SkelComp->GetOwner();
				if (SkelComp->GetAttachParent() != nullptr && (SkelOwner == nullptr))
				{
					SkelOwner = SkelComp->GetAttachParent()->GetOwner();
					OwnerVelocity = SkelOwner->GetVelocity();
				}
				else
				{
					OwnerVelocity = FVector::ZeroVector;
				}
			}

		}
	}
}

