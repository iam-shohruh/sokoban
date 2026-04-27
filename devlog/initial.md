# Devlog

One entry per session. Newest at the top. Tag your name. Link GitHub issues where relevant.
Use this format loosely — write what's useful, skip what isn't.

---

## 2026-04-28 — @shohruh

Set up the data pipeline: parser that converts raw Sokoban level files into structured JSON.

**Done:**
- Implemented `src/sokoban/parser.py` with support for two dataset formats:
  - `.sok` (SokobanYASC format) — used by microban (155 levels, with collection + per-level metadata)
  - `xsokoban` — directory of `screen.N` files, one level per file, no metadata (90 levels)
- Format is auto-detected from directory contents
- Output: `data/parsed/<dataset>/levels.json` with fields: `id`, `title`, `author`, `comment`, `source_file`, `width`, `height`, `grid`
- Wrote README: tile character reference, parser usage, output schema, how to add new datasets

**Decisions:**
- Grid stored as raw variable-width strings (rows not padded to uniform width). `env.py` pads on load. Keeps the parser a faithful representation of the source.
- One `levels.json` per dataset, not one file per level. Simpler for training scripts to load everything at once.
- Tile encoding (char → int) stays out of the parser — that belongs in `env.py`.
- Format detection is automatic: `.sok` file in dir → sok parser, `screen.*` files → xsokoban parser.

**Sanity checks passed:**
- Every level has exactly one player (`@` or `+`)
- Box count == goal count for all 245 levels across both datasets
