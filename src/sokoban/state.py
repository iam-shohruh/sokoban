"""
State representation for Sokoban game is split into two classes:
- `State` - represents the dynamic components
    - player position - Tuple[Position]
    - box positions - ForzenSet[Position]
- `Level` - represents the static components
    - game_id - int
    - width - int
    - height - int
    - walls - FrozenSet[Position]
    - goals - FrozenSet[Position]
"""

from dataclasses import dataclass
from typing import NamedTuple

class Position(NamedTuple):
    row: int
    col: int

class Push(NamedTuple):
    position: Position
    direction: str

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