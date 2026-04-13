import json
import os

root_dir = r"C:\Inside-Unreal-Animation"
source_dirs = [
    r"Engine\Source\Runtime\Engine\Private\Animation",
    r"Engine\Source\Runtime\AnimGraphRuntime",
    r"Engine\Source\Runtime\AnimationCore",
    r"Engine\Plugins\Animation\ControlRig",
    r"Engine\Plugins\Animation\IKRig"
]

include_dirs = [
    r"Engine\Source\Runtime\Engine\Public\Animation",
    r"Engine\Source\Runtime\Engine\Classes\Animation",
    r"Engine\Source\Runtime\Engine\Public",
    r"Engine\Source\Runtime\Engine\Classes",
    r"Engine\Source\Runtime\Core\Public",
    r"Engine\Source\Runtime\CoreUObject\Public",
    r"Engine\Source\Runtime\AnimationCore\Public",
    r"Engine\Source\Runtime\AnimGraphRuntime\Public",
    r"Engine\Plugins\Animation\ControlRig\Source\ControlRig\Public",
    r"Engine\Plugins\Animation\IKRig\Source\IKRig\Public"
]

compilation_db = []
for s_dir in source_dirs:
    full_s_dir = os.path.join(root_dir, s_dir)
    if not os.path.exists(full_s_dir):
        continue
    
    for root, _, files in os.walk(full_s_dir):
        for file in files:
            if file.endswith(".cpp"):
                file_path = os.path.join(root, file)
                command = ["clang++"]
                for i_dir in include_dirs:
                    command.append(f"-I{os.path.join(root_dir, i_dir)}")
                command.append("-c")
                command.append(file_path)
                
                compilation_db.append({
                    "directory": root_dir,
                    "command": " ".join(command),
                    "file": file_path
                })

with open(os.path.join(root_dir, "compile_commands.json"), "w") as f:
    json.dump(compilation_db, f, indent=4)

print(f"Generated compile_commands.json with {len(compilation_db)} entries.")
