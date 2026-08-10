//####################################################################################################
// BIOS-PROGRAM - Movement-Controll-in-32bit-Mode (Kernel)
//####################################################################################################
//====================================================================================================
// Define Constant
//====================================================================================================
#define VRAM 0xA0000
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 200
#define ICON_SIZE 8
// Arrow Key Scan Code
#define KEY_UP		0x48
#define KEY_DOWN	0x50
#define KEY_LEFT	0x4B
#define KEY_RIGHT	0x4D

//====================================================================================================
// Declare Function
//====================================================================================================
static inline unsigned char inb(unsigned short port);
static inline void outb(unsigned short port, unsigned char data);
void delay_ms(unsigned int ms);

//====================================================================================================
// Declare Variable
//====================================================================================================
// Player Icon Sprite
const unsigned char player_icon[ICON_SIZE][ICON_SIZE] = {
	{0, 14, 14, 14, 14, 14, 14, 0},
	{14, 14, 14, 14, 14, 14, 14, 14},
	{14, 15,  0, 14, 14, 15,  0, 14},
	{14, 14, 14, 14, 14, 14, 14, 14},
	{14, 12, 14, 14, 14, 14, 12, 14},
	{14, 14,  0,  0,  0,  0, 14, 14},
	{14, 14, 14, 14, 14, 14, 14, 14},
	{ 0, 14, 14, 14, 14, 14, 14,  0}
};

// Player Object Structure
typedef struct {
	int x;
	int y;
	int dx;
	int dy;
	int speed;
	const unsigned char (*sprite)[ICON_SIZE];
} Player;

//====================================================================================================
// Function
//====================================================================================================
//****************************************************************************************************
// Player
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Player Initialize
//----------------------------------------------------------------------------------------------------
void player_initialize(Player *player, int x, int y, int speed, const unsigned char sprite[ICON_SIZE][ICON_SIZE])
{
	player->x	=	x;
	player->y	=	y;
	player->dx	=	0;
	player->dy	=	0;
	player->speed	=	speed;
	player->sprite	=	sprite;
}

//----------------------------------------------------------------------------------------------------
// Sub Routine (player_handle_input)
// Convert Keyboard State to Player Motion Vector
//----------------------------------------------------------------------------------------------------
void player_handle_input(Player *player, const int *key_state)
{
	player->dx	=	0;
	player->dy	=	0;
	
	if (key_state[KEY_UP])
	{
		player->dy	=	-player->speed;
	}
	if (key_state[KEY_DOWN])
	{
		player->dy	=	player->speed;
	}
	if (key_state[KEY_LEFT])
	{
		player->dx	=	-player->speed;
	}
	if (key_state[KEY_RIGHT])
	{
		player->dx	=	player->speed;
	}
}

//----------------------------------------------------------------------------------------------------
// Sub Routine (player_update)
// Player Position Update
//----------------------------------------------------------------------------------------------------
void player_update(Player *player)
{
	// Potition Update
	player->x	=	player->x + player->dx;
	player->y	=	player->y + player->dy;
	
	// Screen Edge Boundary Check
	if (player->x < 0)
	{
		player->x	=	0;
	}
	if (player->x > SCREEN_WIDTH - ICON_SIZE)
	{
		player->x	=	SCREEN_WIDTH - ICON_SIZE;
	}
	if (player->y < 0)
	{
		player->y	=	0;
	}
	if (player->y > SCREEN_HEIGHT - ICON_SIZE)
	{
		player->y	=	SCREEN_HEIGHT - ICON_SIZE;
	}
}

//----------------------------------------------------------------------------------------------------
// Sub Routine (player_erase)
// Erase Old Player Sprite
//----------------------------------------------------------------------------------------------------
void player_erase(const Player *player, unsigned char *vram)
{
	for (int py = 0; py < ICON_SIZE; py++)
	{
		for (int px = 0; px < ICON_SIZE; px++)
		{
			if (player->sprite[py][px] != 0)
			{
				int screen_x	=	player->x + px;
				int screen_y	=	player->y + py;
				if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT)
				{
					vram[screen_y * SCREEN_WIDTH + screen_x]	=	0;
				}
			}
		}
	}
}

//----------------------------------------------------------------------------------------------------
// Sub Routine (player_draw)
// Draw Player Sprite
//----------------------------------------------------------------------------------------------------
void player_draw(const Player *player, unsigned char *vram)
{
	for (int py = 0; py < ICON_SIZE; py++)
	{
		for (int px = 0; px < ICON_SIZE; px++)
		{
			unsigned char color	=	player->sprite[py][px];
			if (color != 0)
			{
				int screen_x	=	player->x + px;
				int screen_y	=	player->y + py;
				if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT)
				{
					vram[screen_y * SCREEN_WIDTH + screen_x]	=	color;
				}
			}
		}
	}
}

//****************************************************************************************************
// Keyboard Driver
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Sub Routine (process_keyboard)
//----------------------------------------------------------------------------------------------------
void process_keyboard(int *key_state)
{
	while (inb(0x64) & 0x01)
	{
		unsigned char scan	=	inb(0x60);
		if (scan == 0xE0)
		{
			continue;
		}
		
		if (scan & 0x80)
		{
			key_state[scan & 0x7F]	=	0;	// Release
		}
		else
		{
			key_state[scan]	=	1;	// Press
		}
	}
}

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
	
	// Create Player Object
	Player player;
	player_initialize(&player, 156, 96, 1, player_icon);
	
	int key_state[128];
	for (int i = 0; i < 128; i++)
	{
		key_state[i]	=	0;
	}
	
	// Game Loop
	while (1)
	{
		process_keyboard(key_state);
		
		player_erase(&player, vram);
		
		player_handle_input(&player, key_state);
		player_update(&player);
		
		player_draw(&player, vram);
		
		delay_ms(16);
	}
}

//****************************************************************************************************
// Hardware Driver
//****************************************************************************************************
//----------------------------------------------------------------------------------------------------
// Sub Routine (inb)
// Read 1 Byte from IO Port
//----------------------------------------------------------------------------------------------------
static inline unsigned char inb(unsigned short port)
{
	unsigned char result;
	
	__asm__ volatile ("inb %1, %0" : "=a"(result) : "Nd"(port));
	
	return result;
}
//----------------------------------------------------------------------------------------------------
// Sub Routine (outb)
// Write 1 Byte to IO Port
//----------------------------------------------------------------------------------------------------
static inline void outb(unsigned short port, unsigned char data)
{
	__asm__ volatile ("outb %0, %1" : : "a"(data), "Nd"(port));
}
//----------------------------------------------------------------------------------------------------
// Sub Routine (read_pit)
// Read Current PIT Channel 0 Count
//----------------------------------------------------------------------------------------------------
unsigned short read_pit(void)
{
	unsigned short count;
	
	// Send Latch Command (0x00) to PIT Command Port (0x43)
	outb(0x43, 0x00);
	
	count	=	inb(0x40);			// Read Low Byte
	count	=	count | (inb(0x40) << 8);	// Read High Byte
	
	return count;
}
//----------------------------------------------------------------------------------------------------
// Sub Routine (delay_ms)
// Wait for specified MilliSeconds
//----------------------------------------------------------------------------------------------------
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
