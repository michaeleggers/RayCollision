#include "gui.h"

static GUIState gui_state;

void gui_begin(bool (*mouse_left_released_callback)(void), GUIPoint (*mouse_pos_callback)(void))
{
	GUIPoint mouse_pos = mouse_pos_callback();
	gui_state.mouse_x = mouse_pos.x;
	gui_state.mouse_y = mouse_pos.y;

	gui_state.mouse_left_released = mouse_left_released_callback();
}


bool gui_button(char const * text, float x, float y, float width, float height, int button_ID)
{
	if ( (gui_state.mouse_x > x) && (gui_state.mouse_x < x+width) &&
		 (gui_state.mouse_y > y) && (gui_state.mouse_y < y+height) )
	{
		gui_state.hot_ID = button_ID;
	}
	else {
		gui_state.hot_ID = -1;
	}

	if ( (gui_state.hot_ID == button_ID) && gui_state.mouse_left_released) {
		return true;
	}
	
	return false;
}
