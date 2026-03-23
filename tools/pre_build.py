#!/usr/bin/env python3
"""
Pre-build script for PlatformIO
Runs all necessary build preparation scripts in sequence
"""

Import("env")

import sys
import subprocess
from pathlib import Path
import os

# Get project directory and tools directory
project_dir = Path(env.get("PROJECT_DIR"))
tools_dir = project_dir / "tools"

print("\n" + "=" * 60)
print("PlatformIO Pre-Build Scripts")
print("=" * 60)

# List of scripts to run in order
scripts = [tools_dir / "compress_assets.py", tools_dir / "convert_region_map.py"]

# Run all scripts
for script in scripts:
    if not script.exists():
        print(f"✗ Error: Required script {script.name} not found at {script}")
        print(f"✗ Pre-build failed - cannot continue without {script.name}")
        sys.exit(1)

    print(f"\n{'='*60}")
    print(f"Running: {script.name}")
    print(f"{'='*60}")

    try:
        result = subprocess.run([sys.executable, str(script)], cwd=project_dir, check=True)
        print(f"✓ {script.name} completed successfully")
    except subprocess.CalledProcessError as e:
        print(f"✗ {script.name} failed with exit code {e.returncode}")
        sys.exit(1)
    except Exception as e:
        print(f"✗ Error running {script.name}: {e}")
        sys.exit(1)

print("\n" + "=" * 60)
print("✓ All pre-build scripts completed successfully")
print("=" * 60 + "\n")
