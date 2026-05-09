from collections import deque
from sokoban.state import Level, Push, State
from sokoban.env import SokobanEnv

def bfs_solver(level: Level) -> dict:
    env = SokobanEnv(level, debug_deadlocks=True)
    start_state = env.normalize_player(level.init_state)

    queue = deque([start_state])
    visited = {start_state}
    parent: dict[State, tuple[State, Push] | None] = {start_state: None}

    while queue:
        if len(visited) > 1000000: break
        state = queue.popleft()

        if env.is_goal_state(state):
            actions = []
            cur = state
            while parent[cur] is not None:
                prev, action = parent[cur]
                actions.append(action)
                cur = prev
            return {
                "solved": True,
                "path": actions[::-1],
                "explored_states": len(visited),
                "deadlock_stats": dict(env.deadlock_stats),
            }

        for action in env.get_valid_pushes(state):
            new_state = env.normalize_player(env.step(state, action))

            if new_state not in visited and not env.is_deadlock(new_state):
                visited.add(new_state)
                queue.append(new_state)
                parent[new_state] = (state, action)
    return {
        "solved": False,
        "path": [],
        "explored_states": len(visited),
        "deadlock_stats": dict(env.deadlock_stats),
    }
