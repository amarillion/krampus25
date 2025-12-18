#include "demo.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include "abort.h"
#include <stdio.h>
#include <list>
#include <string>
#include "dom.h"
#include <functional>
#include "strutil.h"
#include "multiline.h"

using namespace std;

#define TEST_TEXT "<h1>📣 Notice 📣</h1>\n" \
	"\n" \
	"This <b>text</b> is <i>rendered</i>\n" \
	"	in the browser using <a href=\"https://liballeg.org\">allegro</a>\n" \
	"	and <a href=\"https://emscripten.org\">emscripten.</a>\n" \
	"\n" \
	"	We also support accented characters: á é í ó ú ü ñ ¿ ¡\n" \
	"\n" \
	"	Cool eh? 😎"

struct TextSpan {
	enum Type {
		TYPE_PLAIN,
		TYPE_BOLD,
		TYPE_ITALIC,
		TYPE_HEADER,
		TYPE_LINK,
		TYPE_LINEBREAK
	};

	Type type;

	std::string content;
	std::string href;  // only for TYPE_LINK

	TextSpan(Type t, const std::string &c)
		: type(t), content(c) { }
};

void visit(const xdom::DomNode &root, std::list<TextSpan> &spans) {
	// process cdata and children in parallel
	for (int i = 0; i < (int)root.cdata.size(); ++i) {
		std::string cdata = root.cdata[i];
		if (cdata != "") {
			spans.push_back(TextSpan(TextSpan::TYPE_PLAIN, cdata));
		}
		if (i < (int)root.children.size()) {
			const xdom::DomNode &child = root.children[i];
			if (child.name == "b") {
				std::string cdata = join(child.cdata, "");
				spans.push_back(TextSpan(TextSpan::TYPE_BOLD, cdata));
			} else if (child.name == "i") {
				std::string cdata = join(child.cdata, "");
				spans.push_back(TextSpan(TextSpan::TYPE_ITALIC, cdata));
			} else if (child.name == "h1") {
				std::string cdata = join(child.cdata, "");
				spans.push_back(TextSpan(TextSpan::TYPE_HEADER, cdata));
			} else if (child.name == "a") {
				std::string cdata = join(child.cdata, "");
				TextSpan linkSpan(TextSpan::TYPE_LINK, cdata);
				linkSpan.href = child.attributes.at("href");
				spans.push_back(linkSpan);
			} else if (child.name == "br") {
				spans.push_back(TextSpan(TextSpan::TYPE_LINEBREAK, ""));
			} else {
				// NOTE we're not recursing for known tags.
				visit(child, spans);
			}
		}
	}
}

std::string replaceAll(std::string input, const std::string &replace_word, const std::string &replace_by) {
	// Find the first occurrence of the substring
	size_t pos = input.find(replace_word);

	// Iterate through the string and replace all
	// occurrences
	while (pos != string::npos) {
		// Replace the substring with the specified string
		input.replace(pos, replace_word.size(), replace_by);

		// Find the next occurrence of the substring
		pos = input.find(replace_word,
						 pos + replace_by.size());
	}
	return input;
}
std::list<TextSpan> spansFromText(std::string const &text) {
	// replace double newlines with <br> tags
	std::string processed = "<root>" + replaceAll(text, "\n\n", "<br/>") + "</root>";
	processed = replaceAll(processed, "\n", " "); //TODO: remove tabs and multiple spaces too?

	xdom::DomParser parser = xdom::DomParser(processed);
	xdom::DomNode root = parser.parse();

	// now walk the tree emitting TextSpan objects
	std::list<TextSpan> spans;
	visit(root, spans);

	return spans;
}

class DemoImpl: public Demo {
	
	ALLEGRO_FONT *normal, *bold, *italic, *header;
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
		normal = al_load_font("data/DejaVuSans.ttf", 16, 0);
		bold = al_load_font("data/DejaVuSans-Bold.ttf", 16, 0);
		italic = al_load_font("data/DejaVuSans-Oblique.ttf", 16, 0);
		header = al_load_font("data/DejaVuSans-Bold.ttf", 24, 0);

		// ex.font = al_create_builtin_font();
		if (!normal || !bold || !italic || !header) {
			abort_example("Font not found\n");
		}
		printf("Font loaded.\n");
		background = al_color_name("beige");
		text = al_color_name("black");
		white = al_color_name("white");
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

		// we parse html into spans
		list<TextSpan> spans = spansFromText(TEST_TEXT);
		float xcursor = 0;
		float ycursor = 0;
		for(auto &span : spans) {
			ALLEGRO_FONT *font = nullptr;
			ALLEGRO_COLOR color = text;
			int th = al_get_font_line_height(normal);
			switch(span.type) {
				case TextSpan::TYPE_HEADER:
					font = header;
					th = al_get_font_line_height(font) * 1.5;
					break;
				case TextSpan::TYPE_BOLD:
					font = bold;
					break;
				case TextSpan::TYPE_ITALIC:
					font = italic;
					break;
				case TextSpan::TYPE_PLAIN:
					font = normal;
					break;
				case TextSpan::TYPE_LINK:
					font = normal;
					color = al_color_name("blue");
					break;
			}
			draw_multiline_text(font, color, text_x, text_y, &xcursor, &ycursor, 400, th, 0, span.content.c_str());
			if (span.type == TextSpan::TYPE_LINEBREAK || span.type == TextSpan::TYPE_HEADER) {
				xcursor = 0;
				ycursor += th;
			}
		}

	}

	virtual void update() {

	}

};

unique_ptr<Demo> Demo::newInstance() {
	return make_unique<DemoImpl>();
}
