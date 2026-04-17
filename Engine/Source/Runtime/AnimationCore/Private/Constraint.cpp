// Copyright Epic Games, Inc. All Rights Reserved.

/*=============================================================================
	Constraint.cpp: Constraint implementation
=============================================================================*/

#include "Constraint.h"
#include "AnimationCoreLibrary.h"
#include "AnimationCoreUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(Constraint)

void FConstraintOffset::ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource) const
{
	// in this matter, parent is accumulated first, and then individual component gets applied
 // 在这种情况下，首先累积父级，然后应用单个组件
	// I think that will be more consistent than going the other way
 // 我认为这比采取其他方式更加一致
	// this parent is confusing, rename?
 // 这位家长很困惑，重命名吗？
	OutSource = Parent.GetRelativeTransformReverse(InTarget);

	if (Translation != FVector::ZeroVector)
	{
		OutSource.AddToTranslation(Translation);
	}

	if (Rotation != FQuat::Identity)
	{
		OutSource.SetRotation(OutSource.GetRotation() * Rotation);
	}

	// I know I'm doing just != , not nearly
 // 我知道我只是在做 != ，而不是几乎
	if (Scale != FVector::OneVector)
	{
		OutSource.SetScale3D(OutSource.GetScale3D() * Scale);
	}
}

void FConstraintOffset::SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FConstraintDescription& Operator)
{
	Reset();

	// override previous value, this is rule
 // 覆盖以前的值，这是规则
	if (Operator.bParent)
	{
		Parent = Target.GetRelativeTransform(Source);
	}
	else
	{
		if (Operator.bTranslation)
		{
			Translation = Source.GetTranslation() - Target.GetTranslation();
		}

		if (Operator.bRotation)
		{
			Rotation = Source.GetRotation() * Target.GetRotation().Inverse();
		}

		if (Operator.bScale)
		{
			FVector RecipTarget = FTransform::GetSafeScaleReciprocal(Target.GetScale3D());
			Scale = Source.GetScale3D() * RecipTarget;
		}
	}
}

///////////////////////////////////////////////////////////////////
// new constraint change
// 新的约束变化

void FConstraintData::ApplyInverseOffset(const FTransform& InTarget, FTransform& OutSource, const FTransform& InBaseTransform) const
{
	if (bMaintainOffset)
	{
		//The offset is saved based on 
  // 偏移量的保存基于
		// (Source - Target) - BaseTransform  (SaveInverseOffset)
  // （源 - 目标） - BaseTransform (SaveInverseOffset)
		// note that all of them is in component space
  // 请注意，它们都在组件空间中
		// and also depending on rotation or translation or scale, how the inverse is calculated is different
  // 并且根据旋转、平移或缩放，计算倒数的方式也不同
		// This will get applied to 
  // 这将被应用到
		// Offset + [NewBaseTransform] + [NewTargetTransform] = [New SourceTransform] (ApplyInverseOffset)
  // 偏移量 + [NewBaseTransform] + [NewTargetTransform] = [New SourceTransform] (ApplyInverseOffset)
		if (Constraint.DoesAffectTransform())
		{
			OutSource = (Offset * InBaseTransform) * InTarget;
		}
		else
		{
			if (Constraint.DoesAffectTranslation())
			{
				OutSource.SetTranslation(InTarget.GetTranslation() + InBaseTransform.TransformVectorNoScale(Offset.GetTranslation()));
			}

			if (Constraint.DoesAffectRotation())
			{
				OutSource.SetRotation(InTarget.GetRotation() * InBaseTransform.GetRotation() * Offset.GetRotation());
				OutSource.NormalizeRotation();
			}

			if (Constraint.DoesAffectScale())
			{
				OutSource.SetScale3D(InTarget.GetScale3D() * InBaseTransform.GetScale3D() * Offset.GetScale3D());
			}
		}
	}
	else
	{
		OutSource = InTarget;
	}
}

