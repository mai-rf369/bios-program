#include "keyboard.h"
#include "hardware.h"

void process_keyboard(int *key_state)
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
			key_state[scan & 0x7F]	=	0;
		}
		else
		{
			// Press
			key_state[scan]		=	1;
		}
	}
}

