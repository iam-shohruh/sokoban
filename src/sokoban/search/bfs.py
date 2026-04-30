from collections import deque
from sokoban.state import Level, State
from sokoban.env import SokobanEnv

def bfs_solver(level: Level) -> dict:
    env = SokobanEnv(level)
    start_state = level.state
    queue = deque([(start_state, [])])
    visited = {start_state}
    while queue:
        state, path = queue.popleft()
        
        env.current_state = state

        if state.boxes == level.goals:
            return {"solved": True, "path": path, "moves": len(path)}

        for action in env.get_valid_actions():
            new_state = env.step(action)
            
            if new_state not in visited:
                visited.add(new_state)
                queue.append((new_state, path + [action]))
                
    return {"solved": False, "path": [], "moves": 0}