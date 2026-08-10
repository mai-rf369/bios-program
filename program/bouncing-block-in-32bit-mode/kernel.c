// Define Settings
#define VRAM 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

static inline unsigned char inb(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
void wait_vsync(void);
void delay_ms(unsigned int ms);

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
		delay_ms(16);
	}
}

// Read 1 Byte from IO Port
static inline unsigned char inb(unsigned short port)
{
	unsigned char result;
	
	__asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
	
	return result;
}

// Write 1 Byte to IO Port
static inline void outb(unsigned short port, unsigned char data)
{
	__asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
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

// Read Current PIT Channel 0 Count
unsigned short read_pit(void)
{
	unsigned short count;
	
	// Send Latch Command (0x00) to PIT Command Port (0x43)
	outb(0x43, 0x00);
	
	count	=	inb(0x40);			// Read Low Byte
	count	=	count | (inb(0x40) << 8);	// Read High Byte
	
	return count;
}

// Wait for specified MilliSeconds
void delay_ms(unsigned int ms)
{
	// PIT runs exactly at 1,193,182Hz. (1ms = 1,193 ticks)
	unsigned int target_ticks	=	ms * 1193;
	unsigned int elapsed_ticks	=	0;
	
	unsigned short current_tick	=	read_pit();
	unsigned short prev_tick	=	current_tick();
	
	while (elapsed_ticks < target_ticks)
	{
		current_tick	=	read_pit();
		
		// PIT is a Decrementing Counter
		if (prev_tick >= current_tick)
		{
			elapsed_ticks	=	elapsed_ticks + (prev_tick - current_tick);
		}
		else
		{
			elapsed_ticks	=	elapsed_ticks + (prev_tick + (65536 - current_tick));
		}
		
		prev_tick	=	current_tick;
	}
}
