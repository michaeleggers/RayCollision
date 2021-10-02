#define _CRT_SECURE_NO_WARNINGS

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

#include "platform.h"
#include "gr_draw.h"
#include "tr_math.h"
#include "gui.h"

#define  STBI_NO_SIMD // PIII does not have SSE2
#include "stb_image.h"

#define WINDOW_W     1920
#define WINDOW_H     1080
#define NUM_STARS    2048
#define MAP_SIZE     25

extern HWND g_Wnd;


typedef struct HitPoint
{
	vec2  pos;       // absolute pos
	float distance;
	int   is_walkable;
} HitPoint;

typedef enum AnimationState
{
	ANIM_IDLE,
	ANIM_WALK_LEFT,
	ANIM_WALK_RIGHT,
	ANIM_WALK_UP,
	ANIM_WALK_DOWN,
	ANIM_MAX
} AnimationState;

typedef enum MovementState
{
	MOVE_IDLE,
	MOVE_LEFT,
	MOVE_RIGHT,
	MOVE_UP,
	MOVE_DOWN,
	MOVE_MAX
} MovementState;

typedef struct Animation
{
	int from, to;
} Animation;

typedef enum Facing
{
	FACING_LEFT,
	FACING_RIGHT,
	FACING_UP,
	FACING_DOWN
} Facing;

typedef struct MeRect
{
	float x, y;
	float width, height;
} MeRect;

typedef struct Entity
{
	vec2              pos;
	float             velocity; 
	vec2i             direction;
	Facing            facing;
	Spritesheet       spritesheet;
	int               current_frame;
	float             duration_per_frame;
	float             current_time;
	int               is_moving;
	int               flipped;

	Animation         animations[ ANIM_MAX ];
	AnimationState    current_animation;
	

	MeRect            collision_box;  // relative to x, y
	int               animation_bits;
	int               movement_bits;
} Entity;

typedef struct TitleScreenBitmap
{
	vec3              pos;
	float             angle;
	int               rot_dir;
	float             size;
	Spritesheet       spritesheet;
} TitleScreenBitmap;

static  TitleScreenBitmap    stars[NUM_STARS];
static char g_map[ MAP_SIZE ] = 
{
	's','s','w','s','s',
	's','w','s','w','s',
	's','s','w','s','s',
	's','s','w','w','s',
	's','s','s','s','s'
};

static Platform p;

void init_starfield(Spritesheet spritesheet)
{
	for (int i=0; i<NUM_STARS; ++i) {
		stars[ i ].pos.x = rand_between(-WINDOW_W, WINDOW_W);
		stars[ i ].pos.y = rand_between(-WINDOW_H, WINDOW_H);
		stars[ i ].pos.z = rand_between(-4500, 0);
		stars[ i ].angle = rand_between(0.0f, 360.0f);
		int rot_dir = (int)rand_between(0.0f, 1.99f);
		stars[ i ].rot_dir = rot_dir ? 1 : -1;
		stars[ i ].size = rand_between(50.0f, 90.0f);
		stars[ i ].spritesheet = spritesheet;
	}
}

void move(Entity * entity, float dt)
{
	if ( entity->movement_bits & (1 << MOVE_LEFT) ) {
		entity->pos.x -= dt*entity->velocity;
	}
	if ( entity->movement_bits & (1 << MOVE_RIGHT) ) {
		entity->pos.x += dt*entity->velocity;
	}
	if ( entity->movement_bits & (1 << MOVE_UP) ) {
		entity->pos.y -= dt*entity->velocity;
	}
	if ( entity->movement_bits & (1 << MOVE_DOWN) ) {
		entity->pos.y += dt*entity->velocity;
	}
}

void animate_bitmap_starfield(float width, float height, float dt)
{
	gr_enable_depth_test();
	gr_set_viewport(0, 0, width, height);
	gr_set_perspective(45.0f, width/height, 1.1f, 5000.0f);
	for (int i=0; i<NUM_STARS; ++i) {
		// move star along z axis.
		TitleScreenBitmap * s = &(stars[i]);
		s->pos.z += (0.1f * dt);
		if (s->pos.z >= 0.f) s->pos.z = -4500.0f;

		s->angle += (float)(s->rot_dir)*(0.02f * dt);
		if       (s->angle > 360.0f) s->angle = 0.0f;
		else if (s->angle < 0.0f) s->angle = 360.0f;
		
		float visibility =  1.0f - (-1.0f * s->pos.z) / 4500.0f;

		// draw
		gr_draw_frame_perspective(s->spritesheet, s->pos.x, s->pos.y, s->pos.z, s->size, s->angle, visibility, 0);
	}
	gr_disable_depth_test();
}

