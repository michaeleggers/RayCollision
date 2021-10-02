

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "platform.h"

#ifdef _WIN32

#include <windows.h>
#include <wingdi.h>
#include <gl\gl.h>
#include <gl\glu.h>
//#include <gl\glaux.h>

void win32_read_file(char * filename, unsigned char ** out_buffer, int * out_size)
{
	//FILE * file = 0;
	//fopen_s(&file, filename, "rb");
	//fseek(file, 0L, SEEK_END);
	//*out_size = ftell(file);
	//fseek(file, 0L, SEEK_SET);
	//*out_buffer = (char*)malloc(*out_size);
	//fread(*out_buffer, sizeof(char), *out_size, file);
	//fclose(file);
	HANDLE fileHandle;
	fileHandle = CreateFile(filename,
		GENERIC_READ,
		FILE_SHARE_READ,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL
	);
	if (fileHandle == INVALID_HANDLE_VALUE)
		printf("unable to open file: %s\n", filename);

	DWORD filesize = GetFileSize(fileHandle, NULL);

	*out_buffer = (unsigned char*)VirtualAlloc(
		NULL,
		(filesize+1) * sizeof(unsigned char),
		MEM_RESERVE | MEM_COMMIT,
		PAGE_READWRITE
	);
	//*out_buffer = (uint8_t*)malloc((filesize + 1) * sizeof(uint8_t));
	assert( (*out_buffer != NULL) && "Failed to allocate memory for file" );

	struct _OVERLAPPED ov = { 0 };
	DWORD numBytesRead = 0;
	DWORD error;
	if (ReadFile(fileHandle, *out_buffer, filesize, &numBytesRead, NULL) == 0)
	{
		error = GetLastError();
		char errorMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

		printf("%s\n", errorMsgBuf);
	}
	else
	{
		(*out_buffer)[filesize] = '\0';
		*out_size = filesize;
	}
	CloseHandle(fileHandle);
}

void win32_get_exe_path(char * out_buffer, int buffer_size)
{
	DWORD len = GetModuleFileNameA(NULL, out_buffer, buffer_size);
	if ( !len ) {
		DWORD error = GetLastError();
		char errorMsgBuf[256];
		FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
			NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
			errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

		printf("%s\n", errorMsgBuf);
	}

	// strip actual name of the .exe
	char * last = out_buffer + len;
	while ( *last != '\\') {
		*last-- = '\0';
	}	

	// NOTE: GetCurrentDirectory returns path from WHERE the exe was called from.
	//if (!GetCurrentDirectory(
	//	buffer_size,
	//	out_buffer
	//)) {
	//	DWORD error = GetLastError();
	//	char errorMsgBuf[256];
	//	FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
	//		NULL, error, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
	//		errorMsgBuf, (sizeof(errorMsgBuf) / sizeof(char)), NULL);

	//	printf("%s\n", errorMsgBuf);
	//}
}

void * win32_initialize_memory(unsigned int size)
{
	void * result = malloc(size);
	assert( (result != NULL) && "Failed to initialize memory" );
	memset(result, 0, size);
	return result;
}

static HDC        g_DC;
static HGLRC      g_GLRC;
static HWND       g_Wnd;
static int        suspendRender;
static int        keys[KEY_MAX_KEYS];
static Event      _event;
static int        window_width;
static int        window_height;
static int        mouse_x;
static int        mouse_y;

