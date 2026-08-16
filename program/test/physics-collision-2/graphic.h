#ifndef GRAPHIC_H
#define GRAPHIC_H

#define VRAM		0xA0000
#define SCREEN_WIDTH	320
#define SCREEN_HEIGHT	200

typedef struct {
	const unsigned char *data;
	int width;
	int height;
	int bytes_per_line;
	int first_char;
	int last_char;
} Font;

void draw_char(int x, int y, char c, unsigned char color, const Font *font, unsigned char *vram);
void draw_string(int x, int y, const char *str, unsigned char color, const Font *font, unsigned char *vram);

#endif
