#!/usr/bin/env python3
"""Reject every `goto` token in project C sources (including forward jumps)."""

from __future__ import annotations

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
SOURCE_ROOT = ROOT / "server"
C_SUFFIXES = {".c", ".h"}
SKIP_DIRECTORIES = {".git", "build"}

# Remove comments and string/character literals while preserving newlines, so
# words such as "goto" in diagnostics or documentation comments are ignored.
NON_CODE = re.compile(
    r"//[^\n]*|/\*.*?\*/|\"(?:\\.|[^\"\\])*\"|'(?:\\.|[^'\\])*'",
    re.DOTALL,
)
GOTO_TOKEN = re.compile(r"\bgoto\b")


def blank_non_code(match: re.Match[str]) -> str:
    return "".join("\n" if character == "\n" else " " for character in match.group())


def source_files() -> list[Path]:
    return sorted(
        path
        for path in SOURCE_ROOT.rglob("*")
        if path.is_file()
        and path.suffix in C_SUFFIXES
        and not any(part in SKIP_DIRECTORIES for part in path.parts)
    )


def main() -> int:
    failures = 0
    for path in source_files():
        source = path.read_text(encoding="utf-8")
        code = NON_CODE.sub(blank_non_code, source)
        for match in GOTO_TOKEN.finditer(code):
            line = code.count("\n", 0, match.start()) + 1
            print(f"{path.relative_to(ROOT)}:{line}: error: 'goto' is forbidden", file=sys.stderr)
            failures += 1

    if failures:
        print(f"no-goto lint failed: {failures} forbidden use(s)", file=sys.stderr)
        return 1
    print("no-goto lint passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
