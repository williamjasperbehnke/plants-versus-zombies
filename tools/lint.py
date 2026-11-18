#!/usr/bin/env python3
"""Check project C/C++ layout with clang-format 19; use --fix to apply it."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parent.parent


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--fix', action='store_true', help='format files in place')
    args = parser.parse_args()
    requested = os.environ.get('CLANG_FORMAT')
    formatter = shutil.which(requested) if requested else (
        shutil.which('clang-format-19') or shutil.which('clang-format'))
    if not formatter:
        parser.exit(2, 'clang-format 19 is required. Set CLANG_FORMAT to its executable path.\n')
    version = subprocess.run([formatter, '--version'], check=True, capture_output=True, text=True).stdout
    if 'version 19.' not in version:
        parser.exit(2, f'Expected clang-format 19; found {version.strip()}.\n')
    files = sorted(p for folder in ('include', 'src', 'tests', 'tools')
                   for p in (ROOT / folder).rglob('*') if p.suffix in ('.h', '.cpp', '.c'))
    options = ['-i'] if args.fix else ['--dry-run', '--Werror']
    result = subprocess.run([formatter, '--style=file', *options, *(str(p) for p in files)], cwd=ROOT)
    if result.returncode:
        print('Run python3 tools/lint.py --fix to apply the project style.', file=sys.stderr)
        return result.returncode
    print(f'{"Formatted" if args.fix else "Style passed:"} {len(files)} C/C++ files.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
