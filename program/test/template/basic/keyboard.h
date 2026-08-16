#ifndef KEYBOARD_H
#define KEYBOARD_H

//**************************************************
// Keyboard State Structure
//**************************************************
typedef struct {
	int keys[128];
} KeyboardState;

//**************************************************
// Function
//**************************************************
void initialize_keyboard(KeyboardState *keyboard);
void process_keyboard(KeyboardState *keyboard);

#endif
