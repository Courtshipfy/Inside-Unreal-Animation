// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Curves/CurveFloat.h"
#include "CommonAnimationTypes.generated.h"

/**
 *	An easing type defining how to ease float values.
 */
UENUM()
enum class EEasingFuncType : uint8
{
	// Linear easing (no change to the value)
	// 线性缓动（不改变值）
	Linear,
	// Easing using a sinus function
	// 使用正弦函数进行缓动
	Sinusoidal,
	// Cubic version of the value (only in)
	// 值的立方版本（仅在）
	Cubic,
	// Quadratic version of the value (in and out)
	// 值的二次版本（输入和输出）
	QuadraticInOut,
	// Cubic version of the value (in and out)
	// 值的立方版本（输入和输出）
	CubicInOut,
	// Easing using a cubic hermite function
	// 使用三次 Hermite 函数进行缓动
	HermiteCubic,
	// Quartic version of the value (in and out)
	// 值的四次版本（输入和输出）
	QuarticInOut,
	// Quintic version of the value (in and out)
	// 值的五次版本（输入和输出）
	QuinticInOut,
	// Circular easing (only in)
	// 循环缓动（仅在）
	CircularIn,
	// Circular easing (only out)
	// 循环缓动（仅出）
	CircularOut,
	// Circular easing (in and out)
	// 循环缓动（进出）
	CircularInOut,
	// Exponential easing (only in)
	// 指数缓动（仅在）
	ExpIn,
	// Exponential easing (only out)
	// 指数缓动（仅输出）
	ExpOut,
	// Exponential easing (in and out)
	// 指数缓动（输入和输出）
	ExpInOut,
	// Custom - based on an optional Curve
	// 自定义 - 基于可选曲线
	CustomCurve
};

// A rotational component. This is used for retargeting, for example.
// 旋转组件。例如，这用于重定向。
UENUM()
enum class ERotationComponent : uint8
{
	// Using the X component of the Euler rotation
	// 使用欧拉旋转的 X 分量
	EulerX,
	// Using the Y component of the Euler rotation
	// 使用欧拉旋转的 Y 分量
	EulerY,
	// Using the Z component of the Euler rotation
	// 使用欧拉旋转的 Z 分量
	EulerZ,
	// Using the angle of the quaternion
	// 使用四元数的角度
	QuaternionAngle,
	// Using the angle of the swing quaternion
	// 使用摆动四元数的角度
	SwingAngle,
	// Using the angle of the twist quaternion
	// 使用扭曲四元数的角度
	TwistAngle
};

/**
 *	The FRotationRetargetingInfo is used to provide all of the 
 *	settings required to perform rotational retargeting on a single
 *	transform.
 */
USTRUCT(BlueprintType)
struct FRotationRetargetingInfo
{
	GENERATED_BODY()

public:

	/** Default constructor */
	/** 默认构造函数 */
	FRotationRetargetingInfo(bool bInEnabled = true)
		: bEnabled(bInEnabled)
		, Source(FTransform::Identity)
		, Target(FTransform::Identity)
		, RotationComponent(ERotationComponent::SwingAngle)
		, TwistAxis(FVector(1.f, 0.f, 0.f))
		, bUseAbsoluteAngle(false)
		, SourceMinimum(0.f)
		, SourceMaximum(45.f)
		, TargetMinimum(0.f)
		, TargetMaximum(45.f)
		, EasingType(EEasingFuncType::Linear)
		, CustomCurve(FRuntimeFloatCurve())
		, bFlipEasing(false)
		, EasingWeight(1.f)
		, bClamp(false)
	{
	}

	// Set to true this enables retargeting
	// 设置为 true 可以启用重定向
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	bool bEnabled;

	// The source transform of the frame of reference. The rotation is made relative to this space
	// 参考系的源变换。旋转是相对于该空间进行的
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	FTransform Source;

	// The target transform to project the rotation. In most cases this is the same as Source
	// 目标变换以投影旋转。在大多数情况下，这与源相同
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	FTransform Target;

	// The rotation component to perform retargeting with
	// 用于执行重定向的旋转组件
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	ERotationComponent RotationComponent;

	// In case the rotation component is SwingAngle or TwistAngle this vector is used as the twist axis
	// 如果旋转分量是 SwingAngle 或 TwistAngle，则该向量用作扭转轴
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	FVector TwistAxis;

	// If set to true the angle will be always positive, thus resulting in mirrored rotation both ways
	// 如果设置为 true，角度将始终为正，从而导致双向镜像旋转
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	bool bUseAbsoluteAngle;

	// The minimum value of the source angle in degrees
	// 源角度的最小值（以度为单位）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo", meta = (UIMin = "-90.0", UIMax = "90.0"))
	float SourceMinimum;

	// The maximum value of the source angle in degrees
	// 源角度的最大值（以度为单位）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo", meta = (UIMin = "-90.0", UIMax = "90.0"))
	float SourceMaximum;

	// The minimum value of the target angle in degrees (can be the same as SourceMinimum)
	// 目标角度的最小值（可以与 SourceMinimum 相同）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo", meta = (UIMin = "-90.0", UIMax = "90.0"))
	float TargetMinimum;

	// The target value of the target angle in degrees (can be the same as SourceMaximum)
	// 目标角度的目标值，以度为单位（可以与 SourceMaximum 相同）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo", meta = (UIMin = "-90.0", UIMax = "90.0"))
	float TargetMaximum;

	// The easing to use - pick linear if you don't want to apply any easing
	// 使用缓动 - 如果您不想应用任何缓动，请选择线性
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	EEasingFuncType EasingType;

	/** Custom curve mapping to apply if bApplyCustomCurve is true */
	/** 如果 bApplyCustomCurve 为 true，则应用自定义曲线映射 */
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	FRuntimeFloatCurve CustomCurve;

	// If set to true the interpolation value for the easing will be flipped (1.0 - Value)
	// 如果设置为 true，缓动的插值将被翻转（1.0 - 值）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	bool bFlipEasing;

	// The amount of easing to apply (value should be 0.0 to 1.0)
	// 应用的缓动量（值应为 0.0 到 1.0）
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo", meta = (UIMin = "0.0", UIMax = "1.0"))
	float EasingWeight;
	
	// If set to true the value for the easing will be clamped between 0.0 and 1.0
	// 如果设置为 true，缓动值将被限制在 0.0 和 1.0 之间
	UPROPERTY(EditAnywhere, Category = "FRotationRetargetingInfo")
	bool bClamp;
};
