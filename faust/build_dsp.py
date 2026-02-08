#!/usr/bin/env python3

import subprocess
from pathlib import Path
import sys

# Exit on error
def run_command(cmd):
    try:
        subprocess.run(cmd, check=True)
    except subprocess.CalledProcessError as e:
        print(f"Command failed: {e}", file=sys.stderr)
        sys.exit(1)

# Paths
ROOT_DIR = Path(__file__).resolve().parent
DSP_DIR = ROOT_DIR / "faust_dsp"
OUT_DIR = ROOT_DIR / "include/faust/generated"
ARCH_FILE = ROOT_DIR / "include/faust/faustMinimalInlined.h"

# Ensure output directory exists
OUT_DIR.mkdir(parents=True, exist_ok=True)

# Build all .dsp files
for dsp_path in DSP_DIR.glob("*.dsp"):
    name = dsp_path.stem
    out_path = OUT_DIR / f"{name}.h"

    print(f"FAUST  {dsp_path.name} -> {out_path}")

    run_command([
        "faust",
        "-i",
        "-a", str(ARCH_FILE),
        str(dsp_path),
        "-o", str(out_path)
    ])

print("All DSP files generated successfully.")

