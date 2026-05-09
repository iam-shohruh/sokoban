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
    unreachable = env.level.width * env.level.height
    cost_matrix = []
    for box in state.boxes:
        row = []
        for goal in env.level.goals:
            row.append(env.point_to_goal.get(box, {}).get(goal, unreachable))
        cost_matrix.append(row)

    # Then, the Hungarian algorithm is applied to find the optimal assignment
    row_ind, col_ind = linear_sum_assignment(cost_matrix)
    total_cost = sum(cost_matrix[i][j] for i, j in zip(row_ind, col_ind))
    return total_cost

def custom_hungarian_heuristic(state: State, env: SokobanEnv) -> int:
    """
    Here will go our implementation of Hungarian heuristic.
    """
    raise NotImplementedError("custom_hungarian_heuristic is not implemented")

def a_star_solver(level: Level) -> dict:
    """
    Solves a Sokoban level using the A* search algorithm.

    Args:
        level (Level): The Sokoban level to solve.
    Returns:
        dict: Results with "solved", "path".
    """
    env = SokobanEnv(level, debug_deadlocks=True)
    start_state = env.normalize_player(level.init_state)

    counter = 0
    start_h = hungarian_heuristic(start_state, env)
    queue = [(start_h, 0, start_h, counter, start_state)]
    parent: dict[State, tuple[State, tuple[Push, ...]] | None] = {start_state: None}
    g_costs: dict[State, int] = {start_state: 0}

    closed = set()
    while queue:
        _, _, _, _, state = heapq.heappop(queue)
        if state in closed:
            continue
        closed.add(state)

        if env.is_goal_state(state):
            pushes = []
            current = state
            while parent[current] is not None:
                prev_state, push_seq = parent[current]
                pushes.extend(reversed(push_seq))
                current = prev_state
            pushes.reverse()
            result = {"solved": True, "path": pushes}
            result["explored_states"] = len(closed)
            result["deadlock_stats"] = dict(env.deadlock_stats)
            return result

        for new_state, push_seq in env.get_successors(state):
            if env.is_deadlock(new_state):
                continue
            step_cost = len(push_seq)
            g_cost = g_costs[state] + step_cost
            h_cost = hungarian_heuristic(new_state, env)
            f_cost = g_cost + h_cost
            if g_cost < g_costs.get(new_state, float('inf')):
                parent[new_state] = (state, push_seq)
                g_costs[new_state] = g_cost
                counter += 1
                heapq.heappush(queue, (f_cost, -g_cost, h_cost, counter, new_state))

    result = {"solved": False, "path": [], "explored_states": len(closed), "deadlock_stats": dict(env.deadlock_stats)}
    return result
