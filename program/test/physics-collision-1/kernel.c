//####################################################################################################
// BIOS-PROGRAM - Physics & Collision Test (Kernel)
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
#define MAX_BLOCKS	20

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

// 障害物（ブロック）構造体
typedef struct {
	int active;
	int x;
	int y;
	int w;      // 幅
	int h;      // 高さ
	unsigned char color; // 色
} Block;

// 簡易乱数生成器（横方向に散らすため）
static unsigned int rand_seed = 12345;
static int get_rand(void) {
	rand_seed = rand_seed * 1103515245 + 12345;
	return (int)((rand_seed / 65536) % 32768);
}

// ステージの初期化（ここを書き換えることでステージを作れます）
void init_stage(Block *blocks) {
	for (int i = 0; i < MAX_BLOCKS; i++) {
		blocks[i].active = 0;
	}

	// blocks[インデックス] = (Block){有効フラグ, x, y, 幅, 高さ, 色};
	
	// 空中の足場
	blocks[0] = (Block){1, 100, 140,  80, 10, 2};  // 緑の足場
	blocks[1] = (Block){1,  30, 100,  50, 10, 3};  // 水色の足場
	blocks[2] = (Block){1, 220, 100,  60, 10, 5};  // 紫の足場

	// 縦の壁（障害物）
	blocks[3] = (Block){1, 140,  60,  10, 40, 4};  // 赤の縦壁
	
	// 少し段差のある床
	blocks[4] = (Block){1, 200, 170, 100, 30, 6};  // 茶色の段差
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
	
	Block blocks[MAX_BLOCKS];
	init_stage(blocks); // ステージ配置を呼び出し
	
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
					// 横方向の初速をランダムに与える (-300 〜 +300)
					dots[i].vx = (get_rand() % 600) - 300; 
					dots[i].vy = 0;
					
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
		// 2. Physics & Collision Simulation
		//------------------------------------------------------------------------------------------------
		for (int i = 0; i < MAX_DOTS; i++)
		{
			if (!dots[i].active) continue;
			
			// 軌跡の更新
			for (int t = TRAIL_LENGTH - 1; t > 0; t--)
			{
				dots[i].trail_x[t] = dots[i].trail_x[t - 1];
				dots[i].trail_y[t] = dots[i].trail_y[t - 1];
			}
			dots[i].trail_x[0] = dots[i].x / 256;
			dots[i].trail_y[0] = dots[i].y / 256;
			
			// 重力と移動
			dots[i].vy = dots[i].vy + 15; // 縦の重力
			dots[i].x  = dots[i].x + dots[i].vx;
			dots[i].y  = dots[i].y + dots[i].vy;
			
			// ① 画面端との当たり判定（固定小数点座標で直接判定する）
			if (dots[i].x <= 0) {
				dots[i].x = 0;
				if (dots[i].vx < 0) dots[i].vx = -(dots[i].vx * 8) / 10;
			} else if (dots[i].x >= (SCREEN_WIDTH - 1) * 256) {
				dots[i].x = (SCREEN_WIDTH - 1) * 256;
				if (dots[i].vx > 0) dots[i].vx = -(dots[i].vx * 8) / 10;
			}
			
			// 画面下端との当たり判定
			if (dots[i].y >= (SCREEN_HEIGHT - 1) * 256) {
				dots[i].y = (SCREEN_HEIGHT - 1) * 256;
				if (dots[i].vy > 0) {
					dots[i].vy = -(dots[i].vy * 8) / 10;
					
					// 【修正】跳ね返る勢いが十分でない場合は削除する
					if (dots[i].vy > -40) {
						dots[i].active = 0;
					}
				}
				dots[i].vx = (dots[i].vx * 9) / 10; // 地面との摩擦
			}

			// 既に削除されていたらブロック判定はスキップ
			if (!dots[i].active) continue;

			// ② ブロックとの当たり判定用に、現在のピクセル座標を計算
			int current_x = dots[i].x / 256;
			int current_y = dots[i].y / 256;
			int prev_x = dots[i].trail_x[0];
			int prev_y = dots[i].trail_y[0];

			// ② ブロックとの当たり判定（AABB判定）
			for (int b = 0; b < MAX_BLOCKS; b++) {
				if (!blocks[b].active) continue;
				
				// ドットがブロックの矩形内に入ったか判定
				if (current_x >= blocks[b].x && current_x < blocks[b].x + blocks[b].w &&
				    current_y >= blocks[b].y && current_y < blocks[b].y + blocks[b].h) {
					
					int hit = 0;

					// 上からぶつかった
					if (prev_y < blocks[b].y) {
						// 【修正】1ピクセル(256)上ではなく、境界線の1サブピクセル上に配置する
						dots[i].y = blocks[b].y * 256 - 1; 
						if (dots[i].vy > 0) { // 下に向かっている時のみ
							dots[i].vy = -(dots[i].vy * 8) / 10; // 跳ね返る
							
							// 跳ね返る勢いが十分でない場合は削除する
							if (dots[i].vy > -40) {
								dots[i].active = 0;
								break; // 削除されたのでこのドットのブロック判定を終了
							}
						}
						dots[i].vx = (dots[i].vx * 9) / 10;  // 摩擦
						hit = 1;
					}
					// 下からぶつかった
					else if (prev_y >= blocks[b].y + blocks[b].h) {
						dots[i].y = (blocks[b].y + blocks[b].h) * 256;
						if (dots[i].vy < 0) { // 上に向かっている時のみ
							dots[i].vy = -(dots[i].vy * 8) / 10;
						}
						hit = 1;
					}
					
					// 左からぶつかった
					if (!hit && prev_x < blocks[b].x) {
						// 【修正】ここも同様に境界線の1サブピクセル左に配置する
						dots[i].x = blocks[b].x * 256 - 1;
						if (dots[i].vx > 0) {
							dots[i].vx = -(dots[i].vx * 8) / 10;
						}
						hit = 1;
					}
					// 右からぶつかった
					else if (!hit && prev_x >= blocks[b].x + blocks[b].w) {
						dots[i].x = (blocks[b].x + blocks[b].w) * 256;
						if (dots[i].vx < 0) {
							dots[i].vx = -(dots[i].vx * 8) / 10;
						}
						hit = 1;
					}
				}
			}
		}
		
		//------------------------------------------------------------------------------------------------
		// 3. Draw
		//------------------------------------------------------------------------------------------------
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) backbuffer[i] = 0;
		
		// UI
		draw_string(4, 4, "Physics & Collision Test", 15, &font8x8, backbuffer);
		
		// ブロックの描画
		for (int b = 0; b < MAX_BLOCKS; b++) {
			if (!blocks[b].active) continue;
			for (int y = blocks[b].y; y < blocks[b].y + blocks[b].h; y++) {
				for (int x = blocks[b].x; x < blocks[b].x + blocks[b].w; x++) {
					if (x >= 0 && x < SCREEN_WIDTH && y >= 0 && y < SCREEN_HEIGHT) {
						backbuffer[y * SCREEN_WIDTH + x] = blocks[b].color;
					}
				}
			}
		}

		// ドットと軌跡の描画
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
		
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++) vram[i] = backbuffer[i];
		
		delay_ms(16);
	}
}