"""
State representation for Sokoban game is split into two classes:
- `State` - represents the state of the game - dynamic components
    - player position - Tuple[Position]
    - box positions - ForzenSet[Position]
- `Level` - represents all game components - static and dynamic
    - game_id - int
    - width - int
    - height - int
    - walls - FrozenSet[Position]
    - goals - FrozenSet[Position]
    - state - State
"""

from dataclasses import dataclass
from typing import Tuple

Position = Tuple[int, int]

@dataclass(frozen=True)
class State:
    player: Position
    boxes: frozenset[Position]

@dataclass(frozen=True)
class Level:
    game_id: int
    width: int
    height: int
    walls: frozenset[Position]
    goals: frozenset[Position]
    state: State