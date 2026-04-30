"""
This file contains the Sokoban environment implementation
"""

from sokoban.state import Level, State
#define level and state
class SokobanEnv:

    def __init__(self, level: Level):
        self.level = level
        self.current_state = level.state

    def step(self, action: str):
        
        moves = {
            "UP": (-1,0), "DOWN": (1,0), "LEFT": (0,-1), "RIGHT": (0,1)
        }
        dy,dx = moves[action]


#       update player position
        
        player_y, player_x = self.current_state.player
        next_p = (player_y + dy, player_x + dx)
       
#       wall check  
 
        if next_p in self.level.walls:
            return self.current_state
        
        state_boxes = set(self.current_state.boxes)

#       if player went to box, push box        

        if next_p in state_boxes:
            box_next_p = (next_p[0] + dy, next_p[1] + dx)

#       if box went to a wall or another box, reset state      

        if box_next_p in self.level.walls or box_next_p in state_boxes:
            return self.current_state

#       update new box state by removing old box       

        state_boxes.remove(next_p)
        state_boxes.add(box_next_p)

#       update state

        self.current_state = State(player = next_p, boxes = frozenset(state_boxes))
        return self.current_state
    
    def get_valid_actions(self):
        valid_moves = []
        moves = {"UP": (-1, 0), "DOWN": (1, 0), "LEFT": (0, -1), "RIGHT": (0, 1)}
        
        player_y, player_x = self.current_state.player
        state_boxes = self.current_state.boxes

        for action, (dy, dx) in moves.items():
            next_p = (player_y + dy, player_x + dx)

#           if player step into a wall, the direction isnt valid 
            if next_p in self.level.walls:
                continue

            if next_p in state_boxes:
                box_next_p = (next_p[0] + dy, next_p[1] + dx)
                
#               if the box is pushed into a wall or another box, the direction isnt valid
                if box_next_p in self.level.walls or box_next_p in state_boxes:
                    continue
            
#           direction passed test, add to valid moves array
            valid_moves.append(action)
            
        return valid_moves