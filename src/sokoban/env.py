"""
This file contains the Sokoban environment implementation
"""

from collections import deque

from sokoban.state import Level, Position, Push, State

class SokobanEnv:
    def __init__(self, level: Level):
        self.level = level
        self.init_state = level.init_state
        self.moves = {"UP": (-1,0), "DOWN": (1,0), "LEFT": (0,-1), "RIGHT": (0,1)}
        self.point_to_goal: dict[Position, dict[Position, int]] = self.precompute_point_to_goal()

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
                for dy, dx in self.moves.values():
                    prev_box = Position(pos[0] - dy, pos[1] - dx)
                    player_pos = Position(pos[0] - 2*dy, pos[1] - 2*dx)
                    if (prev_box not in self.level.walls and player_pos not in self.level.walls and prev_box not in visited):
                        visited.add(prev_box)
                        queue.append((prev_box, dist + 1))

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
        """
        Checks if the given state is a deadlock state.

        Args:
            state (State): The current state of the game.
        Returns:
            bool: True if the given state is a deadlock state, False otherwise.
        """
        return (
            self.has_corner_deadlock(state)
            or self.has_dead_square_deadlock(state)
            or self.has_2x2_deadlock(state)
            # or self.has_freeze_deadlock(state)
            # or self.has_matching_deadlock(state)
        )
    
    def has_corner_deadlock(self, state: State) -> bool:
        """
        Checks for deadlocks where a box is in a corner formed by two walls and is not on a goal.
        """
        for box in state.boxes:
            if box not in self.level.goals:
                for dy1, dx1 in self.moves.values():
                    for dy2, dx2 in self.moves.values():
                        if (dy1, dx1) != (dy2, dx2) and (dy1, dx1) != (-dy2, -dx2):
                            adj1 = Position(box[0] + dy1, box[1] + dx1)
                            adj2 = Position(box[0] + dy2, box[1] + dx2)
                            if adj1 in self.level.walls and adj2 in self.level.walls:
                                return True
        return False
    
    def has_dead_square_deadlock(self, state: State) -> bool:
        """
        Checks for box positions that can't reach any goals.
        """
        for box in state.boxes:
            if box not in self.level.goals and not self.point_to_goal.get(box):
                return True
        return False
    
    def has_2x2_deadlock(self, state: State) -> bool:
        """
        Checks for 2x2 blocks of boxes/walls that can't be moved further.
        """
        for box in state.boxes:
            if box not in self.level.goals:
                for dy1, dx1 in self.moves.values():
                    for dy2, dx2 in self.moves.values():
                        if (dy1, dx1) != (dy2, dx2) and (dy1, dx1) != (-dy2, -dx2):
                            adj1 = Position(box[0] + dy1, box[1] + dx1)
                            adj2 = Position(box[0] + dy2, box[1] + dx2)
                            diag = Position(box[0] + dy1 + dy2, box[1] + dx1 + dx2)
                            if ((adj1 in self.level.walls or adj1 in state.boxes) and
                                (adj2 in self.level.walls or adj2 in state.boxes) and
                                (diag in self.level.walls or diag in state.boxes)):
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
        return State(player=min(reachable), boxes=state.boxes)
    