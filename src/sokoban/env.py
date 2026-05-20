"""
Sokoban environment with movement, successor generation and conservative deadlocks.
"""

from collections import deque

from sokoban.state import Level, Position, Push, State


class SokobanEnv:
    def __init__(self, level: Level, debug_deadlocks: bool = False):
        self.level = level
        self.moves: dict[str, tuple[int, int]] = {
            "UP": (-1, 0),
            "DOWN": (1, 0),
            "LEFT": (0, -1),
            "RIGHT": (0, 1),
        }
        self.debug_deadlocks = debug_deadlocks
        self.deadlock_stats: dict[str, int] = {
            "corner": 0,
            "dead_square": 0,
            "2x2": 0,
        }

        self.point_to_goal = self.precompute_point_to_goal()

    # ------------------------------------------------------------------
    # Basic helpers
    # ------------------------------------------------------------------

    def _record_deadlock(self, name: str) -> None:
        if self.debug_deadlocks:
            self.deadlock_stats[name] += 1

    def _is_floor(self, pos: Position) -> bool:
        return (
            0 <= pos.row < self.level.height
            and 0 <= pos.col < self.level.width
            and pos not in self.level.walls
        )

    def _neighbors(self, pos: Position) -> tuple[Position, Position, Position, Position]:
        return (
            Position(pos.row - 1, pos.col),
            Position(pos.row + 1, pos.col),
            Position(pos.row, pos.col - 1),
            Position(pos.row, pos.col + 1),
        )

    def is_goal_state(self, state: State) -> bool:
        return all(box in self.level.goals for box in state.boxes)

    # ------------------------------------------------------------------
    # Reachability / movement
    # ------------------------------------------------------------------

    def _reachable_floor(self, state: State) -> set[Position]:
        reachable = {state.player}
        queue = deque([state.player])
        while queue:
            pos = queue.popleft()
            for nxt in self._neighbors(pos):
                if nxt in reachable or nxt in self.level.walls or nxt in state.boxes:
                    continue
                reachable.add(nxt)
                queue.append(nxt)
        return reachable

    def normalize_player(self, state: State) -> State:
        reachable = self._reachable_floor(state)
        return State(player=min(reachable), boxes=state.boxes)

    def step(self, state: State, push: Push) -> State:
        box_pos, direction = push
        dy, dx = self.moves[direction]
        new_box_pos = Position(box_pos.row + dy, box_pos.col + dx)
        boxes = set(state.boxes)
        boxes.remove(box_pos)
        boxes.add(new_box_pos)
        # after a push, player stands where the pushed box used to be
        return State(player=box_pos, boxes=frozenset(boxes))

    def _valid_pushes_from_reachable(self, reachable: set[Position], boxes: frozenset[Position]) -> list[Push]:
        pushes: list[Push] = []
        for p in reachable:
            for direction, (dy, dx) in self.moves.items():
                box_pos = Position(p.row + dy, p.col + dx)
                if box_pos not in boxes:
                    continue
                dst = Position(box_pos.row + dy, box_pos.col + dx)
                if self._is_floor(dst) and dst not in boxes:
                    pushes.append(Push(box_pos, direction))
        return pushes

    def get_valid_pushes(self, state: State) -> list[Push]:
        return self._valid_pushes_from_reachable(self._reachable_floor(state), state.boxes)

    def get_successors(self, state: State, use_macro: bool = False) -> list[tuple[State, tuple[Push, ...]]]:
        successors: list[tuple[State, tuple[Push, ...]]] = []
        for push in self.get_valid_pushes(state):
            nxt = self.normalize_player(self.step(state, push))
            successors.append((nxt, (push,)))
        return successors

    # ------------------------------------------------------------------
    # Heuristic support precompute
    # ------------------------------------------------------------------

    def precompute_point_to_goal(self) -> dict[Position, dict[Position, int]]:
        point_to_goal: dict[Position, dict[Position, int]] = {
            Position(r, c): {}
            for r in range(self.level.height)
            for c in range(self.level.width)
            if Position(r, c) not in self.level.walls
        }

        for goal in self.level.goals:
            queue = deque([(goal, 0)])
            seen = {goal}
            while queue:
                pos, dist = queue.popleft()
                if pos in point_to_goal:
                    point_to_goal[pos][goal] = dist

                # reverse-push graph
                for dy, dx in self.moves.values():
                    prev_box = Position(pos.row - dy, pos.col - dx)
                    prev_player = Position(pos.row - 2 * dy, pos.col - 2 * dx)
                    if (
                        prev_box not in seen
                        and prev_box not in self.level.walls
                        and prev_player not in self.level.walls
                    ):
                        seen.add(prev_box)
                        queue.append((prev_box, dist + 1))
        return point_to_goal

    # ------------------------------------------------------------------
    # Deadlocks
    # ------------------------------------------------------------------

    def is_deadlock(self, state: State) -> bool:
        checks = (
            ("corner", self.has_corner_deadlock),
            ("dead_square", self.has_dead_square_deadlock),
            ("2x2", self.has_2x2_deadlock),
        )
        for name, fn in checks:
            if fn(state):
                self._record_deadlock(name)
                return True
        return False

    def has_corner_deadlock(self, state: State) -> bool:
        walls = self.level.walls
        goals = self.level.goals
        for box in state.boxes:
            if box in goals:
                continue
            up = Position(box.row - 1, box.col)
            down = Position(box.row + 1, box.col)
            left = Position(box.row, box.col - 1)
            right = Position(box.row, box.col + 1)
            if (up in walls and left in walls) or (up in walls and right in walls):
                return True
            if (down in walls and left in walls) or (down in walls and right in walls):
                return True
        return False

    def has_dead_square_deadlock(self, state: State) -> bool:
        for box in state.boxes:
            if box not in self.level.goals and not self.point_to_goal.get(box):
                return True
        return False

    def has_2x2_deadlock(self, state: State) -> bool:
        blocked = set(state.boxes)
        blocked.update(self.level.walls)
        boxes = state.boxes
        goals = self.level.goals
        for r in range(self.level.height - 1):
            for c in range(self.level.width - 1):
                p00 = Position(r, c)
                p10 = Position(r + 1, c)
                p01 = Position(r, c + 1)
                p11 = Position(r + 1, c + 1)
                if p00 in blocked and p10 in blocked and p01 in blocked and p11 in blocked:
                    block_boxes = [p for p in (p00, p10, p01, p11) if p in boxes]
                    if not block_boxes:
                        continue
                    if all(p not in goals for p in block_boxes):
                        return True
        return False
