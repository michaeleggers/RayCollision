#include <windows.h>

#include <gl\gl.h>
#include <gl\glu.h>

#include "gr_draw.h"
#include "chars.h"

#define  STB_IMAGE_IMPLEMENTATION
#define  STBI_NO_SIMD // PIII does not have SSE2
#include "stb_image.h"

#define  STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

static  unsigned char * g_fontpixels;
static  Texture         g_bmpfont_texture;
static  Spritesheet     g_fontsheet;

Texture gr_create_texture(unsigned char * data, int width, int height, int channels)
{
	Texture texture = { 0 };

	glGenTextures( 1, (GLuint *)&texture.id );
	glBindTexture( GL_TEXTURE_2D, texture.id ); // THIS CALL RIGHT HERE IS *VERY* IMPORTANT!
												// CALLS BELOW WILL HAVE NO EFFECT WHEN TEXTURE IS NOT
												// BOUND!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

	GLuint format;
	switch (channels)
	{
	case 1: format  = GL_RED;  break;
	case 3: format  = GL_RGB;  break;
	case 4: format  = GL_RGBA; break;
	default: format = GL_RGBA; break;
	}

	glTexImage2D( GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	texture.width    = width;
	texture.height   = height;
	texture.channels = channels;

	return texture;
}

void gr_init_bmpfont_texture(void)
{
	int char_pxl_width  = 8;
	int char_pxl_height = 16;
	int bytes_per_row   = 1;
	int pixel_count = CHARDATA_SIZE * char_pxl_width; // Each byte represents 8 horizontal pixels.
													  // TODO: Round pixel count to next power of 2. Square root of that pixel count is width and height.
	int width  = 256;
	int height = 256;
	// TODO: Figure out how many chars can fit in one row, col.
	int chars_per_row = width / char_pxl_width;  
	int chars_per_col = height / char_pxl_height; 
	g_fontpixels = (unsigned char*)malloc(4 * width * height * sizeof(unsigned char));
	memset(g_fontpixels, 0, 4*width*height*sizeof(unsigned char));

	int char_count;
	for (char_count = 0; char_count < NUM_CHARS; ++char_count) {
		int row = char_count / chars_per_row;
		int col = char_count % chars_per_row;
		for (int i = 0; i < 16; ++i) {
			for (int j = 0; j < 8; ++j) {
				int x_offset = col*8*4;
				int y_offset = row*16*width*4;
				int chardata_offset = char_count*16;
				unsigned char value = 0xFF * (((chardata[ chardata_offset + i ] << j) & 0x80) >> 7);
				g_fontpixels[ y_offset + x_offset + (i*width*4 + j*4+0)] = value;
				g_fontpixels[ y_offset + x_offset + (i*width*4 + j*4+1)] = value;
				g_fontpixels[ y_offset + x_offset + (i*width*4 + j*4+2)] = value;
				g_fontpixels[ y_offset + x_offset + (i*width*4 + j*4+3)] = value;
			}
		}
	}

	/* Debug Output of generated Bitmap-Font Texture */
	stbi_write_png("debug_chars.png", width, height, 4, g_fontpixels, width*4);    
	
	g_bmpfont_texture = gr_create_texture(g_fontpixels, 256, 256, 4);
	g_fontsheet = gr_create_spritesheet(g_bmpfont_texture,
		0, 0,
		8, 16,
		256);
}

// Set coordinate system with origin at the middle of the screen.
// Y is up.
void gr_set_view_proj(mat4 view_matrix, mat4 proj_matrix)
{
	//float aspect = height/width;
	
	glMatrixMode(GL_PROJECTION);
	glLoadMatrixf( &(proj_matrix.d[0][0]) );
	glMultMatrixf( &view_matrix.d[0][0] );
	
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

// Set coordinate system with upper left origin to (0, 0).
// Y is still up. So when drawing we have to invert Y! TODO: Fix this.
void gr_set_window_dimensions_pixel_perfect(int width, int height)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glViewport(0, 0, width, height);		
	glOrtho(
		0.0f, (float)width,       // left, right
		-(float)height, 0.0f,      // bottom,  top
		-50.0f, 50.0f
	);  

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void gr_set_viewport(float x, float y, float width, float height)
{
	glViewport( (int)x, (int)y, (int)width, (int)height );
}

void gr_set_ortho(float left, float right, float bottom, float top, float _near, float _far)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glOrtho( left, right, bottom, top, _near, _far );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void gr_set_perspective(float fovy, float aspect, float _near, float _far)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	gluPerspective(fovy, aspect, _near, _far);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void gr_enable_depth_test(void)
{
	glEnable(GL_DEPTH_TEST); 
	glDepthFunc(GL_LESS);
}

void gr_disable_depth_test(void)
{
	glDisable(GL_DEPTH_TEST);
}

void gr_clear_buffers(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_ACCUM_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
}

void gr_init(void)
{
	glShadeModel(GL_FLAT);
	//glEnable(GL_DEPTH_TEST); // If turned on, even with alpha testing, fragments coming last will be rejected!
	//glDepthFunc(GL_LESS);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_NONE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_ALPHA_TEST);
	glAlphaFunc(GL_GREATER, 0.0f);
	glFrontFace(GL_CW);
	glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
	glClearColor(0.f, 0.f, 0.2f, 1.f);

	gr_init_bmpfont_texture();
}

void gr_draw_texture(Texture texture)
{
	glEnable(GL_TEXTURE_2D);
	//glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL);
	glBindTexture(GL_TEXTURE_2D, texture.id);
	glBegin(GL_TRIANGLES);

	glTexCoord2f(0.f, 0.f);
	glVertex3f(-300, -300, 0);

	glTexCoord2f(0.f, 1.f);
	glVertex3f(-300, 300, 0);

	glTexCoord2f(1.f, 1.f);
	glVertex3f(300, 300, 0);

	glTexCoord2f(1.f, 1.f);
	glVertex3f(300, 300, 0);

	glTexCoord2f(1.f, 0.f);
	glVertex3f(300, -300, 0);

	glTexCoord2f(0.f, 0.f);
	glVertex3f(-300, -300, 0);

	glEnd();
	//glFlush();
	glDisable(GL_TEXTURE_2D);
}

Spritesheet gr_create_spritesheet(Texture texture,
	int x, int y, 
	int width, int height,
	int frame_count) 
{
	assert( frame_count < MAX_FRAMES );

	Spritesheet spritesheet = { 0 };
	spritesheet.texture     = texture;
	spritesheet.frame_count = frame_count;

	Frame * frame = spritesheet.frames;
	for (int i=0; i<frame_count; ++i) {
		frame->x      = (i*width) % texture.width;
		frame->y      = (i*width / texture.width) * height;
		frame->width  = width;
		frame->height = height;
		frame->aspect = (float)width / (float)height;
		frame++;
	}

	return spritesheet;
}

// Draw frame from spritesheet onto a unit-square, with origin at (0,0), that is, the middle of the square. So scaling factor will scale uniformly in both negative and positive directions for both x and y.
// y is inverted, so positive y is down for user.
void gr_draw_frame(Spritesheet spritesheet, float x, float y, float scale, int frame_number)
{
	Frame frame = spritesheet.frames[ frame_number ];
	float l = (float)frame.x / (float)spritesheet.texture.width;
	float b = (float)frame.y / (float)spritesheet.texture.height;
	float r = l + (float)frame.width / (float)spritesheet.texture.width;
	float t = b + (float)frame.height / (float)spritesheet.texture.height;
	float aspect = frame.aspect;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spritesheet.texture.id);
	glTranslatef(x, -y, 0.0f);
	glScalef(scale*aspect, scale, 1.0f);
	glBegin(GL_TRIANGLES);

	glTexCoord2f(l, t);
	glVertex3f(-0.5, -0.5, 0);

	glTexCoord2f(l, b);
	glVertex3f(-0.5, 0.5, 0);

	glTexCoord2f(r, b);
	glVertex3f(0.5, 0.5, 0);

	glTexCoord2f(r, b);
	glVertex3f(0.5, 0.5, 0);

	glTexCoord2f(r, t);
	glVertex3f(0.5, -0.5, 0);

	glTexCoord2f(l, t);
	glVertex3f(-0.5, -0.5, 0);

	glEnd();
	//glFlush();
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
}

void gr_draw_frame_perspective(Spritesheet spritesheet, float x, float y, float z, float scale, float angle, float visibility, int frame_number)
{
	Frame frame = spritesheet.frames[ frame_number ];
	float l = (float)frame.x / (float)spritesheet.texture.width;
	float b = (float)frame.y / (float)spritesheet.texture.height;
	float r = l + (float)frame.width / (float)spritesheet.texture.width;
	float t = b + (float)frame.height / (float)spritesheet.texture.height;
	float aspect = frame.aspect;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spritesheet.texture.id);
	glTranslatef(x, -y, z);
	glRotatef(angle, 0.0f, 0.0f, 1.0f);
	glScalef(scale, scale, 1.0f);
	glBegin(GL_TRIANGLES);

	glColor4f(1.0f, 1.0f, 1.0f, visibility);
	glTexCoord2f(l, t);
	glVertex3f(-0.5, -0.5, 0);

	glTexCoord2f(l, b);
	glVertex3f(-0.5, 0.5, 0);

	glTexCoord2f(r, b);
	glVertex3f(0.5, 0.5, 0);

	glTexCoord2f(r, b);
	glVertex3f(0.5, 0.5, 0);

	glTexCoord2f(r, t);
	glVertex3f(0.5, -0.5, 0);

	glTexCoord2f(l, t);
	glVertex3f(-0.5, -0.5, 0);

	glEnd();
	//glFlush();
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
	glColor4f(1.0f, 1.0f, 1.0f, 1.0f);
}

