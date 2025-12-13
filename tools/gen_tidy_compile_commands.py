#!/usr/bin/env python3
import json
import os
import re
import shutil
import sys

SRC_DB = os.path.join('build', 'compile_commands.json')
OUT_DIR = os.path.join('build', 'compile_commands_tidy')
OUT_DB = os.path.join(OUT_DIR, 'compile_commands.json')

# Flags that Clang doesn't accept from Xtensa GCC toolchain
STRIP_FLAGS = [
    '-mlongcalls',
    '-fno-shrink-wrap',
    '-fno-tree-switch-conversion',
    '-fstrict-volatile-bitfields',
]

# Strip entire -f.../ -m... categories if needed
STRIP_PREFIXES = [
    '-mxtensa-',
]

# Replace the compiler with host clang++ for analysis
CLANG_CXX = 'clang++'

# Ensure output directory exists
os.makedirs(OUT_DIR, exist_ok=True)

if not os.path.exists(SRC_DB):
    print(f"Source DB not found: {SRC_DB}", file=sys.stderr)
    sys.exit(1)

with open(SRC_DB, 'r') as f:
    db = json.load(f)

filtered = []
for entry in db:
    # Only include project files (exclude esp-idf sources)
    file_path = entry.get('file', '')
    if re.search(r"/(esp-idf/|bootloader/|build/)", file_path):
        continue

    command = entry.get('command')
    arguments = entry.get('arguments')

    if arguments:
        args = list(arguments)
    else:
        # Split command into args safely (simple split; good enough for typical compile_commands)
        args = command.split()

    # Replace compiler with clang++ and strip Xtensa-specific flags
    if args:
        args[0] = CLANG_CXX

    def keep(a: str) -> bool:
        if a in STRIP_FLAGS:
            return False
        for p in STRIP_PREFIXES:
            if a.startswith(p):
                return False
        # drop target triple flags if present
        if a.startswith('--target=') or a == '-target' or a.startswith('xtensa-'):
            return False
        # drop esp-idf/toolchain include paths to avoid missing system headers
        if a.startswith('-I') or a.startswith('-isystem'):
            inc = a[2:] if a.startswith('-I') else a[len('-isystem'):].strip()
            if 'esp-idf' in inc or 'esp@' in inc or 'xtensa' in inc:
                return False
        return True

    args = [a for a in args if keep(a)]

    # Ensure language/std are present for clang++
    if not any(a.startswith('-std=') for a in args):
        args.append('-std=c++20')

    # Ensure defines for ESP are kept if present; we don't add defaults here

    filtered.append({
        'directory': entry.get('directory'),
        'file': file_path,
        'arguments': args,
    })

with open(OUT_DB, 'w') as f:
    json.dump(filtered, f, indent=2)

print(f"Generated filtered compile_commands for clang-tidy: {OUT_DB}")