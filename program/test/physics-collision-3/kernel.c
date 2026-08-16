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
#include "sprite.h"
#include "map.h"

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
// Functions
//====================================================================================================
// ステージの初期化
void init_stage(void) {
	// マップの初期化
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
		bg_color_buffer[i] = 0;
		collision_map[i] = 0;
	}
	
	// 床ブロックを敷き詰める (y=180に配置)
	for (int i = 0; i < SCREEN_WIDTH / 16; i++) {
		write_sprite_to_map(bg_color_buffer, collision_map, i * 16, 180, &sprite_16x16_floor_block);
	}
	
	// レンガブロックとハードブロックで足場を作る
	write_sprite_to_map(bg_color_buffer, collision_map, 100, 132, &sprite_16x16_brick_block);
	write_sprite_to_map(bg_color_buffer, collision_map, 116, 132, &sprite_16x16_hard_block);
	write_sprite_to_map(bg_color_buffer, collision_map, 132, 132, &sprite_16x16_brick_block);
	
	// 空中にコインを配置する
	write_sprite_to_map(bg_color_buffer, collision_map, 116, 100, &sprite_16x16_coin);
}


//====================================================================================================
// Main
//====================================================================================================
void main(void)
{
	unsigned char *vram = (unsigned char *)VRAM;
	unsigned char backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	
	KeyboardState keyboard;
	initialize_keyboard(&keyboard);
	
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
		process_keyboard(&keyboard);
		
		if (keyboard.keys[KEY_ESC]) system_shutdown();
		
		if (keyboard.keys[KEY_LEFT])  cursor_x = cursor_x - 2;
		if (keyboard.keys[KEY_RIGHT]) cursor_x = cursor_x + 2;
		if (keyboard.keys[KEY_UP])    cursor_y = cursor_y - 2;
		if (keyboard.keys[KEY_DOWN])  cursor_y = cursor_y + 2;
		
		if (cursor_x < 0) cursor_x = 0;
		if (cursor_x >= SCREEN_WIDTH) cursor_x = SCREEN_WIDTH - 1;
		if (cursor_y < 0) cursor_y = 0;
		if (cursor_y >= SCREEN_HEIGHT) cursor_y = SCREEN_HEIGHT - 1;
		
		int current_space_key = keyboard.keys[KEY_SPACE];
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
		
		draw_string(4, 4, "Super Mario Like Physics Engine", 15, &font8x8, backbuffer);
		draw_string(4, 14, "ESC: Shutdown", COLOR_WHITE, &font8x8, backbuffer);
		draw_string(4, 24, "Arrows: Move", COLOR_WHITE, &font8x8, backbuffer);
		draw_string(4, 34, "Space: Drop", COLOR_WHITE, &font8x8, backbuffer);
		
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