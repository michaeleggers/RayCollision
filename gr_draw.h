#ifndef GR_DRAW_H
#define GR_DRAW_H

#include <gl\gl.h>
#include <gl\glu.h>

#include "tr_math.h"

#define MAX_FRAMES   1024

typedef struct Texture
{
	int     width, height, channels;
	int     id;
} Texture;

typedef struct Frame {
	int   x, y;
	int   width, height;
	float aspect;
} Frame;

typedef struct Spritesheet
{
	Texture texture;
	Frame frames[MAX_FRAMES];
	int frame_count;
} Spritesheet;

typedef struct Camera
{
	vec3 pos;
	vec3 center;
	vec3 up;
	float zoom;
} Camera;

Texture gr_create_texture(unsigned char * data, int width, int height, int channels);
void gr_init_bmpfont_texture(void);
void gr_set_view_proj(mat4 view_matrix, mat4 proj_matrix);
void gr_set_window_dimensions_pixel_perfect(int width, int height);
void gr_set_viewport(float x, float y, float width, float height);
void gr_set_ortho(float left, float right, float bottom, float top, float _near, float _far);
void gr_set_perspective(float fovy, float aspect, float _near, float _far);
void gr_enable_depth_test(void);
void gr_disable_depth_test(void);
void gr_clear_buffers(void);
void gr_init(void);
void gr_draw_texture(Texture texture);
Spritesheet gr_create_spritesheet(Texture texture,
	int x, int y, 
	int width, int height,
	int frame_count);
void gr_draw_frame(Spritesheet spritesheet, float x, float y, float scale, int frame_number);
void gr_draw_frame_perspective(Spritesheet spritesheet, float x, float y, float z, float scale, float angle, float visibility, int frame_number);
void gr_draw_frame_00(Spritesheet spritesheet, float x, float y, float scale, int frame_number, int flip);
void gr_draw_rect(float x, float y, float width, float height, float line_width, vec3 color);
void gr_draw_line(float x0, float y0, float x1, float y1, float width, vec3 color);
void gr_draw_frame_ex(Spritesheet spritesheet, float x, float y, float scale, int frame_number, int flip);
void gr_draw_text(char * text, float x, float y, float scale);


#endif
