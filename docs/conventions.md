# Conventions

Ground rules for keeping the repo consistent across contributors.

---

## Devlogs

One file per session, in `devlog/`.

**Naming:** `YYYY-MM-DD_author.md`

```
devlog/
├── 2026-04-28_shohruh.md
└── 2026-04-29_alice.md
```

- ISO date first — files sort chronologically in any file browser
- Author in filename — no collision when two people log the same day
- No time component — write it when the session ends, not when it starts
- Multiple sessions same day: `2026-04-28_shohruh_2.md`

**Entry format:**

```markdown
## YYYY-MM-DD — @author

One-line summary of the session.

**Done:**
- ...

**Decisions:**
- Why X over Y, tradeoffs made, things that aren't obvious from the code

**Blocked / next:**
- ...
```

Write what's useful, skip what isn't. The **Decisions** section matters most — code is self-evident, reasoning isn't.

---

## Branches

```
<type>/<short-description>
```

| Type | When |
|------|------|
| `feat/` | new feature or module |
| `fix/` | bug fix |
| `refactor/` | restructuring without behaviour change |
| `data/` | dataset additions or parser changes |
| `docs/` | documentation only |
| `exp/` | throwaway experiment, not expected to merge cleanly |

Examples: `feat/bfs-solver`, `fix/parser-sok-encoding`, `exp/astar-heuristic-v2`

---

## Commits

```
<type>: <short imperative description>
```

Same types as branches. Keep the subject line under 72 characters. No period at the end.

```
feat: add BFS solver with timeout support
fix: handle variable-width grid rows in env.py
data: add microban dataset (155 levels)
docs: add conventions guide
```

---

## Python

- **Modules:** `snake_case.py`
- **Classes:** `PascalCase`
- **Functions / variables:** `snake_case`
- **Constants:** `UPPER_SNAKE_CASE`
- **Private helpers:** prefix with `_` (e.g. `_run_level`, `_save_results`)

---

## Solver interface

Every solver algorithm (in `search/` or `RL/`) must expose:

```python
def solve(state: State) -> list[Action] | None:
    ...
```

- Returns a list of actions if solved, `None` if unsolved or timed out
- The solver calls `env.py` functions internally — the orchestrator (`eval.py`) does not pass an env
- Register new solvers in the `SOLVERS` dict in `eval.py`

---

## Results

`eval.py` writes to:

```
results/<dataset>/<solver>/
├── levels.json     # per-level: id, solved, moves, elapsed_s
└── summary.json    # aggregate: solved/total, mean time, mean moves
```

Do not commit the `results/` directory.
