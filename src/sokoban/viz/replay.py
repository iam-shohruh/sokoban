import os
import threading

import pygame

from sokoban.env import SokobanEnv
from sokoban.state import Level, Position, State

ASSETS_DIR = os.path.join(os.path.dirname(__file__), "..", "assets")
TILE = 64
HUD_H = 170
MIN_WIN_W = 520
MIN_TILE = 18
MAX_SCREEN_FILL = 0.92


class ReplayViewer:
    """
    Interactive pygame window for stepping through A* search states.

    Controls:
      Left / Right  — previous / next step
      Shift+Left/Right — jump 100 steps
      PageUp/PageDown  — jump 1000 steps
      Home / End   — first / latest available step
      [ / ]        — previous / next group
      1 / 2 / 3    — group mode: f-layer / g-layer / h-plateau
      Space         — play / pause auto-advance
      R             — restart to step 0
      Up / Down     — speed up / slow down auto-play
      Click timeline — jump to step
      Q / Escape    — quit
    """

    def __init__(
        self,
        level: Level,
        steps: list[dict],
        live: bool = False,
        solving_thread: threading.Thread | None = None,
    ):
        self.level = level
        self.steps = steps          # shared list; may grow while live=True
        self.live = live
        self.solving_thread = solving_thread
        self.cursor = 0
        self.playing = False
        self.play_interval = 200    # ms between auto-advance steps
        self._last_tick = 0
        self.group_mode = "f"
        self.groups: list[tuple[int, int]] = []
        self.cursor_group_idx = 0
        self._group_cache_len = -1
        self._group_cache_mode = ""
        self._timeline_rect = pygame.Rect(0, 0, 0, 0)
        self.tile = TILE
        self.hud_h = HUD_H

    # ------------------------------------------------------------------
    # Public entry point
    # ------------------------------------------------------------------

    def run(self) -> None:
        pygame.init()
        display_info = pygame.display.Info()
        win_w, win_h = self._compute_layout(
            display_info.current_w, display_info.current_h
        )
        screen = pygame.display.set_mode((win_w, win_h))
        pygame.display.set_caption("A* Replay Viewer")

        imgs = self._load_images()
        font = pygame.font.SysFont(None, 22)
        font_hint = pygame.font.SysFont(None, 18)
        clock = pygame.time.Clock()

        running = True
        while running:
            now = pygame.time.get_ticks()

            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.KEYDOWN:
                    k = event.key
                    mods = pygame.key.get_mods()
                    shift_down = bool(mods & pygame.KMOD_SHIFT)
                    if k in (pygame.K_ESCAPE, pygame.K_q):
                        running = False
                    elif k == pygame.K_RIGHT:
                        self._step(+100 if shift_down else +1)
                    elif k == pygame.K_LEFT:
                        self._step(-100 if shift_down else -1)
                    elif k == pygame.K_PAGEUP:
                        self._step(-1000)
                    elif k == pygame.K_PAGEDOWN:
                        self._step(+1000)
                    elif k == pygame.K_HOME:
                        self._go_to(0)
                        self.playing = False
                    elif k == pygame.K_END:
                        self._go_to(max(0, len(self.steps) - 1))
                        self.playing = False
                    elif k == pygame.K_LEFTBRACKET:
                        self._jump_group(-1)
                    elif k == pygame.K_RIGHTBRACKET:
                        self._jump_group(+1)
                    elif k == pygame.K_1:
                        self.group_mode = "f"
                    elif k == pygame.K_2:
                        self.group_mode = "g"
                    elif k == pygame.K_3:
                        self.group_mode = "plateau"
                    elif k == pygame.K_SPACE:
                        self.playing = not self.playing
                    elif k == pygame.K_r:
                        self.cursor = 0
                        self.playing = False
                    elif k == pygame.K_UP:
                        self.play_interval = max(50, self.play_interval - 50)
                    elif k == pygame.K_DOWN:
                        self.play_interval = min(2000, self.play_interval + 100)
                elif event.type == pygame.MOUSEBUTTONDOWN and event.button == 1:
                    self._handle_timeline_click(event.pos)

            if self.playing and now - self._last_tick >= self.play_interval:
                self._step(+1)
                self._last_tick = now

            screen.fill((30, 30, 30))
            self._draw_board(screen, imgs)
            self._draw_hud(screen, font, font_hint, win_w)
            pygame.display.flip()
            clock.tick(60)

        pygame.quit()

    # ------------------------------------------------------------------
    # Navigation
    # ------------------------------------------------------------------

    def _compute_layout(self, screen_w: int, screen_h: int) -> tuple[int, int]:
        # Fit board+HUD into the screen to avoid needing fullscreen.
        usable_w = max(200, int(screen_w * MAX_SCREEN_FILL))
        usable_h = max(200, int(screen_h * MAX_SCREEN_FILL))
        tile_by_w = max(1, usable_w // max(1, self.level.width))
        tile_by_h = max(1, (usable_h - 120) // max(1, self.level.height))
        tile = max(MIN_TILE, min(TILE, tile_by_w, tile_by_h))

        board_w = self.level.width * tile
        board_h = self.level.height * tile
        hud_h = min(HUD_H, usable_h - board_h)
        if hud_h < 92:
            hud_h = 92

        self.tile = tile
        self.hud_h = hud_h
        return max(board_w, MIN_WIN_W), board_h + hud_h

    def _step(self, delta: int) -> None:
        total = len(self.steps)
        if total == 0:
            return
        new = self.cursor + delta
        if 0 <= new < total:
            self.cursor = new
        elif new >= total and self.live and self._solving():
            pass  # wait; solver hasn't produced more steps yet
        elif new >= total:
            self.playing = False  # reached the end
            self.cursor = total - 1

    def _go_to(self, idx: int) -> None:
        if not self.steps:
            self.cursor = 0
            return
        self.cursor = max(0, min(idx, len(self.steps) - 1))

    def _refresh_groups(self) -> None:
        total = len(self.steps)
        if total == self._group_cache_len and self.group_mode == self._group_cache_mode:
            self._sync_cursor_group_idx()
            return

        self.groups = []
        if total == 0:
            self.cursor_group_idx = 0
            self._group_cache_len = 0
            self._group_cache_mode = self.group_mode
            return

        start = 0
        if self.group_mode in ("f", "g"):
            field = self.group_mode
            prev = self.steps[0].get(field, 0)
            for i in range(1, total):
                cur = self.steps[i].get(field, 0)
                if cur != prev:
                    self.groups.append((start, i - 1))
                    start = i
                    prev = cur
            self.groups.append((start, total - 1))
        else:
            best_h = self.steps[0].get("h", 0)
            for i in range(1, total):
                h = self.steps[i].get("h", 0)
                if h < best_h:
                    self.groups.append((start, i - 1))
                    start = i
                    best_h = h
            self.groups.append((start, total - 1))

        self._group_cache_len = total
        self._group_cache_mode = self.group_mode
        self._sync_cursor_group_idx()

    def _sync_cursor_group_idx(self) -> None:
        if not self.groups:
            self.cursor_group_idx = 0
            return
        for i, (a, b) in enumerate(self.groups):
            if a <= self.cursor <= b:
                self.cursor_group_idx = i
                return
        self.cursor_group_idx = len(self.groups) - 1

    def _jump_group(self, delta: int) -> None:
        self._refresh_groups()
        if not self.groups:
            return
        idx = max(0, min(self.cursor_group_idx + delta, len(self.groups) - 1))
        self.cursor_group_idx = idx
        self.cursor = self.groups[idx][0]
        self.playing = False

    def _handle_timeline_click(self, pos: tuple[int, int]) -> None:
        if not self._timeline_rect.collidepoint(pos):
            return
        total = len(self.steps)
        if total == 0:
            return
        rel = (pos[0] - self._timeline_rect.x) / max(1, self._timeline_rect.width - 1)
        idx = int(rel * (total - 1))
        self._go_to(idx)
        self.playing = False

    def _solving(self) -> bool:
        return self.live and self.solving_thread is not None and self.solving_thread.is_alive()

    # ------------------------------------------------------------------
    # Rendering
    # ------------------------------------------------------------------

    def _load_images(self) -> dict:
        def load(name: str):
            path = os.path.join(ASSETS_DIR, name)
            return pygame.transform.scale(
                pygame.image.load(path).convert_alpha(), (self.tile, self.tile)
            )

        return {
            "floor":    load("ground_04.png"),
            "wall":     load("block_06.png"),
            "player":   load("player_03.png"),
            "box":      load("crate_07.png"),
            "goal":     load("environment_09.png"),
            "box_goal": load("crate_27.png"),
        }

    def _current_state(self) -> State:
        if not self.steps:
            return self.level.init_state
        return self.steps[self.cursor]["state"]

    def _draw_board(self, screen: pygame.Surface, imgs: dict) -> None:
        state = self._current_state()
        for row in range(self.level.height):
            for col in range(self.level.width):
                x, y = col * self.tile, row * self.tile
                pos = Position(row, col)

                screen.blit(imgs["floor"], (x, y))

                if pos in self.level.walls:
                    screen.blit(imgs["wall"], (x, y))
                elif pos in self.level.goals and pos in state.boxes:
                    screen.blit(imgs["goal"], (x, y))
                    screen.blit(imgs["box_goal"], (x, y))
                elif pos in state.boxes:
                    screen.blit(imgs["box"], (x, y))
                elif pos in self.level.goals:
                    screen.blit(imgs["goal"], (x, y))

                if state.player == pos:
                    screen.blit(imgs["player"], (x, y))

    def _draw_hud(
        self, screen: pygame.Surface, font: pygame.font.Font, font_hint: pygame.font.Font, win_w: int
    ) -> None:
        self._refresh_groups()
        board_h = self.level.height * self.tile
        pygame.draw.rect(screen, (18, 18, 36), pygame.Rect(0, board_h, win_w, self.hud_h))
        pygame.draw.line(screen, (60, 60, 100), (0, board_h), (win_w, board_h), 2)

        total = len(self.steps)
        status = "SOLVING..." if self._solving() else "DONE"
        color_status = (255, 200, 50) if self._solving() else (80, 220, 80)

        step = self.steps[self.cursor] if self.steps else None
        if step and step.get("push"):
            push = step["push"]
            push_str = f"{push.direction}  box@{push.position}"
        else:
            push_str = "initial state"

        g = step["g"] if step else 0
        h = step["h"] if step else 0
        f = step["f"] if step else 0

        # Row 1: step counter + status
        txt = font.render(
            f"Step {self.cursor + 1} / {total}",
            True, (220, 220, 220),
        )
        screen.blit(txt, (10, board_h + 8))
        stxt = font.render(status, True, color_status)
        screen.blit(stxt, (10 + txt.get_width() + 12, board_h + 8))

        # Row 2: push + costs
        line2 = font.render(
            f"Push: {push_str:<28}  g={g}  h={h}  f={f}",
            True, (180, 210, 255),
        )
        screen.blit(line2, (10, board_h + 32))

        mode_name = {"f": "f-layer", "g": "g-layer", "plateau": "h-plateau"}[self.group_mode]
        if self.groups:
            a, b = self.groups[self.cursor_group_idx]
            group_txt = f"Group {self.cursor_group_idx + 1}/{len(self.groups)} [{a + 1}..{b + 1}] mode={mode_name}"
        else:
            group_txt = f"Group 0/0 mode={mode_name}"
        gline = font_hint.render(group_txt, True, (170, 200, 170))
        screen.blit(gline, (10, board_h + 56))

        self._draw_timeline(screen, board_h, win_w)

        speed_txt = f"{1000 // self.play_interval} steps/s"
        if self.hud_h >= 150:
            hint = font_hint.render(
                "←→ step  Shift+←→ ±100  PgUp/PgDn ±1000  Home/End ends  [ ] group  1/2/3 mode",
                True, (120, 120, 140),
            )
            screen.blit(hint, (10, board_h + 124))
            hint2 = font_hint.render(
                f"Space play/pause  R restart  ↑↓ speed ({speed_txt})  Click timeline jump  Q quit",
                True, (120, 120, 140),
            )
            screen.blit(hint2, (10, board_h + 146))
        else:
            compact = font_hint.render(
                f"[ ] group  1/2/3 mode  Space play  ↑↓ speed ({speed_txt})  Q quit",
                True, (120, 120, 140),
            )
            screen.blit(compact, (10, board_h + self.hud_h - 20))

    def _draw_timeline(self, screen: pygame.Surface, board_h: int, win_w: int) -> None:
        left = 10
        top = board_h + 78
        width = win_w - 20
        height = 32
        rect = pygame.Rect(left, top, width, height)
        self._timeline_rect = rect
        pygame.draw.rect(screen, (32, 32, 52), rect, border_radius=4)
        pygame.draw.rect(screen, (70, 70, 95), rect, 1, border_radius=4)

        total = len(self.steps)
        if total <= 1:
            return

        if self.group_mode == "g":
            values = [s.get("g", 0) for s in self.steps]
        elif self.group_mode == "plateau":
            values = [s.get("h", 0) for s in self.steps]
        else:
            values = [s.get("f", 0) for s in self.steps]

        min_v = min(values)
        max_v = max(values)
        span = max(1, max_v - min_v)

        for x in range(rect.width):
            idx = int(x * (total - 1) / max(1, rect.width - 1))
            v = values[idx]
            t = (v - min_v) / span
            color = (int(40 + 180 * t), int(160 - 80 * t), int(220 - 120 * t))
            pygame.draw.line(
                screen,
                color,
                (rect.x + x, rect.y + 2),
                (rect.x + x, rect.y + rect.height - 3),
            )

        # Draw current cursor marker.
        cx = rect.x + int(self.cursor * (rect.width - 1) / max(1, total - 1))
        pygame.draw.line(screen, (255, 245, 120), (cx, rect.y), (cx, rect.y + rect.height), 2)


# ------------------------------------------------------------------
# Convenience functions
# ------------------------------------------------------------------

def visualize_replay(level: Level, steps: list[dict]) -> None:
    """Show an already-collected list of A* steps in the pygame viewer."""
    ReplayViewer(level, steps).run()


def visualize_live(level: Level) -> None:
    """Solve with A* and visualize only the final solution path."""
    from sokoban.search.a_star import a_star_solver

    result = a_star_solver(level)
    if not result.get("solved"):
        print("A* did not solve this level. Nothing to replay.")
        return

    env = SokobanEnv(level)
    state = env.normalize_player(level.init_state)
    steps: list[dict] = [{"state": state, "push": None, "g": 0, "h": 0, "f": 0}]

    g = 0
    for push in result["path"]:
        state = env.normalize_player(env.step(state, push))
        g += 1
        steps.append({"state": state, "push": push, "g": g, "h": 0, "f": g})

    ReplayViewer(level, steps).run()

# ------------------------------------------------------------------
# Entrypoint for running this module directly
# ------------------------------------------------------------------

if __name__ == "__main__":
    import argparse
    from sokoban.eval import load_levels

    parser = argparse.ArgumentParser(description="Visualize A* search steps for a Sokoban level.")
    parser.add_argument("--dataset", type=str, default="microban", help="Dataset to load levels from")
    parser.add_argument("--game_id", type=int, default=1, help="Game ID of the level to visualize")
    args = parser.parse_args()

    dataset_paths = {
        "microban": "data/parsed/microban/levels.json",
        "xsokoban": "data/parsed/xsokoban/levels.json",
    }

    if args.dataset not in dataset_paths:
        print(f"Dataset '{args.dataset}' not found. Available datasets: {list(dataset_paths.keys())}")
        exit(1)

    levels = load_levels(dataset_paths[args.dataset])
    level = next((lvl for lvl in levels if lvl.game_id == args.game_id), None)
    if not level:
        print(f"Level with game_id={args.game_id} not found in dataset '{args.dataset}'.")
        exit(1)

    visualize_live(level)   
