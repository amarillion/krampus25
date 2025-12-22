#include "richtext.h"

#include <string>
#include <list>
#include "dom.h"
#include "strutil.h"
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include "text.h"
#include "multiline.h"

using namespace std;

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

	TextSpan(Type t, const std::string &c) : type(t), content(c) {}
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
	while (pos != std::string::npos) {
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
	list<ComponentPtr> *components;
};

bool cb(int line_num, float xflow, float yflow, const ALLEGRO_USTR *line, void *extra) {
	// printf("Callback line %d at (%.2f, %.2f): '%s'\n", line_num, xcursor, ycursor, line);
	CallBackContext *s = (CallBackContext*) extra;

	float x = s->xoffset + xflow; //TODO: take into account alignment flags...
	float y = s->yoffset + yflow;

	// TODO: line_num no longer needed, remove from callback?
	al_draw_ustr(s->font, s->color, x, y, 0, line);

	// constructor (char *, size) creates a copy of the substring.
	std::string copy(al_cstr(line), al_ustr_size(line));

	auto t = Text::build(s->color, 0, copy).font(s->font).xy(x, y).get();
	t->setVisible(false); // Specifically for animated text...
	s->components->push_back(t);

	// if (s->span->type == TextSpan::TYPE_LINK) {
	// 	float length = al_get_ustr_width(s->font, line);
	// 	al_draw_line(x, y + s->line_height - 2, x + length, y + s->line_height - 2, s->color, 1.0);
	// 	int line_width = al_get_ustr_width(s->font, line);
	// 	int line_height = s->line_height;
	// 	Rect linkRect(x, y, line_width, line_height);
	// 	// al_draw_rectangle(x, y, x + line_width, y + line_height, al_color_name("red"), 1.0);
	// 	s->span->linkHotspots.push_back(linkRect);
	// 	// al_draw_rectangle(x, y, x + line_width, y + line_height, al_color_name("red"), 1.0);
	// }
	return true;
}

void appendRichText(const char *s, float *xflow, float *yflow, int iw, list<ComponentPtr> &components, const StyleData &style) {
	list<TextSpan> spans;
	spans = spansFromText(s);
	// we parse html into spans
	for(auto &span : spans) {

		CallBackContext ctx;
		ctx.components = &components;
		ctx.xoffset = 0;
		ctx.yoffset = 0;

		ctx.font = nullptr;
		ctx.color = style.textColor;
		ctx.line_height = al_get_font_line_height(style.normal);
		switch(span.type) {
			case TextSpan::TYPE_HEADER:
				ctx.font = style.header;
				ctx.line_height = al_get_font_line_height(style.header) * 1.5;
				break;
			case TextSpan::TYPE_BOLD:
				ctx.font = style.bold;
				break;
			case TextSpan::TYPE_ITALIC:
				ctx.font = style.italic;
				break;
			case TextSpan::TYPE_PLAIN:
				ctx.font = style.normal;
				break;
			case TextSpan::TYPE_LINK:
				ctx.font = style.normal;
				ctx.color = style.linkColor;
				break;
		}

		// draw_multiline_text(font, color, 8, 8, &xcursor, &ycursor, iw, th, 0, span.content.c_str());

		ALLEGRO_USTR_INFO info;
		ctx.span = &span;
		// span.linkHotspots = std::vector<Rect>();
		do_multiline_ustr(ctx.font, xflow, yflow, ctx.line_height, iw, al_ref_cstr(&info, span.content.c_str()), cb, &ctx);

		if (span.type == TextSpan::TYPE_LINEBREAK || span.type == TextSpan::TYPE_HEADER) {
			*xflow = 0;
			*yflow += ctx.line_height;
		}
	}
}
