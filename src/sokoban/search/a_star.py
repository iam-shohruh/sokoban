from sokoban.state import Level, Position, Push, State
from sokoban.env import SokobanEnv
import heapq

def manhattan_heuristic(state: State, level: Level) -> int:
    """
    Heuristic function for A* search algorithm. It estimates the cost to reach the goal state from the given state.
    This heuristic is implemented as the sum of the Manhattan distances from each box to the nearest goal.

    Args:
        state (State): The current state of the game.
        level (Level): The level of the game, containing the static components such as walls and goals.

    Returns:
        int: The estimated cost to reach the goal state from the given state.
    """
    total_distance = 0
    taken: set[Position] = set()
    for box in state.boxes:
        chosen_goal: Position = None
        min_distance = float('inf')
        for goal in level.goals:
            if goal not in taken:
                distance = abs(box.row - goal.row) + abs(box.col - goal.col)
                if distance < min_distance:
                    min_distance = distance
                    chosen_goal = goal

        total_distance += min_distance
        taken.add(chosen_goal)
    return total_distance

def hungarian_heuristic(state: State, level: Level) -> int:
    pass

def pattern_database_heuristic(state: State, level: Level) -> int:
    """
    Placeholder for a more advanced heuristic that uses a pattern database.
    Pattern databases store precomputed costs for specific configurations of boxes and goals, allowing for more informed estimates.
    """
    pass

def hybrid_heuristic(state: State, level: Level) -> int:
    """
    Hybrid heuristic combines both pattern and hungarian heuristics, taking the maximum of the two to provide a more accurate estimate.
    """
    pass

def a_star_solver(level: Level, debug: bool = False, steps_out: list | None = None) -> dict:
    """
    Solves a Sokoban level using the A* search algorithm.

    Args:
        level (Level): The Sokoban level to solve.
        debug (bool): When True, collects steps internally and returns them
                      under result["steps"].
        steps_out (list | None): External list to append steps into as the
                                 search runs (used for live pygame replay).
                                 If provided, steps are written here instead
                                 of (or in addition to) the return value.

    Returns:
        dict: Results with "solved", "path", and optionally "steps".
    """
    env = SokobanEnv(level)
    start_state = level.init_state

    # Decide where to collect steps
    collecting = debug or steps_out is not None
    steps: list[dict] | None = steps_out if steps_out is not None else ([] if debug else None)

    counter = 0
    queue = [(0, counter, start_state)]
    visited = set([start_state])
    parent: dict[State, tuple[State, Push] | None] = {start_state: None}
    g_costs: dict[State, int] = {start_state: 0}
    h_costs: dict[State, int] = {start_state: manhattan_heuristic(start_state, level)}

    if collecting:
        h0 = h_costs[start_state]
        steps.append({"state": start_state, "push": None, "g": 0, "h": h0, "f": h0})

    while queue:
        _, _, state = heapq.heappop(queue)
        if env.is_goal_state(state):
            pushes = []
            current = state
            while parent[current] is not None:
                prev_state, push = parent[current]
                pushes.append(push)
                current = prev_state
            pushes.reverse()
            result = {"solved": True, "path": pushes}
            if collecting:
                result["steps"] = steps
            
            print(f"Solved level {level.game_id} in {len(pushes)} pushes")
            return result

        for push in env.get_valid_pushes(state):
            new_state = env.step(state, push)
            if env.is_deadlock(new_state):
                continue
            g_cost = g_costs[state] + 1
            h_cost = manhattan_heuristic(new_state, level)
            f_cost = g_cost + h_cost
            if new_state not in visited or g_cost < g_costs.get(new_state, float('inf')):
                visited.add(new_state)
                parent[new_state] = (state, push)
                g_costs[new_state] = g_cost
                h_costs[new_state] = h_cost
                counter += 1
                heapq.heappush(queue, (f_cost, counter, new_state))
                if collecting:
                    steps.append({"state": new_state, "push": push, "g": g_cost, "h": h_cost, "f": f_cost})

    result = {"solved": False, "path": []}
    if collecting:
        result["steps"] = steps
    return result


