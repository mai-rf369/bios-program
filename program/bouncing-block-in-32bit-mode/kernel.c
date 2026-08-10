// Define Settings
#define VRAM 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define TRAIL_LENGTH 10
#define NUMBER_OF_DOTS 5

static inline unsigned char inb(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
void delay_ms(unsigned int ms);

typedef struct {
	int x;
	int y;
	int dx;
	int dy;
	int trail_x[TRAIL_LENGTH];
	int trail_y[TRAIL_LENGTH];
	unsigned char colors[TRAIL_LENGTH];
} Dot;

void dot_initialize(Dot *dot, int x, int y, int dx, int dy, const unsigned char colors[TRAIL_LENGTH])
{
	dot->x	=	x;
	dot->y	=	y;
	dot->dx	=	dx;
	dot->dy	=	dy;
	for (int i = 0; i < TRAIL_LENGTH; i++)
	{
		dot->trail_x[i]	=	x;
		dot->trail_y[i]	=	y;
		dot->colors[i]	=	colors[i];
	}
}

void dot_update(Dot *dot)
{
	// Shift Position
	for (int i = TRAIL_LENGTH - 1; i > 0; i--)
	{
		dot->trail_x[i]	=	dot->trail_x[i - 1];
		dot->trail_y[i]	=	dot->trail_y[i - 1];
	}
	
	// Update Position
	dot->x	=	dot->x + dot->dx;
	dot->y	=	dot->y + dot->dy;
	
	// Bounce
	if (dot->x <= 0 || dot->x >= SCREEN_WIDTH - 1)
	{
		dot->dx	=	-dot->dx;
	}
	if (dot->y <= 0 || dot->y >= SCREEN_HEIGHT - 1)
	{
		dot->dy	=	-dot->dy;
	}
	
	// Save new Head Position
	dot->trail_x[0]	=	dot->x;
	dot->trail_y[0]	=	dot->y;
}

// Draw Dot
void dot_draw(Dot *dot, unsigned char *vram)
{
	// Erase oldest Trail Dot
	vram[dot->trail_y[TRAIL_LENGTH - 1] * SCREEN_WIDTH + dot->trail_x[TRAIL_LENGTH - 1]]	=	0;
	
	// Draw entire Trail
	for (int i = 0; i < TRAIL_LENGTH; i++)
	{
		vram[dot->trail_y[i] * SCREEN_WIDTH + dot->trail_x[i]]	=	dot->colors[i];
	}
}

void main(void)
{
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Clear Display
	for (int i = 0; i < SCREEN_WIDTH * SCREEN_HEIGHT; i++)
	{
		vram[i]	=	0;
	}
	
	// Define Color Presets for Trails
	unsigned char white_trail[TRAIL_LENGTH]	=	{15, 15, 14, 14, 7, 7, 8, 8, 8, 0};
	unsigned char cyan_trail[TRAIL_LENGTH]	=	{11, 11, 3, 3, 1, 1, 8, 8, 8, 0};
	unsigned char green_trail[TRAIL_LENGTH]	=	{10, 10, 2, 2, 8, 8, 8, 8, 8, 0};
	unsigned char red_trail[TRAIL_LENGTH]	=	{12, 12, 4, 4, 8, 8, 8, 8, 8, 0};
	
	Dot dots[NUMBER_OF_DOTS];
	dot_initialize(&dots[0], 160, 100, 1, 1, white_trail);
	dot_initialize(&dots[1], 100, 50, -1, 2, cyan_trail);
	dot_initialize(&dots[2], 200, 150, 2, -1, green_trail);
	dot_initialize(&dots[3], 50, 120, -2, -2, red_trail);
	
	while (1)
	{
		for (int i = 0; i < NUMBER_OF_DOTS; i++)
		{
			dot_update(&dots[i]);
			dot_draw(&dots[i], vram);
		}
		
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
	unsigned short prev_tick	=	current_tick;
	
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
