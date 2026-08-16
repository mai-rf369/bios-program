//####################################################################################################
// BIOS-PROGRAM - Physics & Collision Test (CCD & Separated Axis)
//####################################################################################################
//====================================================================================================
// Include
//====================================================================================================
#include "hardware.h"
#include "font8x8.h"
#include "timer.h"
#include "graphic.h"
#include "keycode.h"
#include "acpi.h"
#include "keyboard.h"

#define MAX_DOTS	100
#define TRAIL_LENGTH	10

// 絶対値を求めるマクロ（サブステップ計算用）
#define ABS(x) ((x) < 0 ? -(x) : (x))

// 物理ドット構造体
typedef struct {
	int active;
	int x;      // 256倍の固定小数点
	int y;      // 256倍の固定小数点
	int vx;     // 横方向の速度
	int vy;     // 縦方向の速度
	int trail_x[TRAIL_LENGTH];
	int trail_y[TRAIL_LENGTH];
} Dot;

static unsigned char bg_color_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static unsigned char collision_map[SCREEN_WIDTH * SCREEN_HEIGHT];

static unsigned int rand_seed = 12345;
static int get_rand(void) {
	rand_seed = rand_seed * 1103515245 + 12345;
	return (int)((rand_seed / 65536) % 32768);
}

void put_map_pixel(int x, int y, unsigned char color) {
	if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
		bg_color_buffer[y * SCREEN_WIDTH + x] = color;
		collision_map[y * SCREEN_WIDTH + x] = 1;
	}
}

void draw_map_rect(int x, int y, int w, int h, unsigned char color) {
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			put_map_pixel(x + j, y + i, color);
		}
	}
}

void draw_map_hollow_rect(int x, int y, int w, int h, unsigned char color, int thick) {
	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			if (i < thick || i >= h - thick || j < thick || j >= w - thick) {
				put_map_pixel(x + j, y + i, color);
			}
		}
	}
}

void init_stage(void) {
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		bg_color_buffer[i] = 0;
		collision_map[i] = 0;
	}
	
	draw_map_rect(100, 140, 80, 10, 2); 
	draw_map_rect(30, 100, 50, 10, 3);  
	draw_map_rect(140, 60, 10, 40, 4);  
	
	draw_map_hollow_rect(220, 90, 60, 40, 5, 4); 
	for(int i = 0; i < 4; i++) {
		for(int j = 20; j < 40; j++) {
			bg_color_buffer[(90 + i) * SCREEN_WIDTH + (220 + j)] = 0;
			collision_map[(90 + i) * SCREEN_WIDTH + (220 + j)] = 0;
		}
	}

	for (int i = 0; i < 6; i++) {
		draw_map_rect(180 + i * 15, 180 - i * 10, 15, 20 + i * 10, 6);
	}
}

