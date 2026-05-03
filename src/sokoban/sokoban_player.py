import pygame
import sys
import copy
import os

ASSETS_DIR = os.path.join(os.path.dirname(__file__), "assets")

TILE_SIZE = 64

# --- 1. Multiple Levels Data ---
LEVEL_1 = [
    [1, 1, 1, 1, 1, 1, 1],
    [1, 0, 4, 0, 1, 1, 1],
    [1, 0, 3, 3, 4, 0, 1],
    [1, 2, 0, 0, 0, 0, 1],
    [1, 1, 1, 1, 1, 1, 1]
]

LEVEL_2 = [
    [1, 1, 1, 1, 1, 1, 1, 1],
    [1, 0, 0, 1, 0, 4, 0, 1],
    [1, 2, 3, 0, 3, 4, 0, 1],
    [1, 0, 0, 1, 0, 0, 0, 1],
    [1, 1, 1, 1, 1, 1, 1, 1]
]


def main():
    pygame.init()

    # Set window size to fit the largest map
    GRID_WIDTH = 8 * TILE_SIZE
    GRID_HEIGHT = 5 * TILE_SIZE
    screen = pygame.display.set_mode((GRID_WIDTH, GRID_HEIGHT))
    pygame.display.set_caption("Sokoban Project - UI Flow")

    # --- 2. Load Images ---
    floor_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "ground_04.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))
    wall_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "block_06.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))
    player_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "player_03.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))
    box_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "crate_07.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))
    goal_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "environment_09.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))
    box_on_goal_img = pygame.transform.scale(pygame.image.load(os.path.join(ASSETS_DIR, "crate_27.png")).convert_alpha(), (TILE_SIZE, TILE_SIZE))

    # --- 3. Setup Fonts and Buttons ---
    font_title = pygame.font.SysFont(None, 64)
    font_btn = pygame.font.SysFont(None, 40)

    # Create interactive areas (Rectangles) for buttons to easily capture mouse clicks
    btn_start_rect = pygame.Rect(GRID_WIDTH//2 - 100, GRID_HEIGHT//2, 200, 50)
    btn_lvl1_rect = pygame.Rect(GRID_WIDTH//2 - 100, 100, 200, 50)
    btn_lvl2_rect = pygame.Rect(GRID_WIDTH//2 - 100, 180, 200, 50)
    btn_back_rect = pygame.Rect(20, 20, 100, 40)  # Back button is fixed at the top left

    # --- 4. Screen Rendering Functions ---
    def draw_button(rect, text, bg_color=(70, 130, 180), text_color=(255, 255, 255)):
        pygame.draw.rect(screen, bg_color, rect, border_radius=10)
        text_surf = font_btn.render(text, True, text_color)
        text_rect = text_surf.get_rect(center=rect.center)
        screen.blit(text_surf, text_rect)

    def draw_main_menu():
        screen.fill((40, 44, 52))
        title_text = font_title.render("SOKOBAN", True, (255, 215, 0))
        screen.blit(title_text, title_text.get_rect(center=(GRID_WIDTH // 2, GRID_HEIGHT // 3)))
        draw_button(btn_start_rect, "Start Game")

    def draw_level_selection():
        screen.fill((40, 44, 52))
        title_text = font_title.render("Select Level", True, (255, 215, 0))
        screen.blit(title_text, title_text.get_rect(center=(GRID_WIDTH // 2, 50)))

        draw_button(btn_lvl1_rect, "Level 1")
        draw_button(btn_lvl2_rect, "Level 2")
        draw_button(btn_back_rect, "Back", bg_color=(200, 50, 50))  # Red back button

    def draw_grid(level_matrix):
        # Draw the map
        for y, row in enumerate(level_matrix):
            for x, tile in enumerate(row):
                pos_x = x * TILE_SIZE
                pos_y = y * TILE_SIZE
                screen.blit(floor_img, (pos_x, pos_y))
                if tile == 1: screen.blit(wall_img, (pos_x, pos_y))
                elif tile == 2: screen.blit(player_img, (pos_x, pos_y))
                elif tile == 3: screen.blit(box_img, (pos_x, pos_y))
                elif tile == 4: screen.blit(goal_img, (pos_x, pos_y))
                elif tile == 5:
                    screen.blit(goal_img, (pos_x, pos_y))
                    screen.blit(box_on_goal_img, (pos_x, pos_y))
                elif tile == 6:
                    screen.blit(goal_img, (pos_x, pos_y))
                    screen.blit(player_img, (pos_x, pos_y))

        # Draw the back button over the map in the corner
        draw_button(btn_back_rect, "Back", bg_color=(200, 50, 50))

    # --- 5. Main Game Loop & Event Handling ---
    clock = pygame.time.Clock()
    running = True

    current_screen = "MENU"
    current_level_data = []  # Variable to hold the matrix of the selected level

    while running:
        for event in pygame.event.get():
            if event.type == pygame.QUIT:
                running = False

            # Capture mouse clicks
            elif event.type == pygame.MOUSEBUTTONDOWN:
                mouse_pos = event.pos

                if current_screen == "MENU":
                    if btn_start_rect.collidepoint(mouse_pos):
                        current_screen = "LEVELS"

                elif current_screen == "LEVELS":
                    if btn_lvl1_rect.collidepoint(mouse_pos):
                        current_level_data = copy.deepcopy(LEVEL_1)  # Take a fresh copy of the level
                        current_screen = "GAME"
                    elif btn_lvl2_rect.collidepoint(mouse_pos):
                        current_level_data = copy.deepcopy(LEVEL_2)
                        current_screen = "GAME"
                    elif btn_back_rect.collidepoint(mouse_pos):
                        current_screen = "MENU"

                elif current_screen == "GAME":
                    if btn_back_rect.collidepoint(mouse_pos):
                        current_screen = "LEVELS"  # Return to the level selection page

            # Capture movement keys (only works inside the GAME screen)
            elif event.type == pygame.KEYDOWN and current_screen == "GAME":
                pass

        screen.fill((0, 0, 0))

        # Display the appropriate screen based on the state variable
        if current_screen == "MENU":
            draw_main_menu()
        elif current_screen == "LEVELS":
            draw_level_selection()
        elif current_screen == "GAME":
            draw_grid(current_level_data)

        pygame.display.flip()
        clock.tick(60)

    pygame.quit()
    sys.exit()


if __name__ == "__main__":
    main()
