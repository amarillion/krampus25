#pragma once

struct ALLEGRO_FONT;
struct ALLEGRO_COLOR;

void draw_multiline_text(const ALLEGRO_FONT *font,
     ALLEGRO_COLOR color, float x, float y, float *xcursor, float *ycursor, float max_width, float line_height,
     int flags, const char *text);