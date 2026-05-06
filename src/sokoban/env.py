"""
This file contains the Sokoban environment implementation
"""

from collections import deque

from sokoban.state import Level, Position, Push, State

class SokobanEnv:
    def __init__(self, level: Level):
        self.level = level
        self.init_state = level.init_state
        self.point_to_goal: dict[Position, dict[Position, int]] = self.precompute_point_to_goal()
        self.moves = {"UP": (-1,0), "DOWN": (1,0), "LEFT": (0,-1), "RIGHT": (0,1)}

    def precompute_point_to_goal(self) -> dict[Position, dict[Position, int]]:
        """
        Precomputes BFS distances from every non-wall cell to every goal.
        Runs one BFS per goal (outward from goal), which is O(goals × cells)
        instead of the naive O(cells × goals × cells).

        Returns:
            dict[Position, dict[Position, int]]: Maps each position to a dict of
            {goal: distance}. Unreachable goals are omitted.
        """
        point_to_goal: dict[Position, dict[Position, int]] = {}
        for row in range(self.level.height):
            for col in range(self.level.width):
                pos = Position(row, col)
                if pos not in self.level.walls:
                    point_to_goal[pos] = {}

        for goal in self.level.goals:
            queue = deque([(goal, 0)])
            visited = {goal}
            while queue:
                pos, dist = queue.popleft()
                if pos in point_to_goal:
                    point_to_goal[pos][goal] = dist
                for dy, dx in self.rules.moves.values():
                    nxt = Position(pos[0] + dy, pos[1] + dx)
                    if nxt not in self.level.walls and nxt not in visited:
                        visited.add(nxt)
                        queue.append((nxt, dist + 1))

        return point_to_goal
    
    def reset(self) -> State:
        return self.level.init_state

    def step(self, state: State, push: Push) -> State:
        """
        Applies the given push to the state and return the resulting state
        
        Args:
            state (State): The current state of the game.
            push (Push): A tuple containing the position of the box to be pushed and the direction of the push.
        Returns:
            State: The resulting state after applying the push.
        """

        boxes = set(state.boxes)
        box_pos, direction = push
        
        # assign the new position of the box and the player after the push
        new_box_pos = Position(box_pos[0] + self.moves[direction][0], box_pos[1] + self.moves[direction][1])
        new_player_pos = box_pos

        # update the state by removing the old box position and adding the new box position
        boxes.remove(box_pos)
        boxes.add(new_box_pos)

        return State(player=new_player_pos, boxes=frozenset(boxes))


    def is_deadlock(self, state: State) -> bool:
        for box_pos in state.boxes:
            if self.is_pos_deadlock(box_pos):
                return True
        return False

    def is_pos_deadlock(self, new_box_pos: Position) -> bool:
        """
        Checks if moving a box to new_box_pos results in a corner deadlock state.
        This is a foolproof heuristic that ignores freestanding pillars.
        """
        # If the box reaches a goal, it is not deadlocked.
        if new_box_pos in self.level.goals:
            return False

        y, x = new_box_pos[0], new_box_pos[1]
        
        horz_blocked = False
        vert_blocked = False

        # Check all 4 directions using moves offsets.
        for direction, (dy, dx) in self.moves.items():
            if Position(y + dy, x + dx) in self.level.walls:
                if direction in ["UP", "DOWN"]:
                    vert_blocked = True
                elif direction in ["LEFT", "RIGHT"]:
                    horz_blocked = True

        # SAFE DEADLOCK CHECK: Corner or 3-side trap.
        if horz_blocked and vert_blocked:
            return True

        return False


    def get_valid_pushes(self, state: State) -> list[Push]:
        """
        Returns a list of valid pushes that can be applied to the given state.

        Args:
            state (State): The current state of the game.

        Returns:
            list[Push]: A list of valid pushes that can be applied to the given state.
        """
        valid_pushes: list[Push] = []
        player = state.player
        boxes = state.boxes

        visited = set({player})
        queue = [player]

        while queue:
            current_pos = queue.pop()

            for direction, (dy, dx) in self.moves.items():
                next_pos = Position(current_pos[0]+dy, current_pos[1]+dx)
                if next_pos in self.level.walls or next_pos in visited:
                    continue
                elif next_pos in boxes:
                    box_next_pos =Position(next_pos[0]+dy, next_pos[1]+dx)
                    if box_next_pos not in self.level.walls and box_next_pos not in boxes:
                        valid_pushes.append(Push(next_pos, direction))
                else:
                    visited.add(next_pos)
                    queue.append(next_pos)
        
        return valid_pushes
    
    def is_goal_state(self, state: State) -> bool:
        """
        Checks if the given state is a goal state.

        Args:
            state (State): The current state of the game.
        Returns:
            bool: True if the given state is a goal state, False otherwise.
        """
        return all(box in self.level.goals for box in state.boxes)
    
    def normalize_player(self, state: State) -> State:
        """
        Normalizes the player position in the state by moving it to the closest reachable position to any goal.
        This is used to reduce the state space by treating states with the same box configuration but different player positions as equivalent.

        Args:
            state (State): The current state of the game.
        Returns:
            State: A new state with the player position normalized.
        """
        
        reachable = {state.player}
        stack = [state.player]
        while stack:
            pos = stack.pop()
            for dy, dx in self.moves.values():
                nxt = Position(pos[0] + dy, pos[1] + dx)
                if nxt not in self.level.walls and nxt not in state.boxes and nxt not in reachable:
                    reachable.add(nxt)
                    stack.append(nxt)
        return State(player=list(reachable)[0], boxes=state.boxes)
    