"""
This file serves as the main entry point for evaluating Sokoban puzzle solutions using different search algorithms.

In terminal, enter dataset and solver name, then eval.py will call the corresponding solver, which then will 
convert levels into python workable state, solve the levels, saves the solution results in sokoban/results folder,
and finally print the results in terminal.

Example usage: 
    python eval.py --dataset=microban --solver=bfs

"""



from sokoban.search.bfs import bfs_solver
from sokoban.search.a_star import a_star_solver
import time, argparse, json, os
from sokoban.state import Level, State, Position
from datetime import datetime, UTC


SOLVERS = {
    "bfs": bfs_solver,
    "a_star": a_star_solver,
}

DATASETS = {
    "microban": "data/parsed/microban/levels.json",
    "xsokoban": "data/parsed/xsokoban/levels.json",
}


def _format_deadlock_stats(stats: dict[str, int]) -> str:
    total = sum(stats.values())
    non_zero = [(name, count) for name, count in stats.items() if count > 0]
    if not non_zero:
        return f"total={total} (no deadlocks detected)"
    details = ", ".join(f"{name}={count}" for name, count in non_zero)
    return f"total={total} ({details})"

def evaluate(dataset: str, solver: str) -> dict:
    """
    Evaluates and benchmarks specific solvers on the Sokoban puzzles in the specified dataset.

    Args:
        dataset (str): The name of the dataset to evaluate.
        solver (str): The name of the solver to use.

    Returns:
        dict: A summary of the evaluation results.
    """
    if dataset not in DATASETS:
        print(f"Dataset '{dataset}' not found. Available datasets: {list(DATASETS.keys())}")
        return

    if solver not in SOLVERS:
        print(f"Solver '{solver}' not found. Available solvers: {list(SOLVERS.keys())}")
        return

    print(f"Evaluating solver '{solver}' on dataset '{dataset}'...")
    
    levels = load_levels(DATASETS[dataset])
    solver_fn = SOLVERS[solver]

    results = []
    for level in levels:
        if level.game_id in [93, 139, 144, 153]:
            continue
        
        start_time = time.time()

        result = solver_fn(level)

        end_time = time.time()

        explored = result.get("explored_states")
        explored_suffix = f", Explored: {explored}" if explored is not None else ""
        print(
            f"Level {level.game_id} - Solved: {result['solved']}, "
            f"Time: {end_time - start_time:.2f}s, "
            f"Path Length: {len(result['path']) if result['solved'] else 'N/A'}{explored_suffix}"
        )
        if "deadlock_stats" in result:
            print(f"  Deadlocks: {_format_deadlock_stats(result['deadlock_stats'])}")
        results.append(
            {
                "level_id": level.game_id,
                "solved": result["solved"],
                "time_sec": end_time - start_time,
                "path_length": len(result["path"]) if result["solved"] else None,
                "explored_states": result.get("explored_states"),
                "deadlock_stats": result.get("deadlock_stats"),
            }
        )

    summary = _summarize(results)
    save_path = _save(results, summary, dataset, solver)
    print(f"Saved report: {save_path}")
    return summary

def load_levels(dataset: str) -> list[Level]:
    """
    Loads Sokoban levels from the specified dataset.

    Args:
        dataset (str): The path to the dataset containing Sokoban levels.
    Returns:
        list[Level]: A list of Sokoban levels.
    """
    with open(dataset, "r") as f:
        dataset_json = json.load(f)
    
    levels = []
    for level_data in dataset_json["levels"]:
        level_dict = {"walls": [], "goals": [], "player": None, "boxes": []}

        for j, row in enumerate(level_data["grid"]):
            for i, cell in enumerate(row):
                if cell == "#":
                    level_dict["walls"].append(Position(j, i))
                elif cell == "@":
                    level_dict["player"] = Position(j, i)
                elif cell == "+":
                    level_dict["goals"].append(Position(j, i))
                    level_dict["player"] = Position(j, i)
                elif cell == "$":
                    level_dict["boxes"].append(Position(j, i))
                elif cell == "*":
                    level_dict["goals"].append(Position(j, i))
                    level_dict["boxes"].append(Position(j, i))
                elif cell == ".":
                    level_dict["goals"].append(Position(j, i))

        level = Level(
            game_id=level_data["id"],
            width=level_data["width"],
            height=level_data["height"],
            walls=frozenset(level_dict["walls"]),
            goals=frozenset(level_dict["goals"]),
            init_state=State(
                player=level_dict["player"],
                boxes=frozenset(level_dict["boxes"])
            )
        )
        levels.append(level)
    return levels

