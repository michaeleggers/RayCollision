#ifndef GUI_H
#define GUI_H

typedef struct GUIPoint
{
	float x, y;
} GUIPoint;

typedef struct GUIState
{
	int hot_ID;             // hot widget, eg. mouse cursor is over it.

	float mouse_x;
	float mouse_y;

	bool mouse_left_released;
} GUIState;

void gui_begin( bool (*mouse_left_released_callback)(void), GUIPoint (*mouse_pos_callback)(void));
bool gui_button(char const * text, float x, float y, float width, float hight, int button_ID);

#endif