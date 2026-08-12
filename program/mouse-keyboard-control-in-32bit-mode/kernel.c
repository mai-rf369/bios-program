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
#include "mouse.h"

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
	
	// Initialize Key State
	int key_state[128];
	for (int i = 0; i < 128; i++)
	{
		key_state[i]	=	0;
	}
	
	// Initialize Mouse State
	MouseState mouse;
	initialize_mouse(&mouse);
	
	// Initialize Perticle
	Dot dots[MAX_DOTS];
	for (int i = 0; i < MAX_DOTS; i++)
	{
		dots[i].active	=	0;
	}
	
	int previous_left_button	=	0;
	
	// Game Loop
	while (1)
	{
		// Input Process
		process_keyboard(key_state);
		process_mouse(&mouse);
		
		// If ESC Key pressed, Shutdown
		if (key_state[KEY_ESC])
		{
			system_shutdown();
		}
		
		// Create new Dot
		if (mouse.left_button && !previous_left_button)
		{
			// Search inactive Dot
			for (int i = 0; i < MAX_DOTS; i++)
			{
				if (dots[i].active == 0)
				{
					dots[i].active	=	1;
					dots[i].x	=	mouse.x;
					dots[i].y	=	mouse.y * 256;
					dots[i].vy	=	0;
					
					for (int t = 0; t < TRAIL_LENGTH; t++)
					{
						dots[i].trail_x[t]	=	-1;
						dots[i].trail_y[t]	=	-1;
					}
					
					break;
				}
			}
		}
		previous_left_button	=	mouse.left_button;
		
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
		
		// Clear
		for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
		{
			vram[i]	=	0;
		}
		
		// Draw String
		draw_string(4, 4, "ESC: Shutdown", 15, &font8x8, vram);
		draw_string(4, 14, "Mouse Click to Drop", 15, &font8x8, vram);
		
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
					
					vram[ty * SCREEN_WIDTH + tx]	=	color;
				}
			}
			
			int cy	=	dots[i].y / 256;
			if (dots[i].x >= 0 && dots[i].x < SCREEN_WIDTH && cy >= 0 && cy < SCREEN_HEIGHT)
			{
				vram[cy * SCREEN_WIDTH + dots[i].x]	=	11;
			}
		}
		
		// Draw Mouse Pointer
		vram[mouse.y * SCREEN_WIDTH + mouse.x]	=	12;
		
		delay_ms(16);
	}
}
