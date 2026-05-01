from collections import deque
from sokoban.state import Level, Push, State
from sokoban.env import SokobanEnv

def bfs_solver(level: Level) -> dict:
    env = SokobanEnv(level)
    start_state = level.init_state

    queue = deque([start_state])
    visited = set([start_state])
    parent: dict[State, tuple[State, Push] | None] = {start_state: None}

    while queue:
        state = queue.popleft()

        if env.is_goal_state(state):
            actions = []
            cur = state
            while parent[cur] is not None:
                prev, action = parent[cur]
                actions.append(action)
                cur = prev
            print(f"Solved {level.game_id} in {len(actions)} moves")
            return {"solved": True, "path": actions[::-1]}

        for action in env.get_valid_pushes(state):
            new_state = env.step(state, action)

            if new_state not in visited:
                visited.add(new_state)
                queue.append(new_state)
                parent[new_state] = (state, action)
    print(f"Failed to solve {level.game_id}")
    return {"solved": False, "path": []}