void FConstraintData::SaveInverseOffset(const FTransform& Source, const FTransform& Target, const FTransform& InBaseTransform)
{
	ResetOffset();

	if (bMaintainOffset)
	{
		//The offset is saved based on 
  // 偏移量的保存基于
		// (Source - Target) - BaseTransform  (SaveInverseOffset)
  // （源 - 目标） - BaseTransform (SaveInverseOffset)
		// note that all of them is in component space
  // 请注意，它们都在组件空间中
		// and also depending on rotation or translation or scale, how the inverse is calculated is different
  // 并且根据旋转、平移或缩放，计算倒数的方式也不同
		// This will get applied to 
  // 这将被应用到
		// Offset + [NewBaseTransform] + [NewTargetTransform] = [New SourceTransform] (ApplyInverseOffset)
  // 偏移量 + [NewBaseTransform] + [NewTargetTransform] = [New SourceTransform] (ApplyInverseOffset)
		if (Constraint.DoesAffectTransform())
		{
			FTransform ToSource = Source.GetRelativeTransform(Target);
			Offset = ToSource.GetRelativeTransform(InBaseTransform);
		}
		else
		{
			if (Constraint.DoesAffectTranslation())
			{
				FVector DeltaLocation = Source.GetTranslation() - Target.GetTranslation();
				Offset.SetLocation(InBaseTransform.InverseTransformVectorNoScale(DeltaLocation));
			}

			if (Constraint.DoesAffectRotation())
			{
				// this is same as local target's inverse * local source
    // 这与本地目标的逆 * 本地源相同
				// (target.Inverse() * base) * (source.Inverse() * base).inverse()
    // (目标.Inverse() * 基数) * (源.Inverse() * 基数).inverse()
				// = (target.Inverse() * base * base.Inverse() * source
    // = (目标.Inverse() * 基数 * 基数.Inverse() * 源
				// = target.Inverse() * source
    // = 目标.Inverse() * 源
				FQuat DeltaRotation = Target.GetRotation().Inverse() * Source.GetRotation();
				Offset.SetRotation(InBaseTransform.GetRotation().Inverse() * DeltaRotation);
				Offset.NormalizeRotation();
			}

			if (Constraint.DoesAffectScale())
			{
				FVector RecipTarget = FTransform::GetSafeScaleReciprocal(Target.GetScale3D());
				FVector DeltaScale = Source.GetScale3D() * RecipTarget;
				FVector RecipBase = FTransform::GetSafeScaleReciprocal(InBaseTransform.GetScale3D());
				Offset.SetScale3D(DeltaScale * RecipBase);
			}
		}
	}
}

void FConstraintData::ApplyConstraintTransform(const FTransform& TargetTransform, const FTransform& InCurrentTransform, const FTransform& CurrentParentTransform, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const
{
	FTransform OffsetTargetTransform;

	// now apply inverse on the target since that's what we're applying
 // 现在对目标应用逆，因为这就是我们正在应用的
	ApplyInverseOffset(TargetTransform, OffsetTargetTransform, CurrentParentTransform);

	// give the offset target transform
 // 给出偏移目标变换
	Constraint.ApplyConstraintTransform(OffsetTargetTransform, InCurrentTransform, CurrentParentTransform, Weight, BlendHelperInLocalSpace);
}

void FTransformConstraintDescription::AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform, const FTransform& CurrentParentTransform, float Weight, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const
{
	FTransform TargetLocalTransform = TargetTransform.GetRelativeTransform(CurrentParentTransform);

	if (DoesAffectTransform())
	{
		BlendHelperInLocalSpace.AddParent(TargetLocalTransform, Weight);
	}
	else
	{
		if (DoesAffectTranslation())
		{
			FVector Translation = TargetLocalTransform.GetTranslation();
			AxesFilterOption.FilterVector(Translation);
			BlendHelperInLocalSpace.AddTranslation(Translation, Weight);
		}

		if (DoesAffectRotation())
		{
			FQuat DeltaRotation = TargetLocalTransform.GetRotation();
			AxesFilterOption.FilterQuat(DeltaRotation);
			BlendHelperInLocalSpace.AddRotation(DeltaRotation, Weight);
		}

		if (DoesAffectScale())
		{
			FVector Scale = TargetLocalTransform.GetScale3D();
			AxesFilterOption.FilterVector(Scale);
			BlendHelperInLocalSpace.AddScale(Scale, Weight);
		}
	}
}

void FAimConstraintDescription::AccumulateConstraintTransform(const FTransform& TargetTransform, const FTransform& CurrentTransform, const FTransform& CurrentParentTransform, float Weight, FMultiTransformBlendHelper& BlendHelperInLocalSpace) const
{
	// need current transform - I need global transform of Target, I think incoming is local space
 // 需要当前变换 - 我需要目标的全局变换，我认为传入的是本地空间
	FTransform NewTransform = CurrentTransform;

	if (bUseLookUp)
	{
		FQuat DeltaRotation = AnimationCore::SolveAim(NewTransform, LookUpTarget, LookUp_Axis.GetTransformedAxis(NewTransform), false, FVector::ZeroVector);
		NewTransform.SetRotation(DeltaRotation * NewTransform.GetRotation());
	}

	FQuat DeltaRotation = AnimationCore::SolveAim(NewTransform, TargetTransform.GetLocation(), LookAt_Axis.GetTransformedAxis(NewTransform), false, FVector::ZeroVector);
	NewTransform.SetRotation(DeltaRotation * NewTransform.GetRotation());

	FTransform LocalTransform = NewTransform.GetRelativeTransform(CurrentParentTransform);
	FQuat LocalRotation = LocalTransform.GetRotation();
	AxesFilterOption.FilterQuat(LocalRotation);
	BlendHelperInLocalSpace.AddRotation(LocalRotation, Weight);
}
