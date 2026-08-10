// Define Settings
#define VRAM 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static inline unsigned char inb(unsigned short port);
void wait_vsync(void);

void main(void)
{
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Clear Display
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		vram[i]	=	0;
	}
	
	// Initialize Dot State
	int x	=	160;
	int y	=	100;
	int dx	=	1;
	int dy	=	1;
	
	unsigned char dot_color	=	15;	// White
	unsigned char bg_color	=	0;	// Black
	
	while (1)
	{
		// Clear Current Dot
		vram[y * SCREEN_WIDTH + x]	=	bg_color;
		
		// Update Position
		x	=	x + dx;
		y	=	y + dy;
		
		// Bounce
		if (x <= 0 || x >= SCREEN_WIDTH - 1)
		{
			dx	=	-dx;
		}
		if (y <= 0 || y >= SCREEN_HEIGHT - 1)
		{
			dy	=	-dy;
		}
		
		// Draw Dot
		vram[y * SCREEN_WIDTH + x]	=	dot_color;
		
		// Weight for Speed Adjustment
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
		wait_vsync();
	}
}

// Read 1 Byte from IO Port
static inline unsigned char inb(unsigned short port)
{
	unsigned char result;
	
	__asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
	
	return result;
}

// Wait VSYNC
void wait_vsync(void)
{
	// VGA Input Status Register 1 Port: 0x03DA
	// If Bit 3 is 1, VSYNC Interval
	
	// Wait VSYNC End If Already VSYNC
	while (inb(0x03DA) & 0x08)
	{}
	
	// Wait VSYNC Start
	while (!(inb(0x03DA) & 0x08))
	{}
}