int check_aabb(float x0, float y0, float w0, float h0,
	                   float x1, float y1, float w1, float h1)
{
	if (x0+w0 >= x1 && x0 <= x1+w1 &&
		y0+h0 >= y1 && y0 <= y1+h1) return 1;
	return 0;
}

int is_tile_walkable(int tile_x, int tile_y)
{
	if ( (tile_x >= 0) && (tile_x < 5) && (tile_y >= 0) && (tile_y < 5) ) {			

		if ( g_map[ 5*tile_y + tile_x ] == 'w' )
		{
			return 0;
		}
		return 1;
	}
	return 1; // make player walkable in undefined map area
}

// Transform a point p from screenspace (window coordinates) to worldspace
vec2 screenspace_to_worldspace(float screen_x, float screen_y,
							   float screen_width, float screen_height, 							
							   mat4 view_matrix, mat4 proj_matrix, vec2 p)
{
	// compute normalized device coords
	float x_nd = (p.x)/(screen_width/2.0f) - 1.0f;
	float y_nd = -((p.y)/(screen_height/2.0f) - 1.0f);
	// make a v4 so we can use matrix computation
	vec4 p_nd = { x_nd, y_nd, 0.0f, 1.0f };

	// go back from ndc to worldspace
	mat4 view_matrix_inv = mat4_inverse(view_matrix);
	mat4 proj_matrix_inv = mat4_inverse(proj_matrix);
	mat4 proj_x_view = mat4_x_mat4(view_matrix_inv, proj_matrix_inv);
	vec4 p_world = mat4_x_vec4(proj_x_view, p_nd);
	p_world.y *= -1.0f; // HACK: the coordinate system is fucked up

	return {p_world.x, p_world.y};
}

vec2 worldspace_to_tilespace(vec2 p)
{
	vec2 tilepos;

	if (p.x < 0.0f) {
		int tileCountX = int(-p.x / 32.0f);
		p.x *= -1.0f;
		p.x  -= (float)tileCountX*32.0f;
		p.x = 32.0f - p.x;
	}
	if (p.y < 0.0f) {
		int tileCountY = int(-p.y / 32.0f);
		p.y *= -1.0f;
		p.y -= (float)tileCountY*32.0f;
		p.y = 32.0f - p.y;
	}
	
	tilepos.x = p.x / 32.0f;
	tilepos.y = p.y / 32.0f;
	tilepos.x = tilepos.x - (int)tilepos.x;
	tilepos.y = tilepos.y - (int)tilepos.y;
	tilepos.x *= 32.0f;
	tilepos.y *= 32.0f;
	
	return tilepos;
}

vec2i worldspace_to_tilenumbers(vec2 p)
{
	vec2i tilenumbers;
	int fract_x = (tr_fract(p.x/32.0f) > 0.0f) ? 1 : 0;
	int fract_y = (tr_fract(p.y/32.0f) > 0.0f) ? 1 : 0;
	tilenumbers.x = (int)(p.x / 32.0f);
	tilenumbers.y = (int)(p.y / 32.0f);

	if (p.x < 0.0f)
		tilenumbers.x -= 1;
	if (p.y < 0.0f)
		tilenumbers.y -= 1;

	return tilenumbers;
}

vec2i adjust_tilenumbers_to_direction(vec2i tilenumbers, vec2 dir)
{
	vec2i mod_tilenumbers;
	mod_tilenumbers.x = tilenumbers.x;
	mod_tilenumbers.y = tilenumbers.y;

	if (dir.x < 0.0f && dir.y < 0.0f) { // right to left, bottom to top
		mod_tilenumbers.x -= 1;
	}
	else if (dir.x >= 0.0f && dir.y < 0.0f) { // left to right, bottom to top
		mod_tilenumbers.y -= 1;
	}

	return mod_tilenumbers;
}

