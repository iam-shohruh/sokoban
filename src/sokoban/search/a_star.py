from sokoban.state import Level, Position, Push, State
from sokoban.env import SokobanEnv
import heapq

def _hungarian_min_cost(cost_matrix: list[list[int]]) -> int:
    """
    Computes the minimum assignment cost using the Hungarian algorithm.
    Supports rectangular matrices where rows <= cols.
    """
    if not cost_matrix or not cost_matrix[0]:
        return 0

    n = len(cost_matrix)
    m = len(cost_matrix[0])
    if n > m:
        # Transpose when there are more rows than columns so rows <= cols.
        transposed = [[cost_matrix[r][c] for r in range(n)] for c in range(m)]
        return _hungarian_min_cost(transposed)

    # 1-indexed implementation of Hungarian algorithm for minimization.
    u = [0] * (n + 1)
    v = [0] * (m + 1)
    p = [0] * (m + 1)
    way = [0] * (m + 1)

    for i in range(1, n + 1):
        p[0] = i
        minv = [float("inf")] * (m + 1)
        used = [False] * (m + 1)
        j0 = 0
        while True:
            used[j0] = True
            i0 = p[j0]
            delta = float("inf")
            j1 = 0
            for j in range(1, m + 1):
                if used[j]:
                    continue
                cur = cost_matrix[i0 - 1][j - 1] - u[i0] - v[j]
                if cur < minv[j]:
                    minv[j] = cur
                    way[j] = j0
                if minv[j] < delta:
                    delta = minv[j]
                    j1 = j
            for j in range(m + 1):
                if used[j]:
                    u[p[j]] += delta
                    v[j] -= delta
                else:
                    minv[j] -= delta
            j0 = j1
            if p[j0] == 0:
                break
        while True:
            j1 = way[j0]
            p[j0] = p[j1]
            j0 = j1
            if j0 == 0:
                break

    assignment_cost = 0
    for j in range(1, m + 1):
        i = p[j]
        if i != 0:
            assignment_cost += cost_matrix[i - 1][j - 1]
    return int(assignment_cost)

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

    # Then, the Hungarian algorithm is applied to find the optimal assignment.
    return _hungarian_min_cost(cost_matrix)

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
