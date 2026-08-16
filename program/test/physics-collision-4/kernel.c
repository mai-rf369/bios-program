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
// プレイヤー構造体
typedef struct {
	int x;               // 256倍の固定小数点
	int y;               // 256倍の固定小数点
	int vx;              // 横方向の速度
	int vy;              // 縦方向の速度
	int is_grounded;     // 地面に接地しているか (0=空中, 1=接地)
	int can_double_jump; // 2段ジャンプが可能か (0=不可, 1=可能)
	int facing_right;    // 向いている方向 (0=左, 1=右)
} Player;

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
	
	// プレイヤーの初期化
	Player player = {
		.x = 64 * 256,
		.y = 100 * 256,
		.vx = 0,
		.vy = 0,
		.is_grounded = 0,
		.can_double_jump = 1,
		.facing_right = 1
	};
	
	int previous_shift_key = 0;
	int previous_z_key = 0;
	
	while (1)
	{
		//------------------------------------------------------------------------------------------------
		// 1. Input Process
		//------------------------------------------------------------------------------------------------
		process_keyboard(&keyboard);
		
		if (keyboard.keys[KEY_ESC]) system_shutdown();
		
		// 左右移動
		if (keyboard.keys[KEY_LEFT]) {
			player.vx = -400;
			player.facing_right = 0;
		} else if (keyboard.keys[KEY_RIGHT]) {
			player.vx = 400;
			player.facing_right = 1;
		} else {
			player.vx = 0; // キーを離したら即座に停止（アイワナ風）
		}
		
		// Shiftキーでのジャンプ処理（LSHIFTとRSHIFT両対応）
		int current_shift_key = keyboard.keys[KEY_LSHIFT] || keyboard.keys[KEY_RSHIFT];
		if (current_shift_key && !previous_shift_key) {
			if (player.is_grounded) {
				player.vy = -800; // 1段目ジャンプ
				player.is_grounded = 0;
				player.can_double_jump = 1;
			} else if (player.can_double_jump) {
				player.vy = -700; // 2段目ジャンプ
				player.can_double_jump = 0;
			}
		}
		previous_shift_key = current_shift_key;
		
		// Zキーでの射撃処理（弾の発射）
		int current_z_key = keyboard.keys[KEY_Z];
		if (current_z_key && !previous_z_key) {
			for (int i = 0; i < MAX_DOTS; i++) {
				if (dots[i].active == 0) {
					dots[i].active = 1;
					dots[i].x = player.x;
					// プレイヤーの少し上から発射
					dots[i].y = player.y - (8 * 256);
					
					dots[i].vx = player.facing_right ? 600 : -600;
					dots[i].vy = -200; // 少し上向きに放物線を描く
					
					for (int t = 0; t < TRAIL_LENGTH; t++) {
						dots[i].trail_x[t] = dots[i].x / 256;
						dots[i].trail_y[t] = dots[i].y / 256;
					}
					break;
				}
			}
		}
		previous_z_key = current_z_key;
		
		//------------------------------------------------------------------------------------------------
		// 2. Physics & Collision Simulation (Player)
		//------------------------------------------------------------------------------------------------
		player.vy += 15; // 重力
		player.is_grounded = 0;
		
		// プレイヤーのサブステップ計算
		int p_steps_x = (ABS(player.vx) / 256) + 1;
		int p_steps_y = (ABS(player.vy) / 256) + 1;
		int p_steps = (p_steps_x > p_steps_y) ? p_steps_x : p_steps_y;
		int p_step_vx = player.vx / p_steps;
		int p_step_vy = player.vy / p_steps;
		
		for (int s = 0; s < p_steps; s++) {
			// --- X軸の移動と判定 ---
			player.x += p_step_vx;
			
			// 画面端の判定（体の幅を考慮）
			if (player.x - 3 * 256 <= 0) { 
				player.x = 3 * 256; player.vx = 0; p_step_vx = 0; 
			}
			if (player.x + 4 * 256 >= (SCREEN_WIDTH - 1) * 256) { 
				player.x = (SCREEN_WIDTH - 1 - 4) * 256; player.vx = 0; p_step_vx = 0; 
			}
			
			int px = player.x / 256;
			int py = player.y / 256;
			int collision_x = 0;
			
			if (p_step_vx > 0) {
				// 右移動時：右端（px + 4）の頭から足元までをチェック
				int cx = px + 4;
				if (cx < SCREEN_WIDTH) {
					for (int y = py - 15; y <= py; y++) {
						if (y >= 0 && y < SCREEN_HEIGHT && collision_map[y * SCREEN_WIDTH + cx] == 1) {
							collision_x = 1; break;
						}
					}
				}
				if (collision_x) {
					player.x = (cx - 1 - 4) * 256; // 右端が壁の手前になるよう押し戻す
					player.vx = 0; p_step_vx = 0;
				}
			} else if (p_step_vx < 0) {
				// 左移動時：左端（px - 3）の頭から足元までをチェック
				int cx = px - 3;
				if (cx >= 0) {
					for (int y = py - 15; y <= py; y++) {
						if (y >= 0 && y < SCREEN_HEIGHT && collision_map[y * SCREEN_WIDTH + cx] == 1) {
							collision_x = 1; break;
						}
					}
				}
				if (collision_x) {
					player.x = (cx + 1 + 3) * 256; // 左端が壁の手前になるよう押し戻す
					player.vx = 0; p_step_vx = 0;
				}
			}
			
			// --- Y軸の移動と判定 ---
			player.y += p_step_vy;
			
			// 画面下の判定
			if (player.y >= (SCREEN_HEIGHT - 1) * 256) {
				player.y = (SCREEN_HEIGHT - 1) * 256;
				player.vy = 0; p_step_vy = 0;
				player.is_grounded = 1;
			}
			
			px = player.x / 256;
			py = player.y / 256;
			int collision_y = 0;
			
			if (p_step_vy > 0) {
				// 下移動時：足元（py）の左端から右端までをチェック
				int cy = py;
				if (cy < SCREEN_HEIGHT) {
					for (int x = px - 3; x <= px + 4; x++) {
						if (x >= 0 && x < SCREEN_WIDTH && collision_map[cy * SCREEN_WIDTH + x] == 1) {
							collision_y = 1; break;
						}
					}
				}
				if (collision_y) {
					player.y = (cy - 1) * 256; // 足元が床の上になるよう押し戻す
					player.vy = 0; p_step_vy = 0;
					player.is_grounded = 1;
				}
			} else if (p_step_vy < 0) {
				// 上移動時：頭頂部（py - 15）の左端から右端までをチェック
				int cy = py - 15;
				if (cy >= 0) {
					for (int x = px - 3; x <= px + 4; x++) {
						if (x >= 0 && x < SCREEN_WIDTH && collision_map[cy * SCREEN_WIDTH + x] == 1) {
							collision_y = 1; break;
						}
					}
				}
				if (collision_y) {
					player.y = (cy + 1 + 15) * 256; // 頭が天井の下になるよう押し戻す
					player.vy = 0; p_step_vy = 0;
				}
			}
		}

		// ※ここから下は既存の「DotのPhysicsループ」をそのまま残します※
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
		// 背景バッファをコピー
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) {
			backbuffer[i] = bg_color_buffer[i];
		}
		
		draw_string(4, 4, "I Wanna Be Like Physics Engine", 15, &font8x8, backbuffer);
		draw_string(4, 14, "Arrows: Move", COLOR_WHITE, &font8x8, backbuffer);
		draw_string(4, 24, "Shift: Jump / Z: Shoot", COLOR_WHITE, &font8x8, backbuffer);
		
		// 弾（Dot）の描画（既存のまま）
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
		
		// プレイヤーの描画（8x16の簡易的な四角形として描画）
		int px = player.x / 256;
		int py = player.y / 256;
		for (int dy = -15; dy <= 0; dy++) {
			for (int dx = -3; dx <= 4; dx++) {
				int draw_x = px + dx;
				int draw_y = py + dy;
				if (draw_x >= 0 && draw_x < SCREEN_WIDTH && draw_y >= 0 && draw_y < SCREEN_HEIGHT) {
					backbuffer[draw_y * SCREEN_WIDTH + draw_x] = 14; // 黄色
				}
			}
		}
		
		// VRAMへ転送
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) vram[i] = backbuffer[i];
		
		delay_ms(16);
	}
}