MeRect rect_for_tile(int x, int y)
{
	MeRect box;
	box.x = 32.0f*(float)x;
	box.y = 32.0f*(float)y;
	box.width  = 32.0f;
	box.height = 32.0f;
	return box;
}

HitPoint linesegment_vs_map(vec2 pos, vec2 dir, float length)
{
	HitPoint result;
	result.distance    = length;
	result.pos         = vec2_add(pos, vec2_mul(length, dir));
	result.is_walkable = 1;

	vec2 posTileSpace = worldspace_to_tilespace(pos);

	// First intersection with vertical grid-line
	float dx = 32.0f - posTileSpace.x;
	int dirX = 1;
	if (dir.x < 0.0f) {
		dx   = -posTileSpace.x;
		dirX = -1;
	}
	float firstVerticalX = pos.x + dx;
	float mx = dir.y / dir.x;
	float firstVerticalY = pos.y + mx*dx;
	float initVerticalDist = vec2_length( vec2_sub( {firstVerticalX, firstVerticalY}, pos ) );

	// First intersection with horizontal grid-line
	float dy = 32.0f - posTileSpace.y;
	int dirY = 1;
	if (dir.y < 0.0f) {
		dy   = -posTileSpace.y;
		dirY = -1;
	}
	float firstHorizontalY = pos.y + dy;
	float my = dir.x / dir.y;
	float firstHorizontalX = pos.x + my*dy;
	float initHorizontalDist = vec2_length( vec2_sub( {firstHorizontalX, firstHorizontalY}, pos) );

	// next vert. intersection
	float stepY = (float)dirY * 32.0f * tanf( acosf( vec2_dot(dir, {(float)dirX, 0}) ) );
	float nextVerticalDist = vec2_length( {32.0f, stepY} );

	// next hrzt. intersection
	float stepX = (float)dirX * 32.0f * tanf( acosf( vec2_dot(dir, {0, (float)dirY}) ) );
	float nextHorizontalDist = vec2_length( {stepX, 32.0f} );

	vec2i testTileCoords = worldspace_to_tilenumbers(pos);
	char buf[32] = { '\0' };
	int i = 0;
	result.distance = length;
	vec2 currentPos = result.pos;
	while ( (initVerticalDist < length) || (initHorizontalDist < length) ) {
		sprintf(buf, "%d", i);
		if (initVerticalDist < initHorizontalDist) {
			currentPos = {firstVerticalX, firstVerticalY};
			gr_draw_rect(firstVerticalX - 2.0f, firstVerticalY - 2.0f, 4.0f, 4.0f, 3.0f, {1,1,0});
			gr_draw_text(buf, firstVerticalX, firstVerticalY - 10.0f, 0.5f);
			firstVerticalX += (float)dirX*32.0f;
			firstVerticalY += stepY;
			initVerticalDist += nextVerticalDist;
			testTileCoords.x += dirX;
		}
		else {
			currentPos = {firstHorizontalX, firstHorizontalY};
			gr_draw_rect(firstHorizontalX - 2.0f, firstHorizontalY - 2.0f, 4.0f, 4.0f, 3.0f, {1,0,1});
			gr_draw_text(buf, firstHorizontalX, firstHorizontalY + 2.0f, 0.5f);
			firstHorizontalX += stepX;
			firstHorizontalY += (float)dirY*32.0f;
			initHorizontalDist += nextHorizontalDist;
			testTileCoords.y += dirY;
		}

		result.pos      = currentPos;
		if ( !is_tile_walkable(testTileCoords.x, testTileCoords.y) ) {
			result.distance = vec2_length( vec2_sub(currentPos, pos) );
			result.is_walkable = 0;
			gr_draw_rect(testTileCoords.x*32.0f, testTileCoords.y*32.0f, 32.0f, 32.0f, 4.0f, {1,0,0});
			break;
		}

		i++;
		gr_draw_rect(testTileCoords.x*32.0f, testTileCoords.y*32.0f, 32.0f, 32.0f, 4.0f, {0,1,0});
	}

	return result;
}

typedef enum MouseState
{
	MS_CLICKED,
	MS_DRAG,
	MS_RELEASED
} MouseState;

//void gui_begin(bool (*mouse_left_released_callback)(void), GUIPoint (*mouse_pos_callback)(void))
bool gui_mouse_left_released(void) 
{
	if (p.is_key_up(KEY_LMB))
		return true;
	
	return false;
}

