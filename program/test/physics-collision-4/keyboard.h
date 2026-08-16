#ifndef KEYBOARD_H
#define KEYBOARD_H

typedef struct {
	int keys[128];
} KeyboardState;

void initialize_keyboard(KeyboardState *keyboard);
void process_keyboard(KeyboardState *keyboard);

#endif
