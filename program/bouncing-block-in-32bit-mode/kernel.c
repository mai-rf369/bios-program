// Define Settings
#define VRAM 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200

void delay(int count);

void main(void)
{
	unsigned char *vram	=	(unsigned char *)VRAM;
	
	// Clear Display
	
	
	
}

void delay(int count)
{
	for (volatile int i = 0; i < count; i++)
	{}
}
