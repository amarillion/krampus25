#pragma once


#ifdef __cplusplus
   extern "C" {
#endif

#include <allegro5/utf8.h>
#include <allegro5/allegro_font.h>

void draw_multiline_text(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float *xcursor, float *ycursor, float max_width, float line_height,
     int flags, const char *text);

void do_multiline_ustr(const ALLEGRO_FONT *font, 
   float *xcursor, float *ycursor, float line_height, float max_width,
   const ALLEGRO_USTR *ustr,
   bool (*cb)(int line_num, float xcursor, float ycursor, const ALLEGRO_USTR * line, void *extra),
   void *extra);

#ifdef __cplusplus
   }
#endif
