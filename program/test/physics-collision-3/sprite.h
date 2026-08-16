#ifndef SPRITE_H
#define SPRITE_H

typedef struct {
	int width;
	int height;
	const unsigned char *pixels;
	const unsigned char *collisions;
} Sprite;

extern const Sprite sprite_16x16_floor_block;
extern const Sprite sprite_16x16_brick_block;
extern const Sprite sprite_16x16_hard_block;
extern const Sprite sprite_16x16_coin;

#endif
