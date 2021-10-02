#ifndef PLATFORM_H
#define PLATFORM_H

typedef enum EventType
{
	EVENT_NONE,
	EVENT_CREATE,
	EVENT_QUIT,
	EVENT_SIZE,
	EVENT_MAX_EVENTS
} EventType;

typedef struct Event
{
	EventType type;
} Event;

typedef enum Key
{
	KEY_NONE,
	KEY_A,
	KEY_B,
	KEY_LEFT,
	KEY_RIGHT,
	KEY_UP,
	KEY_DOWN,
	// mouse keys. maybe its own enum?
	KEY_LMB,
	KEY_MMB,
	KEY_RMB,
	// more keys...
	KEY_ESCAPE,

	// TODO: not sure...
	KEY_LMB_UP,

	KEY_MAX_KEYS
} Key;

typedef struct Dimensions
{
	int width, height;
} Dimensions;

typedef struct Position2D
{
	int x, y;
} Position2D;

typedef struct Joystick
{
	unsigned int xPos, yPos, zPos;
	unsigned int buttons;
} Joystick;

typedef void(*READ_FILE)(char * filename, unsigned char ** out_buffer, int * out_size);
typedef void(*GET_EXE_PATH)(char * out_buffer, int buffer_size);
typedef void*(*INITIALIZE_MEMORY)(unsigned int size);
typedef int(*CREATE_WINDOW)(int width, int height, char const * title, int show);
typedef void(*DESTROY_WINDOW)(void);
typedef int(*PROCESS_EVENTS)(Event * event);
typedef int(*IS_KEY_DOWN)(Key key);
typedef int(*IS_KEY_UP)(Key key);
typedef void(*SWAP_BUFFERS)(void);
typedef Dimensions(*WINDOW_DIMENSIONS)(void);
typedef void(*DRAW_TEXT)(int x, int y, char * text);
typedef __int64(*TICKS_PER_SECOND)(void);
typedef __int64(*TICKS)(void);
typedef int(*JOYSTICK_1_READY)(void);
typedef Joystick(*JOYSTICK_POS)(void);
typedef Position2D(*MOUSE_POS)(void);

typedef struct Platform
{
	READ_FILE         read_file;
	GET_EXE_PATH      get_exe_path;
	INITIALIZE_MEMORY initialize_memory;
	CREATE_WINDOW     create_window;
	DESTROY_WINDOW    destroy_window;
	PROCESS_EVENTS    process_events;
	IS_KEY_DOWN        is_key_down;
	IS_KEY_UP         is_key_up;
	SWAP_BUFFERS      swap_buffers;
	WINDOW_DIMENSIONS window_dimensions;
	DRAW_TEXT         draw_text;
	TICKS_PER_SECOND  ticks_per_second;
	TICKS             ticks;
	JOYSTICK_1_READY  joystick_1_ready;
	JOYSTICK_POS      joystick_pos;
	MOUSE_POS         mouse_pos;

} Platform;

/* Interface */

#ifdef __cplusplus
extern "C"{
#endif 

void init_platform(Platform * p);

/* API specific. Not supposed to be called by user. */

#ifdef __cplusplus
}
#endif


#endif
