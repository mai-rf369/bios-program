//####################################################################################################
// BIOS-PROGRAM - Physics & Collision Test (Pixel Perfect Sprite & CCD Engine)
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
#include "color.h"

#define MAX_DOTS	100
#define TRAIL_LENGTH	10

// 絶対値を求めるマクロ（サブステップ計算用）
#define ABS(x) ((x) < 0 ? -(x) : (x))

//====================================================================================================
// Data Structures
//====================================================================================================
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

// スプライト定義の構造体（色と当たり判定を分離）
typedef struct {
	int width;
	int height;
	const unsigned char *pixels;     // 見た目（0を透明とする）
	const unsigned char *collisions; // 当たり判定（0=空気, 1=壁）
} SpriteDef;


//====================================================================================================
// Global Variables
//====================================================================================================
// 背景の見た目と、当たり判定マップ
static unsigned char bg_color_buffer[SCREEN_WIDTH * SCREEN_HEIGHT];
static unsigned char collision_map[SCREEN_WIDTH * SCREEN_HEIGHT];

static unsigned int rand_seed = 12345;
static int get_rand(void) {
	rand_seed = rand_seed * 1103515245 + 12345;
	return (int)((rand_seed / 65536) % 32768);
}


//====================================================================================================
// Sprite Definitions (16x16)
//====================================================================================================

// ① マリオのような「レンガ床」 (見た目あり・判定すべてあり)
const unsigned char floor_pix[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,0,6,6,6,6,6,6,6,6,6,6,0,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,6,0,6,6,6,6,6,6,6,6,6,6,0,6,0,
	0,6,6,6,6,6,6,6,6,6,6,6,6,6,6,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};
const unsigned char floor_col[256] = {
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};
const SpriteDef spr_floor = { 16, 16, floor_pix, floor_col };

// ② 斜めの坂道 (階段状のドットと判定)
const unsigned char slope_pix[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,
	0,0,0,0,0,0,0,0,0,0,0,0,0,5,5,5,
	0,0,0,0,0,0,0,0,0,0,0,0,5,5,5,5,
	0,0,0,0,0,0,0,0,0,0,0,5,5,5,5,5,
	0,0,0,0,0,0,0,0,0,0,5,5,5,5,5,5,
	0,0,0,0,0,0,0,0,0,5,5,5,5,5,5,5,
	0,0,0,0,0,0,0,0,5,5,5,5,5,5,5,5,
	0,0,0,0,0,0,0,5,5,5,5,5,5,5,5,5,
	0,0,0,0,0,0,5,5,5,5,5,5,5,5,5,5,
	0,0,0,0,0,5,5,5,5,5,5,5,5,5,5,5,
	0,0,0,0,5,5,5,5,5,5,5,5,5,5,5,5,
	0,0,0,5,5,5,5,5,5,5,5,5,5,5,5,5,
	0,0,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	0,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,
	5,5,5,5,5,5,5,5,5,5,5,5,5,5,5,5
};
const unsigned char slope_col[256] = {
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};
const SpriteDef spr_slope = { 16, 16, slope_pix, slope_col };

// ③ 草むら・背景 (見た目あり・判定なし=すべて0)
const unsigned char grass_pix[256] = { /* 省略してすべて緑(2)で塗りつぶし */
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
};
// 判定はすべて0 (すり抜ける)
const unsigned char grass_col[256] = { 0 }; 
const SpriteDef spr_grass = { 16, 16, grass_pix, grass_col };

// ④ 見えない壁 (見た目なし=すべて0・判定あり)
const unsigned char invis_pix[256] = { 0 };
// 判定はすべて1 (ブロックと同じ)
const SpriteDef spr_invisible = { 16, 16, invis_pix, floor_col };


//====================================================================================================
// Functions
//====================================================================================================

// スプライトをマップ（背景色と当たり判定）にスタンプする関数
void put_sprite_to_map(int map_x, int map_y, const SpriteDef *sprite) {
	for (int y = 0; y < sprite->height; y++) {
		for (int x = 0; x < sprite->width; x++) {
			int px = map_x + x;
			int py = map_y + y;
			
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT) {
				int index = y * sprite->width + x;
				
				// ① 色の描画（0は透明色として扱い、元の背景を残す）
				unsigned char color = sprite->pixels[index];
				if (color != 0) {
					bg_color_buffer[py * SCREEN_WIDTH + px] = color;
				}
				
				// ② 当たり判定の描画（1なら壁にする）
				unsigned char col = sprite->collisions[index];
				if (col == 1) {
					collision_map[py * SCREEN_WIDTH + px] = 1;
				}
			}
		}
	}
}