GUIPoint gui_mouse_pos(void)
{
	Position2D mouse_pos = p.mouse_pos();
	return {
		(float)mouse_pos.x,
		(float)mouse_pos.y
	};
}

int main(int argc, char ** argv)
{	
	init_platform(&p);
	
	char exe_path[256];
	p.get_exe_path(exe_path, 256);

	p.create_window(WINDOW_W, WINDOW_H, "Hello Window!", 1);
	// FreeConsole(); // hides the console.

	if ( p.joystick_1_ready() )
		printf("joystick 1 ready!\n");

	gr_init();

	// OpenGL has uv original bottom left. Most images are stored origin at top-left. => flip it on load.
	stbi_set_flip_vertically_on_load(0);
	int x, y, n;
	unsigned char * image = stbi_load("assets/circle.png", &x, &y, &n, 4);
	Texture texture = gr_create_texture(image, x, y, 4);

	image = stbi_load("assets/spritesheet.png", &x, &y, &n, 4);
	Texture spritesheet_texture = gr_create_texture(image, x, y, 4);
	Spritesheet spritesheet = gr_create_spritesheet(
		spritesheet_texture,
		0, 0,   // x, y start offset in texture
		32, 32, // width, height of a frame
		4);     // how many frames


	// Map
	image = stbi_load("assets/map_sheet.png", &x, &y, &n, 4);
	Texture map_tiles_texture = gr_create_texture(image, x, y, 4);
	Spritesheet map_tilesheet = gr_create_spritesheet(
		map_tiles_texture,
		0, 0,
		32, 32,
		2);

	// Player Sprite
	image = stbi_load("assets/guy5.png", &x, &y, &n, 4);
	Texture player_texture = gr_create_texture(image, x, y, 4);
	Spritesheet player_spritesheet = gr_create_spritesheet(
		player_texture,
		0, 0,
		32, 32,
		2);


	// Virus
	image = stbi_load("assets/virus3.png", &x, &y, &n, 4);
	Texture virus_texture = gr_create_texture(image, x, y, 4);
	Spritesheet virus_spritesheet = gr_create_spritesheet(
		virus_texture,
		0, 0,
		32, 32,
		1);
	init_starfield(virus_spritesheet);

	Entity player = { {32.0f, 0.0f}, 2.13741f, {0, 0}, FACING_RIGHT, player_spritesheet, 0, 100.0f, 0 };
	player.animations[ ANIM_IDLE              ] = { 0, 0 };
	player.animations[ ANIM_WALK_RIGHT ] = { 0, 1 }; // from, to
	player.animations[ ANIM_WALK_LEFT   ] = { 0, 1 };
	player.animations[ ANIM_WALK_UP      ] = { 0, 1 };
	player.animations[ ANIM_WALK_DOWN ] = { 0, 1 };
	player.collision_box = { 12.0, 25.0, 7.0, 6.0 };
	player.animation_bits = 0;

	// Camera for zooming and positioning the map
	Camera camera = { {0,0,1}, {0,0,0}, {0,1,0} };

	int running = 1;
	float dt = 0.0f;
	__int64 start_time, end_time, elapsed_microsecs;
	__int64 freq = p.ticks_per_second();
	float fps = 0.0f;
	float ms  = 0.0f;
	Position2D mouse_pos = { 0, 0 };
	float mouse_dx = (float)p.window_dimensions().width; // HACK: so that the rendering is not super tiny at startup.

	vec2 mouse_a_sspace = {0,0};
	vec2 mouse_b_sspace = {0,0};
	MouseState mouse_state = MS_RELEASED;
	vec2 mouse_a_wspace = { 0, 0 };
	vec2 mouse_b_wspace = { 0, 0 };
	while (running) {

		start_time = p.ticks();
		
		gr_clear_buffers();

		Event event;
		while (p.process_events(&event)) {	
			if ( event.type == EVENT_QUIT )
				running = 0;
			if ( event.type == EVENT_SIZE )
				printf("Size event happened.\n");		
		}	

		if ( p.is_key_down(KEY_ESCAPE) ) {
			running = 0;
		}
		
		if ( p.is_key_down(KEY_RMB) ) {
			printf("Right mouse button down\n");
			Position2D new_mouse_pos = p.mouse_pos();
			mouse_dx += 5.0f*((float)new_mouse_pos.x - (float)mouse_pos.x);
			if (mouse_dx < 0.0f) 
				mouse_dx = 0.0f;
			mouse_pos = p.mouse_pos();
		}
		else {
			mouse_pos = p.mouse_pos();
		}

		if ( p.is_key_up(KEY_LMB) ) {
			Position2D mouse_pos = p.mouse_pos();
			if (mouse_state == MS_DRAG) {
				mouse_state = MS_RELEASED;
			}
			else if (mouse_state == MS_RELEASED) {
				mouse_a_sspace = { (float)mouse_pos.x, (float)mouse_pos.y };
				mouse_b_sspace = { (float)mouse_pos.x, (float)mouse_pos.y };
				mouse_state = MS_CLICKED;
			}
		}
		if ( p.is_key_down(KEY_LMB) ) {
			if (mouse_state == MS_CLICKED) {
				mouse_state = MS_DRAG;
			}
			if (mouse_state == MS_DRAG) {
				Position2D mouse_pos = p.mouse_pos();
				mouse_b_sspace = { (float)mouse_pos.x, (float)mouse_pos.y };
			}
		}

		if ( p.is_key_down(KEY_RIGHT) ) {
			player.current_animation = ANIM_WALK_RIGHT;
			player.movement_bits |= ( 1 << MOVE_RIGHT );
			player.flipped = 0;
		}
		else {
			player.movement_bits &= ~( 1 << MOVE_RIGHT );
		}
		if ( p.is_key_down(KEY_LEFT) ) {
			player.current_animation = ANIM_WALK_LEFT;
			player.movement_bits |= ( 1 << MOVE_LEFT );
			player.flipped = 1;
		}
		else {
			player.movement_bits &= ~( 1 << MOVE_LEFT );
		}
		if ( p.is_key_down(KEY_UP) ) {
			player.current_animation = ANIM_WALK_UP;		
			player.movement_bits |= ( 1 << MOVE_UP );
		}
		else {
			player.movement_bits &= ~( 1 << MOVE_UP );
		}
		if ( p.is_key_down(KEY_DOWN) ) {
			player.current_animation = ANIM_WALK_DOWN;
			player.movement_bits |= ( 1 << MOVE_DOWN );
		}
		else {
			player.movement_bits &= ~( 1 << MOVE_DOWN );
		}
		if ( player.movement_bits ) { // Play animation 
			player.is_moving = 1;
			if ( player.current_time >= player.duration_per_frame ) {
				player.current_time = 0.0f;
				player.current_frame++;
				int from = player.animations[ player.current_animation ].from;
				int to = player.animations[ player.current_animation ].to;
				if ( player.current_frame > to ) {
					player.current_frame = from;
				}
			}
			else {
				player.current_time += dt;
			}
		}
		else {
			player.current_animation = ANIM_IDLE;
			player.current_frame = 0;
			player.animation_bits = 0;
		}
		
		// Setup Camera shit.
		Dimensions dim = p.window_dimensions();
		float map_size_x = 5.0*32.0;
		float map_size_y = 5.0*32.0;
		float cam_zoom = (float)mouse_dx/(float)dim.width;
		camera.zoom =  cam_zoom * 10.0f;
		camera.pos.x = map_size_x / 2.0f;
		camera.pos.y = -map_size_y / 2.0f;
		camera.center.x = map_size_x / 2.0f;
		camera.center.y = -map_size_y / 2.0f;
		mat4 view_matrix = look_at( camera.pos, camera.center, camera.up );
		mat4 ortho_matrix = ortho( 
			-(float)dim.width/camera.zoom, (float)dim.width/camera.zoom,
			-(float)dim.height/camera.zoom, (float)dim.height/camera.zoom,
			-50.0f, 50.0f
		);
		gr_set_view_proj( view_matrix, ortho_matrix );		
		gr_set_viewport(0.0f, 0.0f, (float)dim.width, (float)dim.height);	

		// just a debug thingy for mouse position
		vec2 mouse_pos_world = screenspace_to_worldspace(
			0.0f, 0.0f, (float)dim.width, (float)dim.height,
			view_matrix, ortho_matrix, {(float)mouse_pos.x, (float)mouse_pos.y} );

		// Think
		// Figure out in which map-tile the player is in
		player.direction = { 0, 0 };
		if (player.movement_bits & (1<<MOVE_RIGHT)) { 
			player.direction.x += 1; 
		}
		if (player.movement_bits & (1<<MOVE_LEFT))  { 
			player.direction.x += -1; 
		}
		if (player.movement_bits & (1<<MOVE_UP))    { 
			player.direction.y += -1; 
		}
		if (player.movement_bits & (1<<MOVE_DOWN))  { 
			player.direction.y += 1; 
		}
		
		// TODO: Collision Detection
		vec2 movement = vec2_normalize({(float)player.direction.x, (float)player.direction.y});
		movement = vec2_mul(player.velocity, movement);
		movement = vec2_mul(dt, movement);
		if ( (player.direction.x != 0) || (player.direction.y != 0) ) {
			//vec2 new_player_pos = vec2_add(player.pos, movement);
			vec2 testPos = {
				player.pos.x + player.collision_box.x + player.collision_box.width/2.0f,
				player.pos.y + player.collision_box.y + player.collision_box.height/2.0f
			};
			
			HitPoint hit_x = linesegment_vs_map( testPos, {(float)player.direction.x, 0.0f}, fabsf(movement.x) );
			printf("hit x distance: %f\n", hit_x.distance);
			float walk_dx = (float)player.direction.x*hit_x.distance;
			printf("walk_dx: %f\n", walk_dx);
			float old_x = player.pos.x;
			player.pos.x += walk_dx;
			if ( !hit_x.is_walkable ) {
				if (player.direction.x == 1) {
					player.pos.x = floorf(player.pos.x) - 0.5f;
				}
				else if (player.direction.x == -1) {
					player.pos.x = ceilf(player.pos.x) + 0.5f;
				}
			}
			testPos.x += (player.pos.x - old_x);

			HitPoint hit_y = linesegment_vs_map( testPos, {0.0f, (float)player.direction.y}, fabsf(movement.y) );
			player.pos.y += float(player.direction.y)*hit_y.distance;
			if( !hit_y.is_walkable ) {
				if (player.direction.y == 1) {
					player.pos.y = floorf(player.pos.y) - 0.5f;
				}
				else if (player.direction.y == -1) {
					player.pos.y = ceilf(player.pos.y) + 0.5f;
				}
			}

		}
		
		//animate_bitmap_starfield( (float)dim.width, (float)dim.height, dt );

		// Draw Stuff
		for (int ty = 0; ty < 5; ++ty) {
			for (int tx = 0; tx < 5; ++tx) {				
				
				char tile = g_map[ ty*5 + tx ];
				switch (tile) 
				{
					case 's':  gr_draw_frame_ex(map_tilesheet, tx*32.0f, ty*32.0f, 1.0f, 0, 0); break;
					case 'w': gr_draw_frame_ex(map_tilesheet, tx*32.0f, ty*32.0f, 1.0f, 1, 0); break;
					case ' ':  break;
					default:  gr_draw_frame_ex(map_tilesheet, tx*32.0f, ty*32.0f, 32.0f, 0, 0);
				}
				gr_draw_rect(tx*32.0f, ty*32.0f, 32.0f, 32.0f, 1.0f, {1, 0, 0});
			}
		}

		// Draw Player
		gr_draw_frame_ex( player.spritesheet, player.pos.x, player.pos.y, 1.0f, player.current_frame, player.flipped );
		gr_draw_rect( player.pos.x, player.pos.y, 32.0f, 32.0f, 1.0f, {1.0, 1.0, 0.0});
		gr_draw_rect( player.pos.x + player.collision_box.x,
			player.pos.y + player.collision_box.y,
			player.collision_box.width, player.collision_box.height,
			1.0f, {0.0, 1.0, 0.0});


		HitPoint debug_hitpoint = { };
		if ( mouse_state == MS_DRAG || mouse_state == MS_RELEASED ) {
			mouse_a_wspace = screenspace_to_worldspace(0.0f, 0.0f, (float)dim.width, (float)dim.height, view_matrix, ortho_matrix, mouse_a_sspace);
			mouse_b_wspace = screenspace_to_worldspace(0.0f, 0.0f, (float)dim.width, (float)dim.height, view_matrix, ortho_matrix, mouse_b_sspace);
			vec2 dir = vec2_sub(mouse_b_wspace, mouse_a_wspace);
			float length = vec2_length(dir);
			dir = vec2_normalize(dir);
			gr_draw_line(mouse_a_wspace.x, mouse_a_wspace.y, mouse_b_wspace.x, mouse_b_wspace.y, 4.0f, {1,0,0});
			debug_hitpoint = linesegment_vs_map(mouse_a_wspace, dir, length);		
		}

		// ///////////////////////////////////////////////////////
		// GAME GRAPHICS DONE.
		// DRAW UI OVERLAY FROM HERE ON. ALL IN WINDOW COORDINATES.
		// ///////////////////////////////////////////////////////



		// Draw Text in Window Coordinates
		gr_set_window_dimensions_pixel_perfect( dim.width, dim.height );

		// GUI Test		
		gui_begin(gui_mouse_left_released, gui_mouse_pos);
		if ( gui_button("Click Me", 0, 0, 100, 100, __LINE__) ) {
			gr_draw_text("BUTTON WAS CLICKED!", 500, 500, 1.0f);
		}

		char text_buf[256];
		sprintf(text_buf, "FPS: %f", fps);
		gr_draw_text(text_buf, 0.0f, 0.0f, 1.0f);
		sprintf(text_buf, "Frametime: %fms", ms);
		gr_draw_text(text_buf, 0.0f, 1.5*20.0f, 1.0f);
		sprintf(text_buf, "There are %d stars in this system.", NUM_STARS);
		gr_draw_text(text_buf, 0.0f, 1.5*40.0f, 1.0f);
		sprintf(text_buf, "Executable Location is: %s", exe_path);
		gr_draw_text(text_buf, 0.0f, 1.5*60.0f, 1.0f);

		gr_draw_text("Player Pos:", 0.0f, 1.5*80.0f, 1.0f);
		//sprintf(text_buf, "    TileSpace (x, y):  %d, %d", tile_x, tile_y);	
		//gr_draw_text(text_buf, 0.0f, 1.5*90.0f, 1.0f);
		sprintf(text_buf, "    WorldSpace (x, y): %f, %f", player.pos.x, player.pos.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*100.0f, 1.0f);
		vec2 player_tilespace = worldspace_to_tilespace(player.pos);
		sprintf(text_buf, "    TileSpace (x, y): %f, %f", player_tilespace.x, player_tilespace.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*110.0f, 1.0f);

		gr_draw_text("Mouse Pos:", 0.0f, 1.5*130, 1.0f);
		sprintf(text_buf, "    ScreenSpace(x, y): %d, %d", mouse_pos.x, mouse_pos.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*140.0f, 1.0f);
		sprintf(text_buf, "    WorldSpace(x, y):  %f, %f", mouse_pos_world.x, mouse_pos_world.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*150.0f, 1.0f);

		sprintf(text_buf, "Mouse A SSpace (x, y): %f, %f", mouse_a_sspace.x, mouse_a_sspace.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*170.0f, 1.0f);
		sprintf(text_buf, "Mouse A WSpace (x, y): %f, %f", mouse_a_wspace.x, mouse_a_wspace.y);	
		gr_draw_text(text_buf, 0.0f, 1.5*180.0f, 1.0f);

		sprintf(text_buf, "Debug Hit Point distance: %f", debug_hitpoint.distance);
		gr_draw_text(text_buf, 0.0f, 1.5*200, 1.0f);

		gr_draw_text("upper right test text", dim.width - 180.0f, 0.0f, 1.0f);

		p.swap_buffers();
		glFinish(); // helps with mouse input lagging behind. TODO: not sure if this is actually a good solution!

		end_time = p.ticks();
		elapsed_microsecs = end_time - start_time;
		elapsed_microsecs *= 1000000;
		elapsed_microsecs /= freq;
		ms  = (float)elapsed_microsecs/1000.0f;
		fps = 1000.0f/ms;
		dt = ms;

		//p.draw_text(0, 40, text_buf);
		sprintf(text_buf, "Frametime (ms): %f.", ms);
		//p.draw_text(0, 60, text_buf);

	}

	p.destroy_window();

	return 0;
}