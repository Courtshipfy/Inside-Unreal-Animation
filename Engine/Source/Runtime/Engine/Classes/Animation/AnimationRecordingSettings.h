// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "Animation/AnimTypes.h"
#include "Curves/RichCurve.h"
#include "Misc/FrameRate.h"
#include "AnimationRecordingSettings.generated.h"

/** Settings describing how to record an animation */
/** 描述如何录制动画的设置 */
USTRUCT()
struct FAnimationRecordingSettings
{
	GENERATED_BODY()

	/** 30Hz default sample frame rate */
	/** 30Hz 默认采样帧率 */
	static ENGINE_API const FFrameRate DefaultSampleFrameRate;

	/** 1 minute default length */
	/** 默认时长 1 分钟 */
	static ENGINE_API const float DefaultMaximumLength;

	/** Length used to specify unbounded */
	/** 用于指定无界的长度 */
	static ENGINE_API const float UnboundedMaximumLength;

	FAnimationRecordingSettings()
		: bRecordInWorldSpace(true)
		, bRemoveRootAnimation(true)
		, bAutoSaveAsset(false)
		, SampleFrameRate(DefaultSampleFrameRate)
		, Length((float)DefaultMaximumLength)
		, Interpolation(EAnimInterpolationType::Linear)
		, InterpMode(ERichCurveInterpMode::RCIM_Linear)
		, TangentMode(ERichCurveTangentMode::RCTM_Auto)
		, bCheckDeltaTimeAtBeginning(true)
		, bRecordTransforms(true)
		, bRecordMorphTargets(true)
		, bRecordAttributeCurves(true)
		, bRecordMaterialCurves(true)
		, bTransactRecording(true) 
	{}
	
	/** Whether to record animation in world space, defaults to true */
	/** 是否在世界空间录制动画，默认true */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRecordInWorldSpace;

	/** Whether to remove the root bone transform from the animation */
	/** 是否从动画中移除根骨骼变换 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRemoveRootAnimation;

	/** Whether to auto-save asset when recording is completed. Defaults to false */
	/** 录制完成后是否自动保存资源。默认为 false */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bAutoSaveAsset;

	/** Sample rate of the recorded animation */
	/** 录制动画的采样率 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	FFrameRate SampleFrameRate;

	/** Maximum length of the animation recorded (in seconds). If zero the animation will keep on recording until stopped. */
	/** 录制的动画的最大长度（以秒为单位）。如果为零，动画将继续录制直到停止。 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	float Length;

	/** This defines how values between keys are calculated for transforms.**/
	/** 这定义了如何计算转换的键之间的值。**/
	UPROPERTY(EditAnywhere, Category = "Settings")
	EAnimInterpolationType Interpolation;

	/** Interpolation mode for the recorded keys. */
	/** 所记录关键点的插值模式。 */
	UPROPERTY(EditAnywhere, Category = "Settings", DisplayName = "Interpolation Mode")
	TEnumAsByte<ERichCurveInterpMode> InterpMode;

	/** Tangent mode for the recorded keys. */
	/** [翻译失败: Tangent mode for the recorded keys.] */
	UPROPERTY(EditAnywhere, Category = "Settings")
	TEnumAsByte<ERichCurveTangentMode> TangentMode;

	/** Whether to check DeltaTime at recording for pauses, turned off for TakeRecorder*/
	/** [翻译失败: Whether to check DeltaTime at recording for pauses, turned off for TakeRecorder]*/
	bool bCheckDeltaTimeAtBeginning;

	/** Whether or not to record transforms */
	/** 是否记录变换 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRecordTransforms;

	/** Whether or not to record morph targets */
	/** 是否记录变形目标 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRecordMorphTargets;

	/** Whether or not to record parameter curves */
	/** 是否记录参数曲线 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRecordAttributeCurves;

	/** Whether or not to record material curves */
	/** 是否记录材质曲线 */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bRecordMaterialCurves;

	/** Whether or not to transact recording changes */
	/** [翻译失败: Whether or not to transact recording changes] */
	UPROPERTY(EditAnywhere, Category = "Settings")
	bool bTransactRecording;

	/** Include only the animation bones/curves that match this list */
	/** [翻译失败: Include only the animation bones/curves that match this list] */
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FString> IncludeAnimationNames;

	/** Exclude all animation bones/curves that match this list */
	/** [翻译失败: Exclude all animation bones/curves that match this list] */
	UPROPERTY(EditAnywhere, Category = "Settings")
	TArray<FString> ExcludeAnimationNames;
};
