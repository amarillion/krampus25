#include "demo.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_primitives.h>
#include <allegro5/utf8.h>
#include "abort.h"
#include <stdio.h>
#include <list>
#include <string>
#include "dom.h"
#include <functional>
#include "strutil.h"
#include "multiline.h"
#include "rect.h"
#include "openLink.h"
#include "text.h"
#include "richtext.h"

using namespace std;

#ifdef __EMSCRIPTEN__

#define TEST_TEXT "<h1>Welcome to Allegro</h1>\n" \
	"\n" \
	"This <b>rich text</b> is <i>rendered</i>\n" \
	"in the browser using <a href=\"https://liballeg.org\">allegro</a>\n" \
	"and <a href=\"https://emscripten.org\">emscripten.</a>\n" \
	"\n" \
	"	Accented characters: á é í ó ú ü ñ ¿ ¡ are also supported\n" \
	"\n" \
	"	Cool eh? 😎\n\n"

#else
#define TEST_TEXT "<h1>Welcome to Allegro</h1>\n" \
	"\n" \
	"This <b>rich text</b> is <i>rendered</i>\n" \
	"using <a href=\"https://liballeg.org\">allegro</a>\n" \
	"on the desktop.\n" \
	"\n" \
	"Please open the browser version at\n" \
	"<a href=\"https://amarillion.github.io/krampus25\">https://amarillion.github.io/krampus25</a>\n"
#endif

class DemoImpl: public Demo {
	
	ALLEGRO_FONT *normal, *bold, *italic, *header;
	ALLEGRO_COLOR background, text, white;
	list<ComponentPtr> components;
	StyleData style;
	float xflow, yflow;

	virtual void init() {
		// ex.font = al_load_font("data/Caveat-VariableFont_wght.ttf", 24, 0);
		style.normal = al_load_font("data/DejaVuSans.ttf", 16, 0);
		style.bold = al_load_font("data/DejaVuSans-Bold.ttf", 16, 0);
		style.italic = al_load_font("data/DejaVuSans-Oblique.ttf", 16, 0);
		style.header = al_load_font("data/DejaVuSans-Bold.ttf", 24, 0);

		// ex.font = al_create_builtin_font();
		if (!style.normal || !style.bold || !style.italic || !style.header) {
			abort_example("Font not found\n");
		}
		printf("Font loaded.\n");
		background = al_color_name("beige");
		style.textColor = al_color_name("black");
		style.linkColor = al_color_name("blue");
		white = al_color_name("white");

		float x = 0;
		float y = 0;
		appendRichText(TEST_TEXT, &x, &y, 400, components, style);
	}

	virtual void draw() {
		ALLEGRO_BITMAP *screen;
		void *data;
		int size, i, format;
		
		// al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

		al_clear_to_color(background);

		screen = al_get_target_bitmap();

		GraphicsContext gc;
		gc.buffer = screen;
		gc.xofst = 0;
		gc.yofst = 0;

		for(auto &comp : components) {
			comp->draw(gc);
		}
	}

	virtual void update() {

	}

	virtual void openLinkAt(int x, int y) override {
		// for(auto &span : spans) {
		// 	if (span.type == TextSpan::TYPE_LINK) {
		// 		for (auto &rect : span.linkHotspots) {
		// 			if (rect.contains(Point(x, y))) {
		// 				printf("Opening link: %s\n", span.href.c_str());
		// 				// open in browser
		// 				openLink(span.href.c_str());
		// 				return;
		// 			}
		// 		}
		// 	}
		// }
	}

};

unique_ptr<Demo> Demo::newInstance() {
	return make_unique<DemoImpl>();
}
