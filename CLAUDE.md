# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Repository Purpose

This is a learning repository focused on understanding Unreal Engine's animation framework through source code analysis. It contains:
- Selected Unreal Engine source files (animation-related modules only)
- Learning notes documenting animation system architecture and Q&A
- Development tooling for C++ code navigation with clangd

**Important**: This is NOT a full Unreal Engine project. It's a curated subset for studying animation internals.

## Repository Structure

```
Engine/
├── Source/Runtime/
│   ├── Engine/Private/Animation/     # Core animation implementation
│   ├── AnimationCore/                # Low-level animation math (transforms, IK algorithms)
│   ├── AnimGraphRuntime/             # Animation graph node implementations
│   └── AnimGraph/                    # Animation graph compilation
└── Plugins/Animation/
    ├── ControlRig/                   # Procedural animation & RigVM
    ├── IKRig/                        # Full-body IK & retargeting
    └── MotionWarping/                # Runtime motion adjustment

LearningNotes/
├── 00_Learning_Checklist.md          # Structured learning roadmap
├── 01_QA_Master.md                   # Q&A from source code study
└── 02_Montage_DeepDive.md            # Deep dive into specific topics
```

## Key Concepts

### Animation System Architecture
- **UAnimInstance**: Game thread interface, handles gameplay logic and property updates
- **FAnimInstanceProxy**: Worker thread proxy for parallel animation evaluation
- **FAnimNode_Base**: Base class for all animation graph nodes with Initialize/Update/Evaluate lifecycle
- **FCompactPose**: Core data structure representing bone transforms in local space
- **Component Space vs Local Space**: IK operations typically require component space transforms

### Source Code Organization
- **Public headers**: `Engine/Source/Runtime/*/Public/` - API interfaces
- **Classes**: `Engine/Source/Runtime/Engine/Classes/Animation/` - UObject definitions
- **Private implementation**: `Engine/Source/Runtime/*/Private/` - Implementation details

### Modern Animation Features (UE5+)
- **Control Rig**: VM-based procedural animation system independent of AnimGraph
- **IK Rig**: Full-body IK solver with cross-skeleton retargeting
- **Motion Warping**: Dynamic root motion adjustment for environmental interaction
- **Inertialization**: Dead blending technique superior to traditional crossfade

## Development Commands

### Generate Compilation Database
```bash
python gen_compile_db.py
```
This creates `compile_commands.json` for clangd IntelliSense. Run after modifying source directories in `gen_compile_db.py`.

### Source Directories Tracked
Defined in `gen_compile_db.py`:
- `Engine/Source/Runtime/Engine/Private/Animation`
- `Engine/Source/Runtime/AnimGraphRuntime`
- `Engine/Source/Runtime/AnimationCore`
- `Engine/Plugins/Animation/ControlRig`
- `Engine/Plugins/Animation/IKRig`

### C++ Language Server Configuration
`.clangd` is configured with:
- C++20 standard
- Unreal Engine-specific macros (CORE_API, ENGINE_API, etc.)
- Suppressed diagnostics (this is reference code, not for compilation)
- Microsoft extensions enabled for UE compatibility

## Working with This Repository

### When Reading Source Code
1. Start with the learning checklist (`LearningNotes/00_Learning_Checklist.md`) to understand the recommended study order
2. Use Q&A document (`LearningNotes/01_QA_Master.md`) to understand common confusion points
3. Key entry points for tracing:
   - Animation lifecycle: `UAnimInstance::NativeUpdateAnimation`
   - Evaluation pipeline: `USkeletalMeshComponent::EvaluateAnimation`
   - Node execution: `FAnimNode_Base::Initialize_AnyThread/Update_AnyThread/Evaluate_AnyThread`

### When Adding Notes
- Follow the existing structure in `LearningNotes/`
- Use Chinese for documentation (matches existing notes)
- Include source file paths and line numbers for reference
- Document the "why" behind architectural decisions, not just the "what"

### Code Navigation Tips
- Use clangd's "Go to Definition" (Ctrl+Click) extensively
- Search for class/function names with Grep tool across the codebase
- Animation nodes follow naming pattern: `FAnimNode_*` (structs) and `UAnimGraphNode_*` (editor nodes)
- Core classes use `U` prefix (UObject) or `F` prefix (plain structs)

## Important Notes

- This repository contains READ-ONLY reference code from Unreal Engine
- Do not attempt to build or compile - this is not a buildable project
- The compilation database is only for IDE navigation, not actual compilation
- Focus on understanding architecture and data flow, not modifying code
- When documenting findings, reference specific source files and line numbers
