# IAI 2026 Project Report - Sokoban Puzzle Solver

## 1. Introduction
This project implements a Sokoban solver by adapting the provided generalized search C framework. The selected topic is **Sokoban Puzzle Game** (Project Topic 8), where an agent pushes boxes to goal cells on a grid with walls.

The objective is to compare blind and heuristic search on Microban levels using:
- BFS (`First_InsertFrontier_Search_TREE`)
- Generalized A* (`Insert_Priority_Queue_GENERALIZED_A_Star`)

The report is prepared in Markdown first for reproducibility, then converted to PDF.

## 2. Problem Formulation
### 2.1 State Space
A state is defined as:
- Player position `(row, col)`
- Set of box positions

In code, this is represented by `State` in `data_types.h`.

### 2.2 Initial State
For each level, the initial state is loaded from parsed Microban JSON:
- Wall layout
- Goal layout
- Initial player position
- Initial box positions

### 2.3 Actions
Valid actions are **push actions** in four directions:
- `UP`, `DOWN`, `LEFT`, `RIGHT`

The player may move to reachable cells and push a box if the destination behind the box is free.

### 2.4 Transition Model
Applying a push action:
1. Player moves to the pushed box position.
2. The box moves one cell in the push direction.
3. Box positions are normalized (sorted) to get canonical state representation.

### 2.5 Goal Test
A state is goal if **all boxes are on goal cells**.

### 2.6 Path Cost
Each push has uniform cost `1`. Therefore:
- BFS minimizes number of pushes.
- A* uses `f(n)=g(n)+h(n)` with push cost in `g`.

### 2.7 Constraints
- Agent can only **push**, not pull.
- Movement is 4-connected.
- Walls are impassable.
- Box cannot be pushed into walls/other boxes.

## 3. Algorithm Design
### 3.1 BFS
- Queue-based frontier.
- Visited-state hash table to prevent revisits.
- Deadlock pruning for simple corner deadlocks.

### 3.2 Generalized A*
- Priority queue (binary heap) ordered by lowest `f`.
- `g`: number of pushes so far.
- `h`: Hungarian matching based box-to-goal Manhattan assignment (`hungarian_heuristic`).
- Same visited hash table and deadlock pruning strategy.

### 3.3 Instrumentation Added
To satisfy assignment metrics, `SolverResult` was extended with:
- `nodes_visited`
- `deadlocks_detected`
- `solve_time_sec`

These are now reported in both single-level and batch runs.

## 4. Experimental Setup
### 4.1 Dataset and Scope
Primary dataset: `microban` (`155` levels).  
Detailed comparison in this report uses representative levels:
- Level 44 (1-box case)
- Levels 1 and 8 (2-box cases)
- Levels 60, 120, 150 (larger/harder)

### 4.2 Environment
- Language: C (C11)
- Build: `gcc -O2 -std=c11 -Wall -Wextra`
- Entry binary: `c_version/sokoban.exe`

### 4.3 Commands
```bash
cd c_version
make clean && make

# Single level (human-readable)
./sokoban.exe --dataset=microban --solver=bfs --level=44 --no-animate
./sokoban.exe --dataset=microban --solver=a_star --level=44 --no-animate

# Batch CSV output
./sokoban.exe --dataset=microban --solver=bfs --batch --csv
./sokoban.exe --dataset=microban --solver=a_star --batch --csv
```

### 4.4 Metric Definitions
- `path_len`: solution push count
- `solve_time_sec`: wall-clock solve time in seconds
- `nodes_visited`: number of states removed from frontier and expanded
- `deadlocks_detected`: number of generated states rejected by deadlock checks

## 5. Results
### 5.1 Per-Level Comparison (Selected Microban Levels)
Source file: `c_version/results_microban_selected.csv`

| solver | level_id | solved | path_len | solve_time_sec | nodes_visited | deadlocks_detected |
|---|---:|---:|---:|---:|---:|---:|
| bfs | 44 | 1 | 1 | 0.0005 | 2 | 0 |
| bfs | 1 | 1 | 8 | 0.0004 | 33 | 33 |
| bfs | 8 | 1 | 32 | 0.0011 | 190 | 108 |
| bfs | 60 | 1 | 44 | 0.0318 | 8077 | 8141 |
| bfs | 120 | 1 | 64 | 0.0380 | 12986 | 11119 |
| bfs | 150 | 1 | 43 | 8.6211 | 821071 | 884308 |
| a_star | 44 | 1 | 1 | 0.0005 | 2 | 0 |
| a_star | 1 | 1 | 8 | 0.0004 | 25 | 26 |
| a_star | 8 | 1 | 32 | 0.0008 | 137 | 57 |
| a_star | 60 | 1 | 44 | 0.0379 | 7931 | 7895 |
| a_star | 120 | 1 | 64 | 0.0566 | 12953 | 11102 |
| a_star | 150 | 1 | 43 | 3.8580 | 331841 | 347912 |

### 5.2 Aggregate Summary (Selected Levels)

| solver | levels | solve_rate_% | mean_path_len | mean_time_sec | mean_nodes_visited | mean_deadlocks_detected |
|---|---:|---:|---:|---:|---:|---:|
| bfs | 6 | 100.00 | 32.00 | 1.4488 | 140393 | 150618 |
| a_star | 6 | 100.00 | 32.00 | 0.6590 | 58815 | 61165 |

### 5.3 Required Screenshots to Include in Final PDF
Add screenshots for at least:
1. A 1-box level (e.g., level 44): initial, intermediate, solved.
2. A 2-box level (e.g., level 1 or 8): initial, intermediate, solved.
3. A harder level (e.g., level 150): console output showing metric differences.

## 6. Analysis and Discussion
- Both BFS and A* found optimal push counts on tested levels (same `path_len`).
- A* generally visited fewer nodes than BFS, especially on harder level 150 (`331,841` vs `821,071` visited).
- A* solved level 150 faster (`3.8580s` vs `8.6211s`).
- Deadlock detections grow rapidly on hard levels, indicating a large fraction of successor states are pruned before deeper exploration.
- Deadlock pruning is important to avoid exploring obviously losing box placements.

## 7. Conclusion
The Sokoban solver successfully adapts the course search framework and reports assignment-required metrics. On representative Microban levels, A* with Hungarian-style heuristic provides better scalability than BFS while preserving solution quality (push-optimal paths on tested instances).

Future improvements:
- Stronger deadlock detection (freeze patterns, tunnel deadlocks)
- Pattern databases or domain-specific admissible heuristics
- Memory-aware search variants for very hard levels

## Appendix
### A. Key Source Changes
- `c_version/data_types.h`: extended `SolverResult` with metric fields.
- `c_version/Standart_Search.c`: added BFS/A* instrumentation counters.
- `c_version/GRAPH_SEARCH.c`: unified timing, richer output, `--csv` batch mode.

### B. Data Files
- `c_version/results_microban_selected.csv`: selected-level comparison data.

### C. Markdown to PDF
```bash
cd c_version
pandoc report.md -o report.pdf
```

Optional styling:
```bash
pandoc report.md -o report.pdf --toc -V geometry:margin=1in
```
