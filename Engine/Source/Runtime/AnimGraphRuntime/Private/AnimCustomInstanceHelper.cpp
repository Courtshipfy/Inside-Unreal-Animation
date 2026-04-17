// Copyright Epic Games, Inc. All Rights Reserved.


#include "AnimCustomInstanceHelper.h"
#include "Animation/Skeleton.h"
#include "Engine/SkeletalMesh.h"

/////////////////////////////////////////////////////
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
// FAnimCustomInstanceHelper
/////////////////////////////////////////////////////


bool FAnimCustomInstanceHelper::ShouldCreateCustomInstancePlayer(const USkeletalMeshComponent* SkeletalMeshComponent)
{
	const USkeletalMesh* SkeletalMesh = SkeletalMeshComponent->GetSkeletalMeshAsset();
	const USkeleton*     Skeleton     = SkeletalMesh ? SkeletalMesh->GetSkeleton() : nullptr;

	// create proper anim instance to animate
 // 创建适当的动画实例来制作动画
	UAnimInstance* AnimInstance = SkeletalMeshComponent->GetAnimInstance();

	return (AnimInstance == nullptr || SkeletalMeshComponent->GetAnimationMode() != EAnimationMode::AnimationBlueprint ||
		AnimInstance->GetClass() != SkeletalMeshComponent->AnimClass || !Skeleton || !AnimInstance->CurrentSkeleton);
}