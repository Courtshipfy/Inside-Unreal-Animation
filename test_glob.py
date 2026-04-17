from pathlib import Path
import sys

print("Python version:", sys.version)
print("Current dir:", Path.cwd())

engine_dir = Path('Engine')
print("Engine exists:", engine_dir.exists())

# Try different methods
print("\nTrying rglob:")
files1 = list(engine_dir.rglob('*.cpp'))
print(f"  rglob('*.cpp'): {len(files1)}")

print("\nTrying glob:**")
files2 = list(engine_dir.glob('**/*.cpp'))
print(f"  glob('**/*.cpp'): {len(files2)}")

if files2:
    print("\nFirst 3 files:")
    for f in files2[:3]:
        print(f"  {f}")
