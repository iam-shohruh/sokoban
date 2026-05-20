import os
import threading

import pygame

from sokoban.state import Level, Position, State

ASSETS_DIR = os.path.join(os.path.dirname(__file__), "..", "assets")
TILE = 64
HUD_H = 88
MIN_WIN_W = 520


class ReplayViewer:
    """
    Interactive pygame window for stepping through A* search states.

    Controls:
      Left / Right  — previous / next step
      Space         — play / pause auto-advance
      R             — restart to step 0
      Up / Down     — speed up / slow down auto-play
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

    # ------------------------------------------------------------------
    # Public entry point
    # ------------------------------------------------------------------

    def run(self) -> None:
        pygame.init()
        win_w = max(self.level.width * TILE, MIN_WIN_W)
        win_h = self.level.height * TILE + HUD_H
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
                    if k in (pygame.K_ESCAPE, pygame.K_q):
                        running = False
                    elif k == pygame.K_RIGHT:
                        self._step(+1)
                    elif k == pygame.K_LEFT:
                        self._step(-1)
                    elif k == pygame.K_SPACE:
                        self.playing = not self.playing
                    elif k == pygame.K_r:
                        self.cursor = 0
                        self.playing = False
                    elif k == pygame.K_UP:
                        self.play_interval = max(50, self.play_interval - 50)
                    elif k == pygame.K_DOWN:
                        self.play_interval = min(2000, self.play_interval + 100)

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

    def _solving(self) -> bool:
        return self.live and self.solving_thread is not None and self.solving_thread.is_alive()

    # ------------------------------------------------------------------
    # Rendering
    # ------------------------------------------------------------------

    def _load_images(self) -> dict:
        def load(name: str):
            path = os.path.join(ASSETS_DIR, name)
            return pygame.transform.scale(
                pygame.image.load(path).convert_alpha(), (TILE, TILE)
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
                x, y = col * TILE, row * TILE
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
        board_h = self.level.height * TILE
        pygame.draw.rect(screen, (18, 18, 36), pygame.Rect(0, board_h, win_w, HUD_H))
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

        # Row 3: key hint
        speed_txt = f"{1000 // self.play_interval} steps/s"
        hint = font_hint.render(
            f"←→ step   Space play/pause   R restart   ↑↓ speed ({speed_txt})   Q quit",
            True, (120, 120, 140),
        )
        screen.blit(hint, (10, board_h + 62))


# ------------------------------------------------------------------
# Convenience functions
# ------------------------------------------------------------------

def visualize_replay(level: Level, steps: list[dict]) -> None:
    """Show an already-collected list of A* steps in the pygame viewer."""
    ReplayViewer(level, steps).run()


def visualize_live(level: Level) -> None:
    """Run A* in a background thread and watch the search live in pygame."""
    from sokoban.search.a_star import a_star_solver

    steps: list[dict] = []

    def _solve():
        a_star_solver(level, steps_out=steps)

    thread = threading.Thread(target=_solve, daemon=True)
    thread.start()
    ReplayViewer(level, steps, live=True, solving_thread=thread).run()

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