#include "mouse.h"
#include "hardware.h"
#include "graphic.h"

static void wait_before_write(void)
{
	while ((inb(0x64) & 0x02) != 0)
	{}
}

static void wait_before_read(void)
{
	while ((inb(0x64) & 0x01) == 0)
	{}
}

void initialize_mouse(MouseState *mouse)
{
	// Reset Mouse Position
	mouse->x	=	SCREEN_WIDTH / 2;
	mouse->y	=	SCREEN_HEIGHT / 2;
	mouse->left_button	=	0;
	mouse->right_button	=	0;
	
	// Enable Mouse Port
	wait_before_write();
	outb(0x64, 0xA8);
	
	// Signal to send next Command to Mouse
	wait_before_write();
	outb(0x64, 0xD4);
	
	// Enable Mouse Data Reporting
	wait_before_write();
	outb(0x60, 0xF4);
	
	// Skip Mouse Response
	wait_before_read();
	inb(0x60);
}

void process_mouse(MouseState *mouse)
{
	static unsigned char mouse_cycle	=	0;
	static unsigned char mouse_packet[3];
	
	while ((inb(0x64) & 0x01) && (inb(0x64) & 0x20))
	{
		unsigned char data	=	inb(0x60);
		mouse_packet[mouse_cycle]	=	data;
		
		switch (mouse_cycle)
		{
			case 0:
				if (data & 0x08) {
					mouse_cycle	=	mouse_cycle + 1;
				}
				break;
			case 1:
				mouse_cycle	=	mouse_cycle + 1;
				break;
			case 2:
				{
					int dx	=	mouse_packet[1];
					int dy	=	mouse_packet[2];
					
					if (mouse_packet[0] & 0x10)
					{
						dx	=	dx | 0xFFFFFF00;
					}
					if (mouse_packet[0] & 0x20)
					{
						dy	=	dy | 0xFFFFFF00;
					}
					
					mouse->x	=	mouse->x + dx;
					mouse->y	=	mouse->y - dy;
					
					if (mouse->x < 0)
					{
						mouse->x	=	0;
					}
					if (mouse->x >= SCREEN_WIDTH)
					{
						mouse->x	=	SCREEN_WIDTH - 1;
					}
					if (mouse->y < 0)
					{
						mouse->y	=	0;
					}
					if (mouse->y >= SCREEN_HEIGHT)
					{
						mouse->y	=	SCREEN_HEIGHT - 1;
					}
					
					mouse->left_button	=	mouse_packet[0] & 0x01;
					mouse->right_button	=	(mouse_packet[0] & 0x02) >> 1;
					
					mouse_cycle = 0;
				}
				break;
		}
	}
}
