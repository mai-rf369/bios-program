#include "map.h"
#include "graphic.h"

// マップへのスプライト描画関数（別ファイルへ分割しやすい設計）
void write_sprite_to_map(unsigned char *background_buffer, unsigned char *collision_map, int map_x, int map_y, const Sprite *sprite) {
	for (int y = 0; y < sprite->height; y++) {
		for (int x = 0; x < sprite->width; x++) {
			int px = map_x + x;
			int py = map_y + y;
			
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
				int index = y * sprite->width + x;
				
				// ① 色の描画（0は透明色）
				unsigned char color = sprite->pixels[index];
				if (sprite->overwrites[index] == 1) {
					background_buffer[py * SCREEN_WIDTH + px] = color;
				}
				
				// ② 当たり判定の描画（1なら壁）
				unsigned char col = sprite->collisions[index];
				if (col == 1) {
					collision_map[py * SCREEN_WIDTH + px] = 1;
				}
			}
		}
	}
}