def _save(results: list[dict], summary: dict, dataset: str, solver: str) -> str:
    """
    Saves the evaluation results to a file.

    Args:
        results (list): A list of results from the evaluation.
        dataset (str): The name of the dataset used for evaluation.
        solver (str): The name of the solver used for evaluation.

    Returns:
        str: Path to the saved report.
    """
    out_dir = os.path.join("results", dataset)
    os.makedirs(out_dir, exist_ok=True)
    timestamp = datetime.now(UTC).strftime("%Y%m%dT%H%M%SZ")
    out_path = os.path.join(out_dir, f"{solver}_{timestamp}.txt")

    deadlock_totals = summary.get("deadlock_totals", {})
    deadlock_line = ", ".join(f"{k}={v}" for k, v in sorted(deadlock_totals.items()))
    if not deadlock_line:
        deadlock_line = "none"

    lines: list[str] = []
    lines.append("Sokoban Evaluation Report")
    lines.append(f"dataset={dataset} solver={solver} generated_at_utc={timestamp}")
    lines.append("")
    lines.append("Summary")
    lines.append(f"total_levels={summary['total_levels']}")
    lines.append(f"solved_levels={summary['solved_levels']}")
    lines.append(f"solve_rate={summary['solve_rate']:.4f}")
    lines.append(f"avg_time_sec={summary['avg_time_sec']:.4f}")
    lines.append(f"avg_path_length_solved={summary['avg_path_length_solved']:.4f}")
    lines.append(f"avg_explored_states={summary['avg_explored_states']:.2f}")
    lines.append(f"deadlock_totals={deadlock_line}")
    lines.append("")
    lines.append("Per-level")
    lines.append("level_id solved time_sec path_len explored corner dead_square 2x2")

    for r in sorted(results, key=lambda x: x["level_id"]):
        d = r.get("deadlock_stats") or {}
        lines.append(
            f"{r['level_id']:>7} "
            f"{str(r['solved']):<6} "
            f"{r['time_sec']:>8.4f} "
            f"{(r['path_length'] if r['path_length'] is not None else '-'):>8} "
            f"{(r['explored_states'] if r['explored_states'] is not None else '-'):>8} "
            f"{d.get('corner', 0):>6} "
            f"{d.get('dead_square', 0):>11} "
            f"{d.get('2x2', 0):>3}"
        )

    with open(out_path, "w", encoding="utf-8") as f:
        f.write("\n".join(lines) + "\n")
    return out_path
    
def _summarize(results: list) -> dict:
    """
    Summarizes the evaluation results.

    Args:
        results (list): A list of results from the evaluation.

    Returns:
        dict: A summary of the evaluation results, including metrics such as average time, success rate, etc.
    """
    if not results:
        return {
            "total_levels": 0,
            "solved_levels": 0,
            "solve_rate": 0.0,
            "avg_time_sec": 0.0,
            "avg_path_length_solved": 0.0,
            "avg_explored_states": 0.0,
            "deadlock_totals": {},
        }

    total = len(results)
    solved = [r for r in results if r["solved"]]
    solved_n = len(solved)

    avg_time = sum(r["time_sec"] for r in results) / total
    solved_path_lengths = [r["path_length"] for r in solved if r["path_length"] is not None]
    avg_path = (sum(solved_path_lengths) / len(solved_path_lengths)) if solved_path_lengths else 0.0
    explored_vals = [r["explored_states"] for r in results if r["explored_states"] is not None]
    avg_explored = (sum(explored_vals) / len(explored_vals)) if explored_vals else 0.0

    deadlock_totals: dict[str, int] = {}
    for r in results:
        stats = r.get("deadlock_stats") or {}
        for name, count in stats.items():
            deadlock_totals[name] = deadlock_totals.get(name, 0) + count

    return {
        "total_levels": total,
        "solved_levels": solved_n,
        "solve_rate": solved_n / total,
        "avg_time_sec": avg_time,
        "avg_path_length_solved": avg_path,
        "avg_explored_states": avg_explored,
        "deadlock_totals": deadlock_totals,
    }

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Evaluate Sokoban solvers on specified datasets.")
    parser.add_argument("--solver", type=str, required=True, help="The name of the solver to evaluate.")
    parser.add_argument("--dataset", type=str, required=True, help="The name of the dataset to use.")

    args = parser.parse_args()
    summary = evaluate(args.dataset, args.solver)
    print(summary)
