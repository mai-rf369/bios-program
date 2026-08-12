//####################################################################################################
// BIOS-PROGRAM - KdV Equation Simulation (Amplified Height) in 32bit Mode (Kernel)
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
// KdV Simulation Definitions (Fixed-point arithmetic)
//====================================================================================================
#define FIXED_SHIFT  12
#define FIXED_ONE    (1 << FIXED_SHIFT) // 4096 = 1.0

#define GRID_WIDTH   SCREEN_WIDTH       // 320グリッド

//====================================================================================================
// Main Routine
//====================================================================================================
void main(void)
{
	unsigned char *vram = (unsigned char *)VRAM;
	unsigned char backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	
	int key_state[128];
	for (int i = 0; i < 128; i++) key_state[i] = 0;
	
	// KdV方程式の波形データ
	int u_prev[GRID_WIDTH];
	int u_curr[GRID_WIDTH];
	int u_next[GRID_WIDTH];
	
	// 初期条件の設定：中央に1つの滑らかな孤立波を配置
	for (int i = 0; i < GRID_WIDTH; i++)
	{
		int val = 0;
		int dist = i - 160;
		if (dist > -25 && dist < 25) {
			val = (25 - (dist < 0 ? -dist : dist)) * 30;
		}
		
		u_curr[i] = val;
		u_prev[i] = val;
	}
	
	int cursor_x = SCREEN_WIDTH / 2;
	int cursor_y = SCREEN_HEIGHT / 2;
	
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
		
		if (key_state[KEY_LEFT])  cursor_x -= 2;
		if (key_state[KEY_RIGHT]) cursor_x += 2;
		if (key_state[KEY_UP])    cursor_y -= 2;
		if (key_state[KEY_DOWN])  cursor_y += 2;
		
		if (cursor_x < 0) cursor_x = 0;
		if (cursor_x >= SCREEN_WIDTH) cursor_x = SCREEN_WIDTH - 1;
		if (cursor_y < 0) cursor_y = 0;
		if (cursor_y >= SCREEN_HEIGHT) cursor_y = SCREEN_HEIGHT - 1;
		
		//----------------------------------------------------------------
		// 2. KdV方程式の数値計算 (修正Z-Kスキーム)
		//----------------------------------------------------------------
		for (int i = 0; i < GRID_WIDTH; i++)
		{
			int ip1 = (i + 1) % GRID_WIDTH;
			int ip2 = (i + 2) % GRID_WIDTH;
			int im1 = (i - 1 + GRID_WIDTH) % GRID_WIDTH;
			int im2 = (i - 2 + GRID_WIDTH) % GRID_WIDTH;
			
			int ui   = u_curr[i];
			int uip1 = u_curr[ip1];
			int uip2 = u_curr[ip2];
			int uim1 = u_curr[im1];
			int uim2 = u_curr[im2];
			
			// 非線形項 (3点平均による安定化)
			int u_avg = (uim1 + ui + uip1) / 3;
			long long nonlinear = ((long long)u_avg * (uip1 - uim1)) >> (FIXED_SHIFT + 1);
			
			// 分散項
			long long dispersion = (uip2 - 2 * uip1 + 2 * uim1 - uim2) >> 2;
			
			// 時間発展
			long long change = nonlinear + dispersion;
			long long next_val = (long long)u_prev[i] - change;
			
			// 安全装置のクランプ
			if (next_val > 6000)  next_val = 6000;
			if (next_val < -1000) next_val = -1000;
			
			u_next[i] = (int)next_val;
		}
		
		// バッファのシフト更新
		for (int i = 0; i < GRID_WIDTH; i++)
		{
			u_prev[i] = u_curr[i];
			u_curr[i] = u_next[i];
		}

		//----------------------------------------------------------------
		// 3. バックバッファへの描画
		//----------------------------------------------------------------
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			backbuffer[i] = 0;
		}
		
		draw_string(4, 4, "ESC: Shutdown", 15, &font8x8, backbuffer);
		draw_string(4, 14, "KdV Equation (Amplified Height)", 15, &font8x8, backbuffer);
		
		// 基準線
		int baseline_y = 120;
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			backbuffer[baseline_y * SCREEN_WIDTH + x] = 8;
		}
		
		// 波形ラインの描画（高さ方向を大きく拡大表示）
		for (int x = 0; x < SCREEN_WIDTH; x++)
		{
			// シフト量を小さくし、係数を掛けることで計算を崩さずに高さを拡大
			int height = (u_curr[x] * 4) >> (FIXED_SHIFT - 6);
			int py = baseline_y - height;
			
			if (py >= 25 && py < SCREEN_HEIGHT)
			{
				backbuffer[py * SCREEN_WIDTH + x] = 11;
			}
		}
		
		// カーソルの描画
		for (int dx = -2; dx <= 2; dx++)
		{
			int px = cursor_x + dx;
			int py = cursor_y;
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
			{
				backbuffer[py * SCREEN_WIDTH + px] = 14;
			}
		}
		for (int dy = -2; dy <= 2; dy++)
		{
			int px = cursor_x;
			int py = cursor_y + dy;
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
			{
				backbuffer[py * SCREEN_WIDTH + px] = 14;
			}
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