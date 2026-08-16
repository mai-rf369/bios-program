#ifndef MAP_H
#define MAP_H

#include "sprite.h"

extern void write_sprite_to_map(unsigned char *background_buffer, unsigned char *collision_map, int map_x, int map_y, const Sprite *sprite);

#endif
