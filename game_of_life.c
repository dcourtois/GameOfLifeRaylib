#include "raylib.h"

#include <stdlib.h>
#include <stdint.h>
#include <time.h>


static char *	cells				= NULL;
static int		cell_target_size	= 2;
static int		cell_count_w		= 0;
static int		cell_count_h		= 0;
static Vector2	cell_size			= { 0 };
static int		cell_prob			= (int)(50.0 * (double)RAND_MAX / 100.0);


#define ALIVE_MASK_1	(1 << 7)
#define ALIVE_MASK_2	(1 << 6)
#define CELL(x, y)		(cells[mod((y), cell_count_h) * cell_count_w + mod((x), cell_count_w)])


static int
mod(int a, int b)
{
	a = a % b;
	return a < 0 ? a + b : a;
}


static void
init_cells(char alive)
{
	if (cells) {
		free(cells);
	}

	int size = cell_count_w * cell_count_h;
	cells = malloc(size);

	srand((uint32_t)time(0));
	for (char *cell = cells, *end = cells + size; cell != end; ++cell) {
		*cell = rand() < cell_prob ? alive : 0;
	}
}


int
main(int argc, char ** argv)
{
	(void)argc;
	(void)argv;

	InitWindow(800, 500, "Game of Life.");
	SetWindowState(FLAG_WINDOW_RESIZABLE | FLAG_WINDOW_HIGHDPI);
	SetTargetFPS(20);

	bool hud = false;
	bool paused = false;
	int previous_w = 0;
	int previous_h = 0;
	int mask = ALIVE_MASK_1;

	// Enter the main app loop.
	while (WindowShouldClose() == false) {
		BeginDrawing();

		// Events.

		if (IsKeyPressed(KEY_R)) {
			init_cells((char)mask);
		}
		if (IsKeyPressed(KEY_H)) {
			hud = !hud;
		}
		if (IsKeyPressed(KEY_SPACE)) {
			paused = !paused;
		}
		if (IsKeyPressed(KEY_ESCAPE)) {
			exit(0);
		}

		// Resize buffers on first frame or window size changes.

		int w = GetRenderWidth();
		int h = GetRenderHeight();
		if (cells == NULL || previous_w != w || previous_h != h) {
			previous_w = w;
			previous_h = h;
			cell_count_w = w / cell_target_size;
			cell_count_h = h / cell_target_size;
			cell_size.x = (float)w / cell_count_w;
			cell_size.y = (float)h / cell_count_h;
			init_cells((char)mask);
		}

		// Clear screen.

		ClearBackground(DARKGRAY);

		BeginMode2D((Camera2D){ .zoom = 1.0f });

		// Draw cells.

		for (int y = 0; y != cell_count_h; ++y) {
			for (int x = 0; x != cell_count_w; ++x) {
				if (cells[y * cell_count_w + x] & mask) {
					DrawRectangleV((Vector2){ x * cell_size.x, y * cell_size.y }, cell_size, RAYWHITE);
				}
			}
		}

		// Draw HUD

		if (hud) {
			DrawText("Hello", 10, 10, 30, BLACK);
		}

		EndMode2D();

		if (paused == false) {

			// Update the cells.

			int next_mask = mask == ALIVE_MASK_1 ? ALIVE_MASK_2 : ALIVE_MASK_1;

			for (int y = 0; y != cell_count_h; ++y) {
				for (int x = 0; x != cell_count_w; ++x) {
					int neighbors = ((CELL(x - 1, y - 1) & mask) ? 1 : 0) +
									((CELL(x,     y - 1) & mask) ? 1 : 0) +
									((CELL(x + 1, y - 1) & mask) ? 1 : 0) +
									((CELL(x - 1, y    ) & mask) ? 1 : 0) +
									((CELL(x + 1, y    ) & mask) ? 1 : 0) +
									((CELL(x - 1, y + 1) & mask) ? 1 : 0) +
									((CELL(x,     y + 1) & mask) ? 1 : 0) +
									((CELL(x + 1, y + 1) & mask) ? 1 : 0);

					char *cell = &CELL(x, y);
					switch (neighbors) {
						case 0:
						case 1:
							*cell &= ~next_mask;
							break;
						case 2:
							*cell = (*cell & mask) ? *cell | (char)next_mask : *cell & ~(char)next_mask;
							break;
						case 3:
							*cell |= next_mask;
							break;
						default:
							*cell &= ~next_mask;
							break;
					}
				}
			}

			mask = next_mask;

		}

		EndDrawing();
	}

	return 0;
}
