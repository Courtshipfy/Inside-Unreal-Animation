# Unreal Engine 动画蒙太奇 (Anim Montage) 深度解析

本笔记用于记录虚幻引擎蒙太奇系统的运行时架构、实例管理及混合逻辑。

---

## 核心概念：资源与实例的区分

在源码中，蒙太奇分为两个层面：
- **UAnimMontage (Asset)**: 静态资源。存储在磁盘上，包含 Section、Slot、Notifies 等配置信息。
- **FAnimMontageInstance (Instance)**: 运行时实例。当调用 `PlayMontage` 时在内存中动态创建，记录当前的播放位置、速率、权重和 Section 状态。

---

## 运行时管理：`TArray<FAnimMontageInstance*> MontageInstances`

在 `UAnimInstance` 中，所有正在运行或正在淡出的蒙太奇都存储在这个数组中。

### 1. 为什么使用数组 (TArray)？
虽然逻辑上我们可能只关注一个“当前动作”，但由于**动画过渡 (Blending)** 的存在：
- 当新蒙太奇开始播放（Blend In）时，旧蒙太奇不会立即消失，而是进入淡出状态（Blend Out）。
- 此时数组中会同时存在：**1个主导蒙太奇 + N个正在淡出的旧蒙太奇**。

### 2. 蒙太奇分组 (Montage Groups) 与互斥规则
注释提到：*"only one is primarily active per group"*。
- **分组 (Group)**: 在蒙太奇编辑器中定义。
- **组内互斥**: 同一个 Group 内，同一时刻只能有一个“活跃”的蒙太奇。播放新的同组蒙太奇会导致旧的立刻开始 Blend Out。
- **跨组并行**: 不同 Group 的蒙太奇（如：UpperBody 和 LowerBody）可以完全独立并行播放，它们都会存在于这个数组中。

### 3. 生命周期与内存管理
- **创建**: 通过 `AnimInstance->PlayMontage()` 触发。
- **更新**: 在 `UAnimInstance::UpdateAnimation` 中遍历数组，每帧推动每个 Instance 的进度。
- **销毁**: 
    - 当 Instance 的播放权重降至 0（完成淡出）。
    - 当蒙太奇自然播放结束。
    - 引擎会自动将其从数组中移除并释放内存。

---
