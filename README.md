# Sokoban

A Sokoban solver for Intro2AI course at YTU.

## Project structure

```
sokoban/
├── data/
│   ├── raw/                        # original dataset files (unmodified)
│   │   ├── microban/
│   │   └── xsokoban/
│   └── parsed/                     # output of parser.py, one dir per dataset
│       ├── microban/levels.json
│       └── xsokoban/levels.json
├── results/                        # output of eval.py, one dir per dataset
│   └── <dataset>/
│       └── <solver>/
│           ├── levels.json         # per-level results
│           └── summary.json        # aggregate stats
└── src/sokoban/
    ├── parser.py                   # raw → data/parsed/<dataset>/levels.json
    ├── state.py                    # State dataclass, load_levels(dataset)
    ├── env.py                      # Sokoban environment (step, legal_actions, is_solved)
    ├── eval.py                     # orchestrator: runs a solver on a dataset, saves results
    ├── search/                     # search-based solvers (BFS, A*, IDA*, …)
    ├── RL/                         # reinforcement learning solvers
    └── viz/                        # visualisation utilities
        └── replay.py
```

## Design

```
BFS / A* / IDA*          RL agents
       \                  /
        search/        RL/
              \        /
               solver fn
                  |
  levels.json → eval.py → terminal summary
                  |
            save_results
                  |
           results/<dataset>/<solver>/
```

- **`parser.py`** converts raw dataset files into `data/parsed/<dataset>/levels.json`.
- **`state.py`** loads that JSON and exposes each level as a Python `State` object.
- **`env.py`** defines the Sokoban environment — functions for legal moves, applying an action, and checking if a level is solved. Solver algorithms call these directly.
- **`search/`** and **`RL/`** each contain solver algorithms. Every solver exposes a `solve(state) -> list[Action] | None` function.
- **`eval.py`** is the orchestrator. Given a dataset name and solver name it loads levels via `state.py`, runs the solver on each level, prints a summary to the terminal, and writes per-level and aggregate results to `results/`.

**Setup (first time only)**
```bash
source .venv/bin/activate
pip install -e .
```

**Run**
```bash
python src/sokoban/eval.py --dataset=microban --solver=bfs
```

Available datasets: `microban`, `xsokoban`  
Available solvers: `bfs`, `a_star`

## Level format

Levels are represented as a 2D grid of ASCII characters:

| Char | Meaning          |
|------|------------------|
| `#`  | Wall             |
| ` `  | Floor            |
| `@`  | Player           |
| `+`  | Player on goal   |
| `$`  | Box              |
| `*`  | Box on goal      |
| `.`  | Goal             |

A valid level has exactly one player (`@` or `+`) and equal numbers of boxes and goals.

