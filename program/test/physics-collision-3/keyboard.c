#include "keyboard.h"
#include "hardware.h"

void initialize_keyboard(KeyboardState *keyboard)
{
	for (int i = 0; i < 128; i++)
	{
		keyboard->keys[i]	=	0;
	}
}

void process_keyboard(KeyboardState *keyboard)
{
	// Read Status Register in Keyboard Controller
	while (inb(0x64) & 0x01 && !(inb(0x64) & 0x20))
	{
		// Read Scan Code from Data Port (0x60)
		unsigned char scan	=	inb(0x60);
		
		// Skip Extended Key Prefix
		if (scan == 0xE0)
		{
			continue;
		}
		
		if (scan & 0x80)
		{
			// Release
			keyboard->keys[scan & 0x7F]	=	0;
		}
		else
		{
			// Press
			keyboard->keys[scan]		=	1;
		}
	}
}