void win32_internal_setPixelFormat( HDC hdc )
{
		PIXELFORMATDESCRIPTOR pfd = {
		sizeof( PIXELFORMATDESCRIPTOR ),
		1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER | PFD_TYPE_RGBA,
		16,
		0, 0, 0, 0, 0, 0,
		0,
		0,
		0,
		0, 0, 0, 0,
		16,
		0,
		0,
		PFD_MAIN_PLANE,
		0,
		0, 0, 0
	};
	
	int iPixelFormat = ChoosePixelFormat( hdc, &pfd );
	
	if ( !iPixelFormat ) {
		MessageBox(NULL, TEXT("Could not get Pixelformat!"), TEXT("OH NO!"), MB_OK);
		exit(-1);
	}

	DescribePixelFormat( hdc, iPixelFormat, sizeof(pfd), &pfd );

	BOOL success = SetPixelFormat( hdc, iPixelFormat, &pfd );
	if ( !success ) {
		// TODO: GetErrorMsg!
		MessageBox(NULL, TEXT("Could not set pixel format!"), TEXT("OH NO!"), MB_OK);
		exit(-1);
	}
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT result = 0;

	switch(uMsg) {
		case WM_DESTROY:
		{
			//megEvent.type = MEG_EVENT_DESTROY;
			PostQuitMessage(0);
		}
		break;

		case WM_CREATE:
		{
			_event.type = EVENT_CREATE;
			g_DC = GetDC(hwnd);
			win32_internal_setPixelFormat( g_DC );
			if ( g_GLRC = wglCreateContext( g_DC ) ) {
				wglMakeCurrent( g_DC, g_GLRC );
			}
		}
		break;
	/*	
		case WM_PAINT:
		{				
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hwnd, &ps);
			//FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_GRAYTEXT+1));
			//drawScene();
			//SwapBuffers( g_DC );
			EndPaint(hwnd, &ps);	
		}
		break;
	*/	

		case WM_SIZE: 
		{				
			RECT rect;
			GetClientRect(g_Wnd, &rect);
			window_width  = rect.right;
			window_height = rect.bottom;

			_event.type = EVENT_SIZE;
			if (wParam == SIZE_MINIMIZED) {
				suspendRender = 1;
			}
			else if ( (wParam == SIZE_RESTORED) || (wParam == SIZE_MAXIMIZED) ) {
				suspendRender = 0;
			}
		}
		break;

		case WM_KEYDOWN:
		{
			switch(wParam)
			{
			case 0x41     :                 { keys[KEY_A]      = 1; }       break;
			case 0x42     :                 { keys[KEY_B]      = 1; }       break;
			case VK_LEFT:                 { keys[KEY_LEFT] = 1; }       break;
			case VK_RIGHT:               { keys[KEY_RIGHT] = 1; }    break;
			case VK_UP:                    { keys[KEY_UP] = 1; }         break;
			case VK_DOWN:               { keys[KEY_DOWN] = 1; }    break;
			case VK_ESCAPE:             { keys[KEY_ESCAPE] = 1; } break;
			default:                            { keys[KEY_NONE]   = 1; } 
			}		
		}
		break;

		case WM_KEYUP:
		{
			switch(wParam)
			{
			case 0x41     : { keys[KEY_A]      = 0; } break;
			case 0x42     : { keys[KEY_B]      = 0; } break;
			case VK_LEFT:                 { keys[KEY_LEFT] = 0; }       break;
			case VK_RIGHT:               { keys[KEY_RIGHT] = 0; }    break;
			case VK_UP:                    { keys[KEY_UP] = 0; }         break;
			case VK_DOWN:               { keys[KEY_DOWN] = 0; }    break;
			case VK_ESCAPE: { keys[KEY_ESCAPE] = 0; } break;
			default       : { keys[KEY_NONE]   = 0; } 
			}		
		}
		break;

		case WM_LBUTTONDOWN:
		{
			keys[ KEY_LMB ] = 1; 
			keys[ KEY_LMB_UP ] = 0;
		} break;
		case WM_LBUTTONUP:
		{
			keys[ KEY_LMB ] = 0;
			keys[ KEY_LMB_UP ] = 1;
		} break;

		case WM_MBUTTONDOWN:
		{
			keys[ KEY_MMB ] = 1;
		} break;
		case WM_MBUTTONUP:
		{
			keys[ KEY_MMB ] = 0;
		} break;

		case WM_RBUTTONDOWN:
		{
			keys[ KEY_RMB ] = 1;
		} break;
		case WM_RBUTTONUP:
		{
			keys[ KEY_RMB ] = 0;
		} break;

		case WM_MOUSEMOVE:
		{
			mouse_x = LOWORD(lParam);
			mouse_y = HIWORD(lParam);
		}
		break;

		default:
		{
			result = DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
	}
	
	return result;
}

int win32_create_window(int width, int height, char const * title, int show)
{
	HINSTANCE hInstance = GetModuleHandle(NULL);
	WNDCLASS wc         = { 0 };

	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = "Window Class";
	wc.style         = CS_OWNDC | CS_VREDRAW | CS_HREDRAW;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wc.hIcon         = LoadIcon(hInstance, "staricon");
	wc.hbrBackground = NULL;

	RegisterClass(&wc);

	g_Wnd = CreateWindowEx(
		0,
		(char*)"Window Class",
		title,
		// WS_CLIPCHILDREN | WS_CLIPSIBLINGS required by OpenGL (see SuperBible p. 116)
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,

		CW_USEDEFAULT, CW_USEDEFAULT,      // upper left origin 
		width, height, 

		NULL,
		NULL,
		hInstance,
		NULL
		);

	if (g_Wnd == NULL) {
		return 0;
	}

	ShowWindow(g_Wnd, show);
	SetForegroundWindow(g_Wnd);
	SetFocus(g_Wnd);

	// FullScreen Stuff
	// see: https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353
	/*
	DWORD dwStyle = GetWindowLong(g_Wnd, GWL_STYLE);
	WINDOWPLACEMENT g_wpPrev = { sizeof(g_wpPrev) };
	MONITORINFO mi = { sizeof(mi) };
	if ( GetMonitorInfo(MonitorFromWindow(g_Wnd, MONITOR_DEFAULTTOPRIMARY), &mi) ) 
	{
		SetWindowLong(g_Wnd, GWL_STYLE, dwStyle & ~WS_OVERLAPPEDWINDOW);
		SetWindowPos(g_Wnd, HWND_TOP,
			mi.rcMonitor.left, mi.rcMonitor.top,
			mi.rcMonitor.right - mi.rcMonitor.left,
			mi.rcMonitor.bottom - mi.rcMonitor.top,
			SWP_NOOWNERZORDER | SWP_FRAMECHANGED);
	}
	*/
	return 0;		
}

void win32_destroy_window(void)
{
	wglMakeCurrent( NULL, NULL );
	ReleaseDC( g_Wnd, g_DC );
	wglDeleteContext( g_GLRC );
	DestroyWindow(g_Wnd);
}

int win32_process_events(Event * event)
{	
	_event.type = EVENT_NONE;
 	MSG msg = { 0 };
	if ( PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) ) {

		if (msg.message == WM_QUIT) 
		{
			event->type = EVENT_QUIT;	
			return  1;
		}		
		
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		
		event->type = _event.type;

		return 1;
	} 
	
	return 0;
}

