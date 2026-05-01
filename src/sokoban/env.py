"""
This file contains the Sokoban environment implementation
"""

from collections import deque

from sokoban.state import Level, Position, Push, State

class SokobanRules:
    def __init__(self):
        self.moves = {"UP": (-1,0), "DOWN": (1,0), "LEFT": (0,-1), "RIGHT": (0,1)}

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

    def get_valid_pushes(self, state: State, level: Level) -> list[Push]:
        """
        Returns a list of valid pushes that can be applied to the given state.

        Args:
            state (State): The current state of the game.
            level (Level): The level of the game, containing the static components such as walls and goals.

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
                if next_pos in level.walls or next_pos in visited:
                    continue
                elif next_pos in boxes:
                    box_next_pos = Position(next_pos[0]+dy, next_pos[1]+dx)
                    if box_next_pos not in level.walls and box_next_pos not in boxes:
                        valid_pushes.append(Push(next_pos, direction))
                else:
                    visited.add(next_pos)
                    queue.append(next_pos)
        
        return valid_pushes
    
    def is_goal_state(self, state: State, level: Level) -> bool:
        """
        Checks if the given state is a goal state.

        Args:
            state (State): The current state of the game.
            level (Level): The level of the game, containing the static components such as walls and goals.
        Returns:
            bool: True if the given state is a goal state, False otherwise.
        """
        return all(box in level.goals for box in state.boxes)

class SokobanEnv():
    def __init__(self, level: Level):
        self.rules = SokobanRules()
        self.level = level

    def reset(self) -> State:
        return self.level.init_state
    
    def step(self, state: State, push: Push) -> State:
        return self.rules.step(state, push)
    
    def get_valid_pushes(self, state: State) -> list[Push]:
        return self.rules.get_valid_pushes(state, self.level)
    
    def is_goal_state(self, state: State) -> bool:
        return self.rules.is_goal_state(state, self.level)
    