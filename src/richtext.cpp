#include "richtext.h"

#include <string>
#include <list>
#include "xml.h"
#include "strutil.h"
#include <allegro5/allegro_color.h>
#include <allegro5/allegro_font.h>
#include "text.h"
#include "multiline.h"
#include "openLink.h"

using namespace std;

/**
 * Linearized rich text.
 */
struct TextSpan {
	string tag; // NOTE: if we want to store bold AND italic, we'd have to expand this to a set of tags.
	string content;
	map<string, string> attributes; // used for e.g. href.

	TextSpan(): tag(), content(), attributes() {}
	TextSpan(const string &_tag, const string &c) : tag(_tag), content(c), attributes() {}
	TextSpan(const string &_tag, const string &c, const map<string, string> &attr) : tag(_tag), content(c), attributes(attr) {}
};

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

	if (s->span->tag == "a") {
		t->setDecoration(TextStyle::UNDERLINE);
		string hrefcpy = s->span->attributes["href"];
		t->onClick([=](){ openLink(hrefcpy); });
	}
	// else {
	// 	string lcpy = string(al_cstr(line));
	// 	t->onClick([=](){ cout << "Clicked on span with text " << lcpy << endl; });
	// }

	t->setVisible(false); // Specifically for animated text...
	s->components->push_back(t);

	return true;
}

/** 
 * Linearizes html-like rich text string into a sequence of spans, each span having a single style. 
 * throws XmlException if the string is invalid XML.
 **/
list<TextSpan> spansFromHtml(const string &str) {
	list<TextSpan> textSpans;
	TextSpan currentSpan { "", "" };
	bool hasContent = false;
	XmlParser parser(str);
	while (parser.next() != XmlParser::END_DOCUMENT) {
		switch (parser.getEventType()) {
			case XmlParser::START_TAG:
				if (hasContent) {
					textSpans.push_back(currentSpan);
				}
				currentSpan = TextSpan(parser.getName(), "", parser.getAttributes());
				hasContent = true;
				break;
			case XmlParser::END_TAG:
				if (hasContent) {
					textSpans.push_back(currentSpan);
				}
				currentSpan = TextSpan("", "");
				hasContent = false;
				break;
			case XmlParser::TEXT: {
				hasContent = true;
				std::string text = parser.getText();
				if (!text.empty()) {
					// TODO: also flatten consecutive whitespace
					currentSpan.content += text;
				}
				break;
			}
			default: break;
		}
	}
	if (hasContent) {
		textSpans.push_back(currentSpan);
	}
	return textSpans;
}

void appendRichText(
	const char *s, float *xflow, float *yflow, int iw, list<ComponentPtr> &components, const StyleData &style
) {
	list<TextSpan> spans;
	spans = spansFromHtml(s);
	// we parse html into spans
	for(auto &span : spans) {

		CallBackContext ctx;
		ctx.components = &components;
	
		ctx.xoffset = 0;
		ctx.yoffset = 0;

		ctx.font = nullptr;
		ctx.color = style.textColor;
		ctx.line_height = al_get_font_line_height(style.normal);
		if(span.tag == "h1") {
			ctx.font = style.header;
			ctx.line_height = al_get_font_line_height(style.header) * 1.5;
		}
		else if (span.tag == "b") {
			ctx.font = style.bold;
		}
		else if (span.tag == "i") {
			ctx.font = style.italic;
		}
		else if (span.tag == "") {
			ctx.font = style.normal;
		}
		else if (span.tag == "a") {
			ctx.font = style.normal;
			ctx.color = style.linkColor;
		}
		else if (span.tag == "br") {
			// no styling associated with line breaks.
		}

		// draw_multiline_text(font, color, 8, 8, &xcursor, &ycursor, iw, th, 0, span.content.c_str());

		ALLEGRO_USTR_INFO info;
		ctx.span = &span;
		// span.linkHotspots = std::vector<Rect>();
		do_multiline_ustr(ctx.font, xflow, yflow, ctx.line_height, iw, al_ref_cstr(&info, span.content.c_str()), cb, &ctx);

		if (span.tag == "br" || span.tag == "h1") {
			*xflow = 0;
			*yflow += ctx.line_height;
		}
	}
}