// Draw frame from spritesheet onto a unit-square, with origin at upper left corner.
// y is inverted, so positive y is down for user.
void gr_draw_frame_00(Spritesheet spritesheet, float x, float y, float scale, int frame_number, int flip)
{
	Frame frame = spritesheet.frames[ frame_number ];
	float l = (float)frame.x / (float)spritesheet.texture.width;
	float b = (float)frame.y / (float)spritesheet.texture.height;
	float r = l + (float)frame.width / (float)spritesheet.texture.width;
	float t = b + (float)frame.height / (float)spritesheet.texture.height;
	if (flip) {
		float temp = l;
		l = r;
		r = temp;
	}

	float aspect = frame.aspect;

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spritesheet.texture.id);
	glTranslatef(x, -y, 0.0f);
	glScalef(scale*aspect, scale, 1.0f);
	glBegin(GL_TRIANGLES);

	glTexCoord2f(l, b);
	glVertex3f(0, 0, 0);

	glTexCoord2f(r, b);
	glVertex3f(1, 0, 0);

	glTexCoord2f(l, t);
	glVertex3f(0, -1, 0);

	glTexCoord2f(l, t);
	glVertex3f(0, -1, 0);

	glTexCoord2f(r, b);
	glVertex3f(1, 0, 0);

	glTexCoord2f(r, t);
	glVertex3f(1, -1, 0);

	glEnd();
	//glFlush();
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
}

