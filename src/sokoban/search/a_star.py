from sokoban.state import Level, Position, Push, State
from sokoban.env import SokobanEnv
import heapq
from scipy.optimize import linear_sum_assignment

def manhattan_heuristic(state: State, env: SokobanEnv) -> int:
    """
    Heuristic function for A* search algorithm. It estimates the cost to reach the goal state from the given state.
    This heuristic is implemented as the sum of the Manhattan distances from each box to the nearest goal.

    Args:
        state (State): The current state of the game.
        env (SokobanEnv): The Sokoban environment, containing the level and other game components.

    Returns:
        int: The estimated cost to reach the goal state from the given state.
    """
    total_distance = 0
    taken: set[Position] = set()
    for box in state.boxes:
        chosen_goal: Position = None
        min_distance = float('inf')
        for goal in env.level.goals:
            if goal not in taken:
                distance = abs(box.row - goal.row) + abs(box.col - goal.col)
                if distance < min_distance:
                    min_distance = distance
                    chosen_goal = goal

        total_distance += min_distance
        taken.add(chosen_goal)
    return total_distance

def hungarian_heuristic(state: State, env: SokobanEnv) -> int:
    """
    This heuristic uses the Hungarian algorithm to find the optimal assignment of boxes to goals, minimizing the total
    Manhattan distance to reach goals. The improvement from the basic Manhattan heuristic is that it considers the best 
    possible matching of boxes to goals, rather than just summing individual distances.

    Args:
        state (State): The current state of the game.
        env (SokobanEnv): The Sokoban environment, containing the level and other game components.

    Returns:
        int: The estimated cost to reach the goal state from the given state.
    """

    # First, cost matrix should be constructed where cost[i][j] is the distance from box i to goal j
    cost_matrix = []
    for box in state.boxes:
        row = []
        for goal in env.level.goals:
            row.append(env.point_to_goal[box][goal])
        cost_matrix.append(row)

    # Then, the Hungarian algorithm is applied to find the optimal assignment
    row_ind, col_ind = linear_sum_assignment(cost_matrix)
    total_cost = sum(cost_matrix[i][j] for i, j in zip(row_ind, col_ind))
    return total_cost

def custom_hungarian_heuristic(state: State, env: SokobanEnv) -> int:
    """
    Here will go our implementation of Hungarian heuristic.
    """
    pass

def a_star_solver(level: Level, debug: bool = False, steps_out: list | None = None) -> dict:
    """
    Solves a Sokoban level using the A* search algorithm.

    Args:
        level (Level): The Sokoban level to solve.
        heuristic (str): The heuristic function to use. Default is "hungarian". Options are "manhattan", "hungarian", and "custom_hungarian".
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
    h_costs: dict[State, int] = {start_state: hungarian_heuristic(start_state, env)}

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
            
            return result

        for push in env.get_valid_pushes(state):
            new_state = env.step(state, push)
            if env.is_deadlock(new_state):
                continue
            g_cost = g_costs[state] + 1
            h_cost = hungarian_heuristic(new_state, env)
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


