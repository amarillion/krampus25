#include "demo.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include "abort.h"
#include <stdio.h>

using namespace std;

#define TEST_TEXT "This is utf-8 €€€€€ multi line text output with a\nhard break,\n\ntwice even!"

class DemoImpl: public Demo {
	
	ALLEGRO_FONT *font;
	ALLEGRO_COLOR background, text, white;

	double timer[4], counter[4];
	float text_x, text_y;

	void set_xy(float x, float y)
	{
		text_x = x;
		text_y = y;
	}

	void get_xy(float *x, float *y)
	{
		*x = text_x;
		*y = text_y;
	}

	virtual void init() {
		// ex.font = al_load_font("data/Caveat-VariableFont_wght.ttf", 16, 0);
		font = al_load_font("data/DejaVuSans-Bold.ttf", 16, 0);
		// ex.font = al_create_builtin_font();
		if (!font) {
			abort_example("Font not found\n");
		}
		printf("Font loaded.\n");
		background = al_color_name("beige");
		text = al_color_name("black");
		white = al_color_name("white");
	}

	void print(char const *format, ...)
	{
		va_list list;
		char message[1024];
		int th = al_get_font_line_height(font);
		va_start(list, format);
		vsnprintf(message, sizeof message, format, list);
		va_end(list);

		// al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_INVERSE_ALPHA);
		al_draw_textf(font, text, text_x, text_y, 0, "%s", message);
		text_y += th;
	}


	virtual void draw() {
		float x, y;
		int iw = 300;
		int ih = 300;
		ALLEGRO_BITMAP *screen;
		void *data;
		int size, i, format;
		
		// al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

		al_clear_to_color(background);

		screen = al_get_target_bitmap();

		set_xy(8, 8);

		/* Test 2. */
		print("Line 1");

		/* Test 3. */
		print("Line 2");

		/* Test 4. */
		print("Line 3");


	}

	virtual void update() {

	}

};

unique_ptr<Demo> Demo::newInstance() {
	return make_unique<DemoImpl>();
}
