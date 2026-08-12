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
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Clear Display
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		vram[i]	=	0;
	}
	
	// Initialize Key State
	int key_state[128];
	for (int i = 0; i < 128; i++)
	{
		key_state[i]	=	0;
	}
	
	// Initialize Mouse State
	MouseState mouse;
	initialize_mouse(&mouse);
	int previous_x	=	mouse.x;
	int previous_y	=	mouse.y;
	
	// Game Loop
	while (1)
	{
		process_keyboard(key_state);
		process_mouse(&mouse);
		
		// If ESC Key pressed, Shutdown
		if (key_state[KEY_ESC])
		{
			system_shutdown();
		}
		
		// Clear old Mouse Dot
		vram[previous_y * SCREEN_WIDTH + previous_x]	=	0;
		
		// Draw String
		draw_string(4, 4, "ESC: Shutdown", 15, &font8x8, vram);
		draw_string(4, 14, "Mouse: Dot Control", 15, &font8x8, vram);
		
		// Draw Mouse Dot
		vram[mouse.y * SCREEN_WIDTH + mouse.x]	=	15;
		
		// Save Previous Mouse Position
		previous_x	=	mouse.x;
		previous_y	=	mouse.y;
		
		delay_ms(16);
	}
}
