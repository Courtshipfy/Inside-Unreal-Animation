# Unreal Engine 动画系统源码学习清单

这份清单旨在帮助你系统性地理解虚幻引擎的动画系统。建议按照从核心框架到具体节点，再到高级特性的顺序进行阅读和源码追踪。

## 阶段一：核心架构与生命周期 (The Core Framework)
*目标：理解动画是如何被驱动的，以及动画数据在引擎中是如何流转的。*

- [ ] **`UAnimInstance` (动画实例) 的生命周期**
  - 源码位置: `Engine/Source/Runtime/Engine/Private/Animation/AnimInstance.cpp`
  - 重点函数: 
    - [x] `InitializeAnimation()` - 初始化入口（第 438 行）
    - [ ] `UpdateAnimation(float DeltaSeconds)` - 每帧更新入口（第 565 行）
    - [ ] `PostEvaluateAnimation()` - 求值后处理（第 927 行）
  - 核心概念: 它是连接 Gameplay 逻辑和动画蓝图逻辑的桥梁。理解 Event Graph 是如何更新属性的。
- [ ] **动画求值流程 (Evaluation Pipeline)**
  - 源码位置: `USkeletalMeshComponent::EvaluateAnimation` 以及 `UAnimInstance::EvaluateAnimation`
  - 重点分析: 引擎是在哪个线程（GameThread 还是 WorkerThread）执行动画计算的？了解 `bUseMultiThreadedAnimationUpdate` 的影响。
- [ ] **动画核心数据结构 (Animation Pose Data)**
  - 源码位置: `Engine/Source/Runtime/Engine/Public/Animation/AnimationPoseData.h`, `BonePose.h`
  - 核心概念: `FCompactPose`, `FBlendedCurve`, `FStackCustomAttributes`。理解局部空间 (Local Space) 和组件空间 (Component Space) 变换的区别。

## 阶段二：动画蓝图与节点图表 (AnimGraph & Nodes)
*目标：探究连连看的底层原理，理解节点是如何计算出最终位姿的。*

- [ ] **动画节点基类 (`FAnimNode_Base`)**
  - [ ] 源码位置: `Engine/Source/Runtime/Engine/Classes/Animation/AnimNodeBase.h`
  - [ ] 核心机制: 理解三大核心生命周期：`Initialize_AnyThread`, `Update_AnyThread`, `Evaluate_AnyThread`。
- [ ] **执行上下文 (`FAnimationInitializeContext`, `FAnimationUpdateContext`, `FAnimationPoseContext`)**
  - [ ] 分析这些上下文对象是如何在节点树（Node Tree）中自顶向下或自底向上级联传递的。
- [ ] **常用节点的源码剖析**
  - [ ] 播放节点: `FAnimNode_SequencePlayer` (看它是如何根据 DeltaTime 推进进度的)。
  - [ ] 混合节点: `FAnimNode_BlendListByBool`, `FAnimNode_LayeredBoneBlend` (看它是如何合并两个 `FCompactPose` 的)。
  - [ ] 状态机节点: `FAnimNode_StateMachine` (非常复杂但极其重要，研究状态切换、过渡规则和混合逻辑)。

## 阶段三：高级动画系统与底层骨骼数学
*目标：掌握更复杂的动画混合、空间转换和底层数学运算。*

- [ ] **空间转换节点**
  - [ ] `FAnimNode_ConvertLocalToComponentSpace` 和 `FAnimNode_ConvertComponentToLocalSpace`。
  - [ ] 思考：为什么 IK 运算通常要在 Component Space 下进行？
- [ ] **动画底层数学 (`AnimationCore`)**
  - [ ] 源码位置: `Engine/Source/Runtime/AnimationCore/`
  - [ ] 核心结构: `FTransform`, 四元数 (`FQuat`) 的球面线性插值 (Slerp)。
  - [ ] 基础 IK 算法: `FABRIK` 和 `CCDIK` 的底层数学实现。

## 阶段四：现代动画技术与前沿插件 (Modern Features)
*目标：学习 UE5 引入的最新动画技术。*

- [ ] **Control Rig (程序化动画与控制绑定)**
  - [ ] 源码位置: `Engine/Plugins/Animation/ControlRig`
  - [ ] 重点: 理解基于 VM（虚拟机）的执行架构（RigVM），它是如何做到高性能并独立于传统 AnimGraph 运行的。
- [ ] **IK Rig & IK Retargeter (全新重定向与 IK 框架)**
  - [ ] 源码位置: `Engine/Plugins/Animation/IKRig`
  - [ ] 重点: 研究 Full Body IK (FBIK) 求解器和不同骨架之间的数据映射原理。
- [ ] **Motion Warping (动作矫正/运动扭曲)**
  - [ ] 源码位置: `Engine/Plugins/Runtime/MotionWarping`
  - [ ] 重点: 分析它是如何在动画播放过程中，动态修改 Root Motion 数据以匹配环境目标的（比如翻越障碍物）。
- [ ] **Inertialization (惯性混合)**
  - [ ] 源码位置: `FAnimNode_Inertialization`
  - [ ] 重点: 理解它与传统 Crossfade (淡入淡出) 的本质区别。为什么它性能更好且能实现死混合 (Dead Blending)？

## 学习建议与技巧
1. **带着问题看源码**：不要盲目从头读到尾。比如“当我修改了状态机的一个变量，底层的状态是如何发生切换的？”带着这个问题去 Trace。
2. **利用你的 Zed 索引**：多用 `Ctrl+Click` (跳转到定义) 和查找所有引用。
3. **做笔记、画流程图**：动画系统的执行流非常深，调用栈很长，强推用画图工具（如 Excalidraw, Draw.io 或直接在 Markdown 里用 Mermaid 语法）画出数据流向。
