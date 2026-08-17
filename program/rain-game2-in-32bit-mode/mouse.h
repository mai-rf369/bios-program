#ifndef MOUSE_H
#define MOUSE_H

typedef struct {
	int x;
	int y;
	int left_button;
	int right_button;
} MouseState;

void initialize_mouse(MouseState *mouse);
void process_mouse(MouseState *mouse);

#endif
