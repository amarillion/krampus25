#include "demo.h"

#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_color.h>
#include <allegro5/utf8.h>
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
	"in the browser using <a href=\"https://liballeg.org\">allegro</a>\n" \
	"and <a href=\"https://emscripten.org\">emscripten.</a>\n" \
	"\n" \
	"	We also support accented characters: á é í ó ú ü ñ ¿ ¡\n" \
	"\n" \
	"	Cool eh? 😎\n\n" \
	"	There are still some\n" \
	"	<i>glitches</i> when it comes to\n" \
	"	<b>Tab</b> characters"

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

	TextSpan(Type t, const std::string &c) : type(t), content(c) {

	}
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

struct CallBackContext {
	TextSpan *span;
	float xoffset, yoffset;
	ALLEGRO_FONT *font;
	ALLEGRO_COLOR color;
	float line_height;
};

bool cb(int line_num, float xflow, float yflow, const ALLEGRO_USTR *line, void *extra) {
	// printf("Callback line %d at (%.2f, %.2f): '%s'\n", line_num, xcursor, ycursor, line);
	CallBackContext *s = (CallBackContext*) extra;

	float x = s->xoffset + xflow; //TODO: take into account alignment flags...
	float y = s->yoffset + yflow;

	// TODO: line_num no longer needed, remove from callback?
	al_draw_ustr(s->font, s->color, x, y, 0, line);

	return true;
}

class DemoImpl: public Demo {
	
	ALLEGRO_FONT *normal, *bold, *italic, *header;
	ALLEGRO_COLOR background, text, white;

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
		int iw = 400;
		ALLEGRO_BITMAP *screen;
		void *data;
		int size, i, format;
		
		// al_set_blender(ALLEGRO_ADD, ALLEGRO_ONE, ALLEGRO_ZERO);

		al_clear_to_color(background);

		screen = al_get_target_bitmap();


		// we parse html into spans
		list<TextSpan> spans = spansFromText(TEST_TEXT);
		float xflow = 0;
		float yflow = 0;
		for(auto &span : spans) {

			CallBackContext ctx;
			ctx.xoffset = 8;
			ctx.yoffset = 8;

			ctx.font = nullptr;
			ctx.color = text;
			ctx.line_height = al_get_font_line_height(normal);
			switch(span.type) {
				case TextSpan::TYPE_HEADER:
					ctx.font = header;
					ctx.line_height = al_get_font_line_height(header) * 1.5;
					break;
				case TextSpan::TYPE_BOLD:
					ctx.font = bold;
					break;
				case TextSpan::TYPE_ITALIC:
					ctx.font = italic;
					break;
				case TextSpan::TYPE_PLAIN:
					ctx.font = normal;
					break;
				case TextSpan::TYPE_LINK:
					ctx.font = normal;
					ctx.color = al_color_name("blue");
					break;
			}

			// draw_multiline_text(font, color, 8, 8, &xcursor, &ycursor, iw, th, 0, span.content.c_str());

			ALLEGRO_USTR_INFO info;

			do_multiline_ustr(ctx.font, &xflow, &yflow, ctx.line_height, iw, al_ref_cstr(&info, span.content.c_str()), cb, &ctx);

			if (span.type == TextSpan::TYPE_LINEBREAK || span.type == TextSpan::TYPE_HEADER) {
				xflow = 0;
				yflow += ctx.line_height;
			}
		}

	}

	virtual void update() {

	}

};

unique_ptr<Demo> Demo::newInstance() {
	return make_unique<DemoImpl>();
}
