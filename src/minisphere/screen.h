#ifndef MINISPHERE__DISPLAY_H__INCLUDED
#define MINISPHERE__DISPLAY_H__INCLUDED

#include "image.h"
#include "transform.h"

typedef struct screen screen_t;

screen_t*        screen_new               (const char* title, image_t* icon, int x_size, int y_size, int frameskip, bool avoid_sleep);
void             screen_free              (screen_t* it);
image_t*         screen_backbuffer        (const screen_t* it);
ALLEGRO_DISPLAY* screen_display           (const screen_t* it);
bool             screen_fullscreen        (const screen_t* it);
bool             screen_is_skipframe      (const screen_t* it);
void             screen_set_fullscreen    (screen_t* it, bool fullscreen);
uint32_t         screen_now               (const screen_t* it);
int              screen_get_frameskip     (const screen_t* it);
void             screen_get_mouse_xy      (const screen_t* it, int* o_x, int* o_y);
void             screen_set_frameskip     (screen_t* it, int max_skips);
void             screen_set_mouse_xy      (screen_t* it, int x, int y);
void             screen_draw_status       (screen_t* it, const char* text, color_t color);
void             screen_flip              (screen_t* it, int framerate);
image_t*         screen_grab              (screen_t* it, int x, int y, int width, int height);
void             screen_queue_screenshot  (screen_t* it);
void             screen_resize            (screen_t* it, int x_size, int y_size);
void             screen_show_mouse        (screen_t* it, bool visible);
void             screen_toggle_fps        (screen_t* it);
void             screen_toggle_fullscreen (screen_t* it);
void             screen_unskip_frame      (screen_t* it);

#endif // MINISPHERE__DISPLAY_H__INCLUDED