void win32_swap_buffers(void)
{
	SwapBuffers( g_DC );	
}

int win32_internal_translate_keycode(Key key)
{
	switch(key)
	{
	case    KEY_A     : return 0x41;
		// more keys...
	case    KEY_ESCAPE: return VK_ESCAPE;
	default			  : return KEY_NONE;
	}
}

int win32_is_key_down(Key key)
{
	return keys[key];
	//int win32_vk_key = win32_internal_translate_keycode(key);
	//return GetAsyncKeyState(win32_vk_key);
}

int win32_is_key_up(Key key)
{
	switch (key)
	{
		case KEY_LMB: 
		{
			if (keys[KEY_LMB_UP])
			{
				keys[KEY_LMB_UP] = 0;
				return 1;
			}
		} break;
		default: return 0;
	}

	return 0; // cannot reach
}

int win32_internal_str_len(char * str)
{	
	char * c  = str;
	int count = 0;
	while ( *c++ != '\0') count++;
	return count;
}

void win32_draw_text(int x, int y, char * text)
{
	TextOut( g_DC, x, y, text, win32_internal_str_len(text) );
}

__int64 win32_ticks_per_second(void)
{
	LARGE_INTEGER freq;
	QueryPerformanceFrequency(&freq);
	return freq.QuadPart;
}

__int64 win32_ticks(void)
{
	LARGE_INTEGER ticks;
	QueryPerformanceCounter(&ticks);
	return ticks.QuadPart;
}

/* Joystick Stuff is *very* experimental! */
int win32_joystick_1_ready(void)
{
	unsigned int num_devices = joyGetNumDevs();
	JOYINFO joyinfo;
	unsigned int result = joyGetPos(JOYSTICKID1, &joyinfo);
	if (result == JOYERR_UNPLUGGED) return 0;
	return 1;
}

Joystick win32_joystick_pos(void)
{
	Joystick joystick;
	JOYINFO joyinfo;
	joyGetPos(JOYSTICKID1, &joyinfo);
	joystick.xPos = joyinfo.wXpos;
	joystick.yPos = joyinfo.wYpos;
	joystick.zPos = joyinfo.wZpos;
	return joystick;
}

Position2D win32_mouse_pos(void)
{
	Position2D pos;
	pos.x = mouse_x;
	pos.y = mouse_y;
	return pos;
}

#elif __APPLE__

void mac_read_file_binary(char const * filename, char ** out_buffer, int * out_size)
{

}

#elif __linux__

#endif 


Dimensions window_dimensions(void)
{
	Dimensions dim;
	dim.width  = window_width;
	dim.height = window_height;

	return dim;
}

void init_platform(Platform * p)
{
	/* Platform specific */
#ifdef _WIN32
	p->read_file         = win32_read_file;
	p->get_exe_path      = win32_get_exe_path;
	p->initialize_memory = win32_initialize_memory;	
	p->create_window     = win32_create_window;
	p->destroy_window    = win32_destroy_window;
	p->process_events    = win32_process_events;
	p->swap_buffers      = win32_swap_buffers;
	p->is_key_down       = win32_is_key_down;
	p->is_key_up         = win32_is_key_up;
	p->draw_text         = win32_draw_text;
	p->ticks_per_second  = win32_ticks_per_second;
	p->ticks             = win32_ticks;
	p->joystick_1_ready  = win32_joystick_1_ready;
	p->joystick_pos      = win32_joystick_pos;
	p->mouse_pos         = win32_mouse_pos;

#elif __APPLE__
	p->read_file         = mac_read_file;
	p->get_exe_path      = mac_get_exe_path;

#elif __linux__


#endif

	/* Platform independent*/
	p->window_dimensions = window_dimensions;

	memset(keys, 0, KEY_MAX_KEYS*sizeof(int));
}
