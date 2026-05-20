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
import time, argparse, json
from sokoban.state import Level, State, Position


SOLVERS = {
    "bfs": bfs_solver,
    "a_star": a_star_solver,
}

DATASETS = {
    "microban": "data/parsed/microban/levels.json",
    "xsokoban": "data/parsed/xsokoban/levels.json",
}

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
        start_time = time.time()

        result = solver_fn(level)

        end_time = time.time()

        result["time"] = end_time - start_time
        results.append(result)

    # _save(results, dataset, solver)

    # return _summarize(results)

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

def _save(results: list, dataset: str, solver: str) -> None:
    """
    Saves the evaluation results to a file.

    Args:
        results (list): A list of results from the evaluation.
        dataset (str): The name of the dataset used for evaluation.
        solver (str): The name of the solver used for evaluation.

    Returns:
        None
    """
    # Placeholder for saving results to a file
    # This function should save the results in a structured format (e.g., JSON, CSV) in the sokoban/results folder
    pass
    
def _summarize(results: list) -> dict:
    """
    Summarizes the evaluation results.

    Args:
        results (list): A list of results from the evaluation.

    Returns:
        dict: A summary of the evaluation results, including metrics such as average time, success rate, etc.
    """
    # Placeholder for summarizing results
    # This function should compute and return summary based on the evaluation results
    return {}

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Evaluate Sokoban solvers on specified datasets.")
    parser.add_argument("--solver", type=str, required=True, help="The name of the solver to evaluate.")
    parser.add_argument("--dataset", type=str, required=True, help="The name of the dataset to use.")

    args = parser.parse_args()
    summary = evaluate(args.dataset, args.solver)
    print(summary)




