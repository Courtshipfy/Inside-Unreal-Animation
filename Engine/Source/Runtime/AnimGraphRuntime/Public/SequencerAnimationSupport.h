// Copyright Epic Games, Inc. All Rights Reserved.

/**
 * Sequencer Animation Track Support interface - this is required for animation track to work
 */

#pragma once
#include "CoreMinimal.h"
#include "HAL/Platform.h"
#include "UObject/Interface.h"
#include "UObject/Object.h"
#include "UObject/ObjectMacros.h"
#include "UObject/UObjectGlobals.h"

#include "SequencerAnimationSupport.generated.h"

class UAnimInstance;
class UAnimSequenceBase;
class UObject;

UINTERFACE(meta = (CannotImplementInterfaceInBlueprint), MinimalAPI)
class USequencerAnimationSupport : public UInterface
{
	GENERATED_UINTERFACE_BODY()
};

class ISequencerAnimationSupport
{
	GENERATED_IINTERFACE_BODY()

	/** Source Animation Getter for the support of the Sequencer Animation Track interface */
	/** Source Animation Getter，用于支持 Sequencer Animation Track 接口 */
	/** Source Animation Getter，用于支持 Sequencer Animation Track 接口 */
	/** Source Animation Getter，用于支持 Sequencer Animation Track 接口 */
	virtual UAnimInstance* GetSourceAnimInstance() = 0;
	virtual void SetSourceAnimInstance(UAnimInstance* SourceAnimInstance) = 0;
	/** 在此实例中更新动画序列播放器 */
	virtual bool DoesSupportDifferentSourceAnimInstance() const = 0;
	/** 在此实例中更新动画序列播放器 */
	/** Update an animation sequence player in this instance */
	/** 在此实例中更新动画序列播放器 */
	/** 构造该实例中的所有节点 */
	virtual void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InPosition, float Weight, bool bFireNotifies) = 0;
	virtual void UpdateAnimTrack(UAnimSequenceBase* InAnimSequence, int32 SequenceId, float InFromPosition, float InToPosition, float Weight, bool bFireNotifies) = 0;
	/** 构造该实例中的所有节点 */
	/** 重置此实例中的所有节点 */

	/** Construct all nodes in this instance */
	/** 构造该实例中的所有节点 */
	/** 重置此实例的姿势*/
	/** 重置此实例中的所有节点 */
	virtual void ConstructNodes() = 0;

	/** 保存命名姿势后恢复 */
	/** Reset all nodes in this instance */
	/** 重置此实例的姿势*/
	/** 重置此实例中的所有节点 */
	virtual void ResetNodes() = 0;

	/** 保存命名姿势后恢复 */
	/** Reset the pose for this instance*/
	/** 重置此实例的姿势*/
	virtual void ResetPose() = 0;

	/** Saved the named pose to restore after */
	/** 保存命名姿势后恢复 */
	virtual void SavePose() = 0;
};