void gr_draw_rect(float x, float y, float width, float height, float line_width, vec3 color)
{
	y *= -1.0f;

	glLineWidth(line_width);
	glBegin(GL_LINES);

	glColor3f(color.x, color.y, color.z);
	glVertex3f(x, y, 0);
	glVertex3f(x + width, y, 0);
	glVertex3f(x + width, y, 0);
	glVertex3f(x + width, y - height, 0);
	glVertex3f(x + width, y - height, 0);
	glVertex3f(x, y - height, 0);
	glVertex3f(x, y - height, 0);
	glVertex3f(x, y, 0);

	glEnd();

	glLineWidth(1.0f);
	glColor3f(1.0, 1.0, 1.0);
}

void gr_draw_line(float x0, float y0, float x1, float y1, float width, vec3 color)
{
	y0 *= -1.0f;
	y1 *= -1.0f;

	glLineWidth(width);
	glBegin(GL_LINES);
	
	glColor3f(color.x, color.y, color.z);
	glVertex3f(x0, y0, 0);
	glVertex3f(x1, y1, 0);

	glEnd();

	glLineWidth(1.0f);
	glColor3f(1.0, 1.0, 1.0);
}

// Map frame of a spritesheet onto a rectangle with its upper left corner at (0,0).
// (width, height) extend to (-1, -1).
void gr_draw_frame_ex(Spritesheet spritesheet, float x, float y, float scale, int frame_number, int flip)
{
	Frame frame = spritesheet.frames[ frame_number ];
	float l = (float)frame.x / (float)spritesheet.texture.width;
	float b = (float)frame.y / (float)spritesheet.texture.height;
	float r = l + (float)frame.width / (float)spritesheet.texture.width;
	float t = b + (float)frame.height / (float)spritesheet.texture.height;
	float aspect = frame.aspect;
	if (flip) {
		float temp = l;
		l = r;
		r = temp;
	}

	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, spritesheet.texture.id);

	glTranslatef(x, -y, 0.f);
	glScalef(scale*frame.width, scale*frame.height, 1.0f);
	glBegin(GL_TRIANGLES);

	glTexCoord2f(l, b);
	glVertex3f(0, 0, 0);

	glTexCoord2f(r, b);
	glVertex3f(1, 0, 0);

	glTexCoord2f(l, t);
	glVertex3f(0, -1, 0);

	glTexCoord2f(l, t);
	glVertex3f(0, -1, 0);

	glTexCoord2f(r, b);
	glVertex3f(1, 0, 0);

	glTexCoord2f(r, t);
	glVertex3f(1, -1, 0);

	glEnd();
	//glFlush();
	glDisable(GL_TEXTURE_2D);
	glLoadIdentity();
}

void gr_draw_text(char const * text, float x, float y, float scale)
{
	int i = 0;
	float x_offset = 0.0f;
	glColor3f(1.0f, 1.0f, 1.0f);
	while ( *text != '\0' ) {
		gr_draw_frame_ex(g_fontsheet, x + x_offset, y, scale, *text, 0);		
		int width = g_fontsheet.frames[ *text ].width;
		x_offset += scale*(float)width;
		text++;
		i++;
	}
}