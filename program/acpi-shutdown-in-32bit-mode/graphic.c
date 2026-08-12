#include "graphic.h"

void draw_char(int x, int y, char c, unsigned char color, const Font *font, unsigned char *vram)
{
	// Skip undefined Character
	if (c < font->first_char || c > font->last_char)
	{
		return;
	}
	
	// Calculate head Address
	int char_index	=	c - font->first_char;
	int char_size	=	font->height * font->bytes_per_line;
	const unsigned char *glyph	=	font->data + (char_index * char_size);
	
	for (int py = 0; py < font->height; py++)
	{
		for (int px = 0; px < font->width; px++)
		{
			int byte_index	=	px / 8;
			int bit_index	=	7 - (px % 8);
			
			if ((glyph[py * font->bytes_per_line + byte_index] >> bit_index) && 0x01)
			{
				int screen_x	=	x + px;
				int screen_y	=	y + py;
				
				if (screen_x >= 0 && screen_x < SCREEN_WIDTH && screen_y >= 0 && screen_y < SCREEN_HEIGHT)
				{
					vram[screen_y * SCREEN_WIDTH + screen_x]	=	color;
				}
			}
		}
	}
}

void draw_string(int x, int y, const char *str, unsigned char color, const Font *font, unsigned char *vram)
{
	int current_x	=	x;
	
	for (int i = 0; str[i] != '\0'; i++)
	{
		draw_char(current_x, y, str[i], color, font, vram);
		current_x	=	current_x + font->width;
	}
}
