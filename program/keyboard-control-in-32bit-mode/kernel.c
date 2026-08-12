//####################################################################################################
// BIOS-PROGRAM - Mouse-Keyboard-Controll-in-32bit-Mode (Kernel)
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

typedef struct {
	int active;
	int x;
	int y;
	int vy;
	int trail_x[TRAIL_LENGTH];
	int trail_y[TRAIL_LENGTH];
} Dot;

//====================================================================================================
// Function
//====================================================================================================
//****************************************************************************************************
// Main
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Main Routine (main)
//----------------------------------------------------------------------------------------------------
void main(void)
{
	// Create VRAM
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Back Buffer
	unsigned char backbuffer[SCREEN_WIDTH * SCREEN_HEIGHT];
	
	// Initialize Key State
	int key_state[128];
	for (int i = 0; i < 128; i++)
	{
		key_state[i]	=	0;
	}
	
	// Initialize Perticle
	Dot dots[MAX_DOTS];
	for (int i = 0; i < MAX_DOTS; i++)
	{
		dots[i].active	=	0;
	}
	
	int cursor_x	=	SCREEN_WIDTH / 2;
	int cursor_y	=	SCREEN_HEIGHT / 2;
	
	int previous_space_key	=	0;
	
	// Game Loop
	while (1)
	{
		// Input Process
		process_keyboard(key_state);
		
		// If ESC Key pressed, Shutdown
		if (key_state[KEY_ESC])
		{
			system_shutdown();
		}
		
		if (key_state[KEY_LEFT])
		{
			cursor_x	=	cursor_x - 2;
		}
		if (key_state[KEY_RIGHT])
		{
			cursor_x	=	cursor_x + 2;
		}
		if (key_state[KEY_UP])
		{
			cursor_y	=	cursor_y - 2;
		}
		if (key_state[KEY_DOWN])
		{
			cursor_y	=	cursor_y + 2;
		}
		
		if (cursor_x < 0)
		{
			cursor_x	=	0;
		}
		if (cursor_x >= SCREEN_WIDTH)
		{
			cursor_x	=	SCREEN_WIDTH - 1;
		}
		if (cursor_y < 0)
		{
			cursor_y	=	0;
		}
		if (cursor_y >= SCREEN_HEIGHT)
		{
			cursor_y	=	SCREEN_HEIGHT - 1;
		}
		
		int current_space_key	=	key_state[KEY_SPACE];
		if (current_space_key && !previous_space_key)
		{
			for (int i = 0; i < MAX_DOTS; i++)
			{
				if (dots[i].active == 0)
				{
					dots[i].active = 1;
					dots[i].x  = cursor_x;
					dots[i].y  = cursor_y * 256;
					dots[i].vy = 0;
					
					for (int t = 0; t < TRAIL_LENGTH; t++) {
						dots[i].trail_x[t] = -1;
						dots[i].trail_y[t] = -1;
					}
					break;
				}
			}
		}
		previous_space_key	=	current_space_key;
		
		// Update Physics Simulation
		for (int i = 0; i < MAX_DOTS; i++)
		{
			if (!dots[i].active)
			{
				continue;
			}
			
			// Shift Trail History
			for (int t = TRAIL_LENGTH - 1; t > 0; t--)
			{
				dots[i].trail_x[t]	=	dots[i].trail_x[t - 1];
				dots[i].trail_y[t]	=	dots[i].trail_y[t - 1];
			}
			
			dots[i].trail_x[0]	=	dots[i].x;
			dots[i].trail_y[0]	=	dots[i].y / 256;
			
			dots[i].vy	=	dots[i].vy + 10;
			dots[i].y	=	dots[i].y + dots[i].vy;
			
			int current_y	=	dots[i].y / 256;
			if (current_y >= SCREEN_HEIGHT - 1)
			{
				dots[i].y	=	(SCREEN_HEIGHT - 1) * 256;
				
				dots[i].vy	=	-(dots[i].vy * 9) / 10;
				
				if (dots[i].vy > -30 && dots[i].vy < 30)
				{
					dots[i].active = 0;
				}
			}
		}
		
		// Clear Back Buffer
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			backbuffer[i]	=	0;
		}
		
		// Draw String
		draw_string(4, 4, "ESC: Shutdown", 15, &font8x8, backbuffer);
		draw_string(4, 14, "Arrows: Move", 15, &font8x8, backbuffer);
		draw_string(4, 24, "Space: Drop", 15, &font8x8, backbuffer);
		
		// Draw Dot & Trail
		for (int i = 0; i < MAX_DOTS; i++)
		{
			if (!dots[i].active)
			{
				continue;
			}
			
			for (int t = 0; t < TRAIL_LENGTH; t++)
			{
				int tx	=	dots[i].trail_x[t];
				int ty	=	dots[i].trail_y[t];
				
				if (tx >= 0 && tx < SCREEN_WIDTH && ty >= 0 && ty < SCREEN_HEIGHT)
				{
					unsigned char color	=	8;	// Dark Gray
					
					if (t < 3)
					{
						color	=	15;	// White
					}
					else if (t < 6)
					{
						color	=	7;	// Light Gray
					}
					
					backbuffer[ty * SCREEN_WIDTH + tx]	=	color;
				}
			}
			
			int cy	=	dots[i].y / 256;
			if (dots[i].x >= 0 && dots[i].x < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT)
			{
				backbuffer[cy * SCREEN_WIDTH + dots[i].x]	=	11;
			}
		}
		
		// Draw Cursor Pointer
		for (int dx = -2; dx <= 2; dx++)
		{
			int px	=	cursor_x + dx;
			int py	=	cursor_y;
			
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
			{
				backbuffer[py * SCREEN_WIDTH + px]	=	14;
			}
		}
		for (int dy = -2; dy <= 2; dy++)
		{
			int px	=	cursor_x;
			int py	=	cursor_y + dy;
			
			if (px >= 0 && px < SCREEN_WIDTH && py >= 0 && py < SCREEN_HEIGHT)
			{
				backbuffer[py * SCREEN_WIDTH + px]	=	14;
			}
		}
		
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			vram[i]	=	backbuffer[i];
		}
		
		delay_ms(16);
	}
}