//====================================================================================================
// Main
//====================================================================================================
void main(void)
{
	unsigned char *vram = (unsigned char *)VRAM;
	unsigned char backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	
	int key_state[128];
	for (int i = 0; i < 128; i++) key_state[i] = 0;
	
	Dot dots[MAX_DOTS];
	for (int i = 0; i < MAX_DOTS; i++) dots[i].active = 0;
	
	init_stage();
	
	int cursor_x = SCREEN_WIDTH / 2;
	int cursor_y = SCREEN_HEIGHT / 4;
	
	int previous_space_key = 0;
	
	while (1)
	{
		process_keyboard(key_state);
		
		if (key_state[KEY_ESC]) system_shutdown();
		
		if (key_state[KEY_LEFT])  cursor_x = cursor_x - 2;
		if (key_state[KEY_RIGHT]) cursor_x = cursor_x + 2;
		if (key_state[KEY_UP])    cursor_y = cursor_y - 2;
		if (key_state[KEY_DOWN])  cursor_y = cursor_y + 2;
		
		if (cursor_x < 0) cursor_x = 0;
		if (cursor_x >= SCREEN_WIDTH) cursor_x = SCREEN_WIDTH - 1;
		if (cursor_y < 0) cursor_y = 0;
		if (cursor_y >= SCREEN_HEIGHT) cursor_y = SCREEN_HEIGHT - 1;
		
		int current_space_key = key_state[KEY_SPACE];
		if (current_space_key && !previous_space_key)
		{
			for (int i = 0; i < MAX_DOTS; i++)
			{
				if (dots[i].active == 0)
				{
					dots[i].active = 1;
					dots[i].x  = cursor_x * 256;
					dots[i].y  = cursor_y * 256;
					// 高速なすり抜けテストのため、初速を少し広げています
					dots[i].vx = (get_rand() % 1200) - 600; 
					dots[i].vy = (get_rand() % 600) - 300; 
					
					for (int t = 0; t < TRAIL_LENGTH; t++) {
						dots[i].trail_x[t] = cursor_x;
						dots[i].trail_y[t] = cursor_y;
					}
					break;
				}
			}
		}
		previous_space_key = current_space_key;
		
		//------------------------------------------------------------------------------------------------
		// 2. Physics & Collision Simulation (Continuous Collision Detection)
		//------------------------------------------------------------------------------------------------
		for (int i = 0; i < MAX_DOTS; i++)
		{
			if (!dots[i].active) continue;
			
			for (int t = TRAIL_LENGTH - 1; t > 0; t--)
			{
				dots[i].trail_x[t] = dots[i].trail_x[t - 1];
				dots[i].trail_y[t] = dots[i].trail_y[t - 1];
			}
			dots[i].trail_x[0] = dots[i].x / 256;
			dots[i].trail_y[0] = dots[i].y / 256;
			
			// 重力の加算
			dots[i].vy = dots[i].vy + 15;
			
			// サブステップの計算
			// 1ピクセル(256)以上動く場合、移動を分割して飛び級を防ぐ
			int steps_x = (ABS(dots[i].vx) / 256) + 1;
			int steps_y = (ABS(dots[i].vy) / 256) + 1;
			int steps = (steps_x > steps_y) ? steps_x : steps_y;
			
			int step_vx = dots[i].vx / steps;
			int step_vy = dots[i].vy / steps;

			// 分割した回数だけ少しずつ移動と判定を繰り返す
			for (int s = 0; s < steps; s++) {
				if (!dots[i].active) break;

				// --- [A] X軸の移動と判定 ---
				dots[i].x += step_vx;
				
				// 画面端判定(X)
				if (dots[i].x <= 0) {
					dots[i].x = 0;
					if (dots[i].vx < 0) { dots[i].vx = -(dots[i].vx * 8) / 10; step_vx = -(step_vx * 8) / 10; }
				} else if (dots[i].x >= (SCREEN_WIDTH - 1) * 256) {
					dots[i].x = (SCREEN_WIDTH - 1) * 256;
					if (dots[i].vx > 0) { dots[i].vx = -(dots[i].vx * 8) / 10; step_vx = -(step_vx * 8) / 10; }
				}

				// マップ判定(X)
				int cx = dots[i].x / 256;
				int cy = dots[i].y / 256;
				if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
					if (collision_map[cy * SCREEN_WIDTH + cx] == 1) {
						if (step_vx > 0) { // 右向きに衝突
							dots[i].x = cx * 256 - 1;
						} else if (step_vx < 0) { // 左向きに衝突
							dots[i].x = (cx + 1) * 256;
						}
						// 速度を反転させ、残りのサブステップも反転した方向に進むようにする
						dots[i].vx = -(dots[i].vx * 8) / 10;
						step_vx = -(step_vx * 8) / 10;
					}
				}

				// --- [B] Y軸の移動と判定 ---
				dots[i].y += step_vy;
				
				// 画面下端判定(Y)
				if (dots[i].y >= (SCREEN_HEIGHT - 1) * 256) {
					dots[i].y = (SCREEN_HEIGHT - 1) * 256;
					if (dots[i].vy > 0) {
						dots[i].vy = -(dots[i].vy * 8) / 10;
						step_vy = -(step_vy * 8) / 10;
						if (dots[i].vy > -40) { dots[i].active = 0; break; }
					}
					// 摩擦
					dots[i].vx = (dots[i].vx * 9) / 10;
					step_vx = (step_vx * 9) / 10;
				}

				// マップ判定(Y)
				cx = dots[i].x / 256;
				cy = dots[i].y / 256;
				if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
					if (collision_map[cy * SCREEN_WIDTH + cx] == 1) {
						if (step_vy > 0) { // 下向きに衝突（床）
							dots[i].y = cy * 256 - 1;
							dots[i].vy = -(dots[i].vy * 8) / 10;
							step_vy = -(step_vy * 8) / 10;
							
							if (dots[i].vy > -40) { dots[i].active = 0; break; }
							
							// 床との摩擦
							dots[i].vx = (dots[i].vx * 9) / 10;
							step_vx = (step_vx * 9) / 10;
						} else if (step_vy < 0) { // 上向きに衝突（天井）
							dots[i].y = (cy + 1) * 256;
							dots[i].vy = -(dots[i].vy * 8) / 10;
							step_vy = -(step_vy * 8) / 10;
						}
					}
				}
			} // サブステップループ終了
		}
		
		//------------------------------------------------------------------------------------------------
		// 3. Draw
		//------------------------------------------------------------------------------------------------
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
			backbuffer[i] = bg_color_buffer[i];
		}
		
		draw_string(4, 4, "Continuous Collision Detection", 15, &font8x8, backbuffer);
		draw_string(4, 14, "ESC: Shutdown", 15, &font8x8, backbuffer);
		draw_string(4, 24, "Arrows: Move", 15, &font8x8, backbuffer);
		draw_string(4, 34, "Space: Drop", 15, &font8x8, backbuffer);
		
		for (int i = 0; i < MAX_DOTS; i++)
		{
			if (!dots[i].active) continue;
			for (int t = 0; t < TRAIL_LENGTH; t++)
			{
				int tx = dots[i].trail_x[t];
				int ty = dots[i].trail_y[t];
				if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT) {
					unsigned char color = 8;
					if (t < 3) color = 15;
					else if (t < 6) color = 7;
					backbuffer[ty * SCREEN_WIDTH + tx] = color;
				}
			}
			int cx = dots[i].x / 256;
			int cy = dots[i].y / 256;
			if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
				backbuffer[cy * SCREEN_WIDTH + cx] = 11;
			}
		}
		
		for (int dx = -2; dx <= 2; dx++) {
			int px = cursor_x + dx;
			int py = cursor_y;
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) backbuffer[py * SCREEN_WIDTH + px] = 14;
		}
		for (int dy = -2; dy <= 2; dy++) {
			int px = cursor_x;
			int py = cursor_y + dy;
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) backbuffer[py * SCREEN_WIDTH + px] = 14;
		}
		
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) vram[i] = backbuffer[i];
		
		delay_ms(16);
	}
}