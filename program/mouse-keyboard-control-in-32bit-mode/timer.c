#include "timer.h"
#include "hardware.h"

unsigned short read_pit(void)
{
	unsigned short count;
	
	// Send Latch Command (0x00) to PIT Command Port (0x43)
	outb(0x43, 0x00);
	
	count	=	inb(0x40);			// Read Lower Byte
	count	=	count | (inb(0x40) << 8);	// Read Higher Byte
	
	return count;
}

void delay_ms(unsigned int ms)
{
	// PIT works exactly 1,193,182 Hz (1ms = 1193 ticks)
	unsigned int target_ticks	=	ms * 1193;
	unsigned int elapsed_ticks	=	0;
	
	unsigned short current_tick	=	read_pit();
	unsigned short previous_tick	=	current_tick;
	
	while (elapsed_ticks < target_ticks)
	{
		current_tick	=	read_pit();
		
		if (previous_tick >= current_tick)
		{
			elapsed_ticks	=	elapsed_ticks + (previous_tick - current_tick);
		}
		else
		{
			// If Underflow occurs (from 0 to 65535)
			elapsed_ticks	=	elapsed_ticks + (previous_tick + (65536 - current_tick));
		}
		
		previous_tick	=	current_tick;
	}
}