// ステージの初期化
void init_stage(void) {
	// マップの初期化
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		bg_color_buffer[i] = 0;
		collision_map[i] = 0;
	}
	
	// マリオ風の床を敷き詰める (y=180に配置)
	for (int i = 0; i < SCREEN_WIDTH / 16; i++) {
		put_sprite_to_map(i * 16, 180, &spr_floor);
	}
	
	// 坂道を配置
	put_sprite_to_map(100, 164, &spr_slope);
	put_sprite_to_map(116, 148, &spr_slope);
	put_sprite_to_map(132, 164, &spr_floor);
	put_sprite_to_map(132, 148, &spr_floor);
	put_sprite_to_map(132, 132, &spr_slope);
	
	// すり抜ける草むらを配置 (ドットが重なっても跳ね返らない)
	put_sprite_to_map(40, 164, &spr_grass);
	put_sprite_to_map(56, 164, &spr_grass);
	
	// 見えない壁を空中に配置 (見た目はないがドットが跳ね返る)
	put_sprite_to_map(200, 100, &spr_invisible);
	put_sprite_to_map(216, 100, &spr_invisible);
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
		//------------------------------------------------------------------------------------------------
		// 1. Input Process
		//------------------------------------------------------------------------------------------------
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
					dots[i].vx = (get_rand() % 800) - 400; 
					dots[i].vy = (get_rand() % 400) - 200; 
					
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
		// 2. Physics & Collision Simulation (CCD & Separated Axis)
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
			int steps_x = (ABS(dots[i].vx) / 256) + 1;
			int steps_y = (ABS(dots[i].vy) / 256) + 1;
			int steps = (steps_x > steps_y) ? steps_x : steps_y;
			
			int step_vx = dots[i].vx / steps;
			int step_vy = dots[i].vy / steps;

			for (int s = 0; s < steps; s++) {
				if (!dots[i].active) break;

				// --- [A] X軸の移動と判定 ---
				dots[i].x += step_vx;
				
				if (dots[i].x <= 0) {
					dots[i].x = 0;
					if (dots[i].vx < 0) { dots[i].vx = -(dots[i].vx * 8) / 10; step_vx = -(step_vx * 8) / 10; }
				} else if (dots[i].x >= (SCREEN_WIDTH - 1) * 256) {
					dots[i].x = (SCREEN_WIDTH - 1) * 256;
					if (dots[i].vx > 0) { dots[i].vx = -(dots[i].vx * 8) / 10; step_vx = -(step_vx * 8) / 10; }
				}

				int cx = dots[i].x / 256;
				int cy = dots[i].y / 256;
				if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
					if (collision_map[cy * SCREEN_WIDTH + cx] == 1) {
						if (step_vx > 0) {
							dots[i].x = cx * 256 - 1;
						} else if (step_vx < 0) {
							dots[i].x = (cx + 1) * 256;
						}
						dots[i].vx = -(dots[i].vx * 8) / 10;
						step_vx = -(step_vx * 8) / 10;
					}
				}

				// --- [B] Y軸の移動と判定 ---
				dots[i].y += step_vy;
				
				if (dots[i].y >= (SCREEN_HEIGHT - 1) * 256) {
					dots[i].y = (SCREEN_HEIGHT - 1) * 256;
					if (dots[i].vy > 0) {
						dots[i].vy = -(dots[i].vy * 8) / 10;
						step_vy = -(step_vy * 8) / 10;
						if (dots[i].vy > -40) { dots[i].active = 0; break; }
					}
					dots[i].vx = (dots[i].vx * 9) / 10;
					step_vx = (step_vx * 9) / 10;
				}

				cx = dots[i].x / 256;
				cy = dots[i].y / 256;
				if (cx >= 0 && cx < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT) {
					if (collision_map[cy * SCREEN_WIDTH + cx] == 1) {
						if (step_vy > 0) {
							dots[i].y = cy * 256 - 1;
							dots[i].vy = -(dots[i].vy * 8) / 10;
							step_vy = -(step_vy * 8) / 10;
							
							if (dots[i].vy > -40) { dots[i].active = 0; break; }
							
							dots[i].vx = (dots[i].vx * 9) / 10;
							step_vx = (step_vx * 9) / 10;
						} else if (step_vy < 0) {
							dots[i].y = (cy + 1) * 256;
							dots[i].vy = -(dots[i].vy * 8) / 10;
							step_vy = -(step_vy * 8) / 10;
						}
					}
				}
			} 
		}
		
		//------------------------------------------------------------------------------------------------
		// 3. Draw
		//------------------------------------------------------------------------------------------------
		// 背景バッファをコピー（地形や色を復元）
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
			backbuffer[i] = bg_color_buffer[i];
		}
		
		draw_string(4, 4, "Ultimate Sprite Physics Engine", 15, &font8x8, backbuffer);
		
		// ドットの描画
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
		
		// カーソルの描画
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
		
		// VRAMへ転送
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) vram[i] = backbuffer[i];
		
		delay_ms(16);
	}
}