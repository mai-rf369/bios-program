//####################################################################################################
// BIOS-PROGRAM - Rain Dodge Game (Square Player & Blue Trail) in 32bit Mode (Kernel)
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

//====================================================================================================
// Game Definitions
//====================================================================================================
#define FIXED_SHIFT  8
#define FIXED_ONE    (1 << FIXED_SHIFT) // 256 = 1.0

#define MAX_RAINS    15
#define TRAIL_LEN    6                  // 雨粒の軌跡の長さ

// 簡易乱数生成器
static unsigned int rng_seed = 12345;
static int get_rand(void)
{
	rng_seed = rng_seed * 1103515245 + 12301;
	return (unsigned int)(rng_seed / 65536) % 32767;
}

// 雨粒構造体（軌跡データ付き）
typedef struct {
	int active;
	int x;
	int y;
	int vx;
	int vy;
	int trail_x[TRAIL_LEN];
	int trail_y[TRAIL_LEN];
} Rain;

//====================================================================================================
// Main Routine
//====================================================================================================
void main(void)
{
	unsigned char *vram = (unsigned char *)VRAM;
	unsigned char backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	
	int key_state[128];
	for (int i = 0; i < 128; i++) key_state[i] = 0;
	
	// ゲーム状態変数
	int player_x = SCREEN_WIDTH / 2;
	int player_size = 8;                 // 正方形のサイズ (幅8, 高さ8)
	int player_y = 180;                  // 地面の上
	
	Rain rains[MAX_RAINS];
	for (int i = 0; i < MAX_RAINS; i++) rains[i].active = 0;
	
	int spawn_timer = 0;
	int score = 0;
	int game_over = 0;
	int prev_space = 0;
	
	// Game Loop
	while (1)
	{
		//----------------------------------------------------------------
		// 1. 入力処理
		//----------------------------------------------------------------
		process_keyboard(key_state);
		
		if (key_state[KEY_ESC])
		{
			system_shutdown();
		}
		
		int current_space = key_state[KEY_SPACE];
		
		if (game_over)
		{
			if (current_space && !prev_space)
			{
				game_over = 0;
				score = 0;
				player_x = SCREEN_WIDTH / 2;
				for (int i = 0; i < MAX_RAINS; i++) rains[i].active = 0;
			}
			prev_space = current_space;
			delay_ms(16);
			continue;
		}
		
		// プレイヤー移動
		if (key_state[KEY_LEFT])  player_x -= 4;
		if (key_state[KEY_RIGHT]) player_x += 4;
		
		if (player_x < 0) player_x = 0;
		if (player_x + player_size >= SCREEN_WIDTH) player_x = SCREEN_WIDTH - player_size;
		
		//----------------------------------------------------------------
		// 2. 物理シミュレーション
		//----------------------------------------------------------------
		spawn_timer++;
		if (spawn_timer > 15)
		{
			spawn_timer = 0;
			for (int i = 0; i < MAX_RAINS; i++)
			{
				if (!rains[i].active)
				{
					rains[i].active = 1;
					rains[i].x = (get_rand() % (SCREEN_WIDTH - 20) + 10) << FIXED_SHIFT;
					rains[i].y = 10 << FIXED_SHIFT;
					rains[i].vx = (get_rand() % 120) - 60;
					rains[i].vy = get_rand() % 40;
					
					// 軌跡の初期化
					for (int t = 0; t < TRAIL_LEN; t++) {
						rains[i].trail_x[t] = -1;
						rains[i].trail_y[t] = -1;
					}
					break;
				}
			}
		}
		
		for (int i = 0; i < MAX_RAINS; i++)
		{
			if (!rains[i].active) continue;
			
			// 軌跡の履歴を更新
			for (int t = TRAIL_LEN - 1; t > 0; t--)
			{
				rains[i].trail_x[t] = rains[i].trail_x[t - 1];
				rains[i].trail_y[t] = rains[i].trail_y[t - 1];
			}
			rains[i].trail_x[0] = rains[i].x >> FIXED_SHIFT;
			rains[i].trail_y[0] = rains[i].y >> FIXED_SHIFT;
			
			// 重力加算
			rains[i].vy += 10;
			
			rains[i].x += rains[i].vx;
			rains[i].y += rains[i].vy;
			
			int rx = rains[i].x >> FIXED_SHIFT;
			int ry = rains[i].y >> FIXED_SHIFT;
			
			// 壁での跳ね返り
			if (rx <= 2) {
				rains[i].x = 2 << FIXED_SHIFT;
				rains[i].vx = -(rains[i].vx * 7) / 10;
			} else if (rx >= SCREEN_WIDTH - 3) {
				rains[i].x = (SCREEN_WIDTH - 3) << FIXED_SHIFT;
				rains[i].vx = -(rains[i].vx * 7) / 10;
			}
			
			// 地面での跳ね返り (Y = 190)
			if (ry >= 190) {
				rains[i].y = 190 << FIXED_SHIFT;
				rains[i].vy = -(rains[i].vy * 6) / 10;
				if (rains[i].vy > -20 && rains[i].vy < 20) {
					rains[i].active = 0;
					score += 10;
				}
			}
			
			// 正方形プレイヤーとの当たり判定
			if (ry >= player_y && ry < player_y + player_size &&
				rx >= player_x && rx < player_x + player_size)
			{
				game_over = 1;
			}
		}

		//----------------------------------------------------------------
		// 3. バックバッファへの描画
		//----------------------------------------------------------------
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			backbuffer[i] = 0;
		}
		
		draw_string(4, 4, "Rain Dodge Game", 15, &font8x8, backbuffer);
		draw_string(4, 14, "ESC: Shutdown", 15, &font8x8, backbuffer);
		draw_string(4, 24, "Arrow: Move", 15, &font8x8, backbuffer);
		
		// 地面ライン
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			backbuffer[190 * SCREEN_WIDTH + x] = 8;
		}
		
		// 正方形プレイヤーの描画（黄色: 14）
		for (int px = player_x; px < player_x + player_size; px++)
		{
			for (int py = player_y; py < player_y + player_size; py++)
			{
				if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
				{
					backbuffer[py * SCREEN_WIDTH + px] = 14;
				}
			}
		}
		
		// 雨粒と青系統の軌跡の描画
		for (int i = 0; i < MAX_RAINS; i++)
		{
			if (!rains[i].active) continue;
			
			// 軌跡の描画（古い部分ほど暗い青、先端に近いほど明るい青）
			for (int t = 0; t < TRAIL_LEN; t++)
			{
				int tx = rains[i].trail_x[t];
				int ty = rains[i].trail_y[t];
				
				if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT)
				{
					unsigned char color = 1; // 標準の青
					if (t < 2) color = 9;    // 先頭付近は明るい青
					else if (t < 4) color = 1; // 中間は通常の青
					
					backbuffer[ty * SCREEN_WIDTH + tx] = color;
				}
			}
			
			// 雨粒のヘッド
			int rx = rains[i].x >> FIXED_SHIFT;
			int ry = rains[i].y >> FIXED_SHIFT;
			if (rx >= 0 && rx < SCREEN_WIDTH && ry >= 0 && ry < SCREEN_HEIGHT)
			{
				backbuffer[ry * SCREEN_WIDTH + rx] = 11; // 最先端は明るいシアン系
			}
		}
		
		if (game_over)
		{
			draw_string(125, 90, "GAME OVER", 4, &font8x8, backbuffer);
			draw_string(75, 110, "Press SPACE to Restart", 15, &font8x8, backbuffer);
		}

		//----------------------------------------------------------------
		// 4. VRAM転送 ＆ 待機
		//----------------------------------------------------------------
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			vram[i] = backbuffer[i];
		}
		
		delay_ms(16);
	}
}