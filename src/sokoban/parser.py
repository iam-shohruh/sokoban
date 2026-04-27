"""
Parses raw Sokoban level datasets from data/raw/<dataset>/ into
data/parsed/<dataset>/levels.json.

Supported formats (auto-detected):
  - sok:      single .sok file containing multiple levels (SokobanYASC format)
  - xsokoban: directory of screen.N files, one level each, pure grid

Grid is stored as a list of raw strings (rows), variable-width.
width = max row length, height = row count.
Tile chars per README: # wall, $ box, * box-on-goal, . goal, @ player, + player-on-goal, space floor.
"""

import argparse
import json
import re
import sys
from pathlib import Path

_ROOT = Path(__file__).resolve().parents[2]
_RAW_DIR = _ROOT / "data" / "raw"
_PARSED_DIR = _ROOT / "data" / "parsed"

_GRID_CHARS = frozenset(" #.$*@+")


def _is_grid_line(line: str) -> bool:
    return bool(line) and all(c in _GRID_CHARS for c in line)


# ---------------------------------------------------------------------------
# .sok parser  (SokobanYASC / Sokobano.de format)
# ---------------------------------------------------------------------------

def _parse_collection_header(lines: list[str]) -> dict:
    """Extract file-level key:value pairs before the first level block."""
    meta = {}
    for line in lines:
        m = re.match(r'^([A-Za-z][A-Za-z\-]*)\s*:\s*(.*)', line)
        if m:
            meta[m.group(1).lower()] = m.group(2).strip()
    return meta


def _parse_sok(path: Path, dataset_name: str) -> dict:
    with open(path, encoding="latin-1") as f:
        raw = f.read()

    lines = raw.splitlines()

    # Split into blocks separated by blank lines
    blocks: list[list[str]] = []
    current: list[str] = []
    for line in lines:
        if line.strip() == "":
            if current:
                blocks.append(current)
                current = []
        else:
            current.append(line)
    if current:
        blocks.append(current)

    # The header block(s) come before the first level block.
    # A level block starts with a bare integer line.
    header_lines: list[str] = []
    level_blocks: list[list[str]] = []
    in_levels = False
    for block in blocks:
        if not in_levels and re.match(r'^\d+$', block[0].strip()):
            in_levels = True
        if in_levels:
            level_blocks.append(block)
        else:
            header_lines.extend(block)

    collection_meta = _parse_collection_header(header_lines)

    levels = []
    for block in level_blocks:
        # First line is the level number within the set
        level_id = int(block[0].strip())
        rest = block[1:]

        grid = []
        meta_lines = []
        in_grid = True
        comment_lines: list[str] = []
        in_comment = False

        for line in rest:
            if in_comment:
                if line.startswith("Comment-End:"):
                    in_comment = False
                else:
                    comment_lines.append(line)
            elif line.startswith("Comment:"):
                in_comment = True
            elif in_grid and _is_grid_line(line):
                grid.append(line)
            else:
                in_grid = False
                meta_lines.append(line)

        per_level_meta = _parse_collection_header(meta_lines)
        comment = "\n".join(comment_lines).strip() or None

        width = max((len(r) for r in grid), default=0)
        height = len(grid)

        levels.append({
            "id": level_id,
            "title": per_level_meta.get("title"),
            "author": per_level_meta.get("author"),
            "comment": comment,
            "source_file": path.name,
            "width": width,
            "height": height,
            "grid": grid,
        })

    return {
        "dataset": dataset_name,
        "format": "sok",
        "collection_meta": collection_meta,
        "levels": levels,
    }


# ---------------------------------------------------------------------------
# xsokoban parser  (one screen.N file per level, pure grid)
# ---------------------------------------------------------------------------

def _parse_xsokoban(dataset_path: Path, dataset_name: str) -> dict:
    screen_files = sorted(
        dataset_path.glob("screen.*"),
        key=lambda p: int(p.suffix.lstrip(".")),
    )

    levels = []
    for path in screen_files:
        level_id = int(path.suffix.lstrip("."))
        with open(path, encoding="ascii") as f:
            grid = [line.rstrip("\n") for line in f.readlines()]

        # Drop trailing blank lines
        while grid and not grid[-1].strip():
            grid.pop()

        width = max((len(r) for r in grid), default=0)
        height = len(grid)

        levels.append({
            "id": level_id,
            "title": None,
            "author": None,
            "comment": None,
            "source_file": path.name,
            "width": width,
            "height": height,
            "grid": grid,
        })

    return {
        "dataset": dataset_name,
        "format": "xsokoban",
        "collection_meta": {},
        "levels": levels,
    }


# ---------------------------------------------------------------------------
# Format detection + dispatch
# ---------------------------------------------------------------------------

def _detect_format(dataset_path: Path) -> str:
    if list(dataset_path.glob("*.sok")):
        return "sok"
    if list(dataset_path.glob("screen.*")):
        return "xsokoban"
    raise ValueError(
        f"Cannot detect format for '{dataset_path}'. "
        "Expected .sok file or screen.N files."
    )


def parse_dataset(dataset_name: str) -> None:
    dataset_path = _RAW_DIR / dataset_name
    if not dataset_path.is_dir():
        raise FileNotFoundError(
            f"'{dataset_path}' not found. "
            f"Input directory must be within data/raw/."
        )

    fmt = _detect_format(dataset_path)
    print(f"Detected format: {fmt}")

    if fmt == "sok":
        sok_files = list(dataset_path.glob("*.sok"))
        if len(sok_files) != 1:
            raise ValueError(f"Expected exactly one .sok file, found {len(sok_files)}")
        result = _parse_sok(sok_files[0], dataset_name)
    elif fmt == "xsokoban":
        result = _parse_xsokoban(dataset_path, dataset_name)
    else:
        raise ValueError(f"Unhandled format: {fmt}")

    out_dir = _PARSED_DIR / dataset_name
    out_dir.mkdir(parents=True, exist_ok=True)
    out_path = out_dir / "levels.json"

    with open(out_path, "w", encoding="utf-8") as f:
        json.dump(result, f, indent=2, ensure_ascii=False)

    print(f"Wrote {len(result['levels'])} levels → {out_path}")


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Parse a raw Sokoban dataset into data/parsed/<dataset>/levels.json"
    )
    parser.add_argument(
        "dataset",
        help="Directory name within data/raw/ (e.g. microban, xsokoban)",
    )
    args = parser.parse_args()
    try:
        parse_dataset(args.dataset)
    except (FileNotFoundError, ValueError) as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()
