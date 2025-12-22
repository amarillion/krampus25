#include "color.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>
#include <cstdio>
#include <iostream>
#include "text.h"
#include "text2.h"

using namespace std;

class ComponentRemover
{
public:
	bool operator()(ComponentPtr &p)
	{
		if (!p->isAlive())
		{
			return true;
		}
		return false;
	}
};

void TextCanvas::clear()
{
	lines.clear();
	cursor = lines.begin();
	activeColor = WHITE;
	yoffset = 0;
	xco = 0;
	yco = 0;
}

void TextCanvas::startAnimationAtCursor()
{
	(*cursor)->setVisible(true);

	shared_ptr<Text> textSegment = dynamic_pointer_cast<Text>(*cursor);
	if (textSegment)
	{
		if (!speedUp)
		{
			textSegment->startTypewriter(Text::LETTER_BY_LETTER);
			waitForAnimationDone = true;
		}
	}
}

void TextCanvas::handleMessage(ComponentPtr src, int msg)
{
	// if we receive a message that the current segment is done animating
	if (msg == MSG_ANIMATION_DONE)
	{
		// advance to next segment
		waitForAnimationDone = false;
	}
}

void TextCanvas::update()
{
	busy = 0; // 1 = busy, 0 = ready
	//TODO: better system would be to send event when busy state changes.

	if (cursor == lines.end())
	{
		cursor = lines.begin();
		while (cursor != lines.end() && (*cursor)->isVisible()) { cursor++; }
	}

	if (cursor != lines.end())
	{
		busy = 1;

		if (waitForAnimationDone)
		{
			(*cursor)->update();
			checkMessages(*cursor);
		}
		else
		{
			if (!(*cursor)->isVisible())
			{
				startAnimationAtCursor();
			}
			else
			{
				cursor++;
				if (cursor != lines.end()) {
					startAnimationAtCursor();
				}
				else {
					busy = 0;
				}
			}
		}
	}

	if (yco - yoffset > y + h)
	{
		yoffset += scrollSpeed; // scroll by one pixel
		busy = 1;
	}

	// clean up lines above the fold...
	lines.remove_if(ComponentRemover());
}

void TextCanvas::append(const string &line, ALLEGRO_COLOR color)
{
	assert (activeFont);

	int segstart = 0;
	int breakPos = -1;
	int nonBreakPos = -1;

	// scan forward until we either hit a newline char, or if
	// keep remembering latest valid breakpoint
	for (unsigned int pos = 0; pos < line.length(); ++pos)
	{
		if (line[pos] == '\n' || line[pos] == '\r')
		{
			if (pos - segstart > 0)
			{
				auto segment = Text::build(color, line.substr(segstart, (pos-segstart))).xy(xco, yco).font(activeFont).get();
				segment->setVisible(false);
				lines.push_back(segment);
			}
			xco = 0;
			yco += al_get_font_line_height(activeFont);

			segstart = pos + 1;
		}
		else
		{
			bool fits = (xco + al_get_text_width(activeFont, line.substr(segstart, (pos-segstart)).c_str()) <= w);

			if (fits)
			{
				nonBreakPos = pos;
			}

			if (line[pos] == ' ' || line[pos] == '\t')
			{
				if (fits)
				{
					breakPos = pos;
				}
			}

			if (!fits)
			{
				if (breakPos > 0)
				{
					auto segment = Text::build(color, ALLEGRO_ALIGN_LEFT, line.substr(segstart, (breakPos-segstart))).xy(xco, yco).font(activeFont).get();
					segment->setVisible(false);
					carriageReturn();

					lines.push_back(segment);

					segstart = breakPos + 1;
					nonBreakPos = -1;
					breakPos = -1;
				}
				else if (nonBreakPos > 0)
				{
					auto segment = Text::build(color, ALLEGRO_ALIGN_LEFT, line.substr(segstart, (nonBreakPos-segstart))).xy(xco, yco).font(activeFont).get();
					segment->setVisible(false);
					carriageReturn();

					lines.push_back(segment);

					segstart = nonBreakPos + 1;
					nonBreakPos = -1;
					breakPos = -1;
				}
				else
				{
					if (xco != 0)
					{
						// nothing will fit in remainder of this row. move to the next row
						xco = 0;
						yco += al_get_font_line_height(activeFont);
					}
					else
					{
						// nothing fits on the entire width. Give up.
						break;
					}
				}


			}
		}
	}

	// create segment for remainder
	auto segment = Text::build(color, ALLEGRO_ALIGN_LEFT, line.substr(segstart, line.length())).xy(xco, yco).font(activeFont).get();
	segment->setVisible(false);
	lines.push_back(segment);

	xco += al_get_text_width(activeFont, line.substr(segstart, line.length()).c_str());
}

void TextCanvas::appendLine(const string &line)
{
	append (line, activeColor);
}

void TextCanvas::appendRich(const string &line) {
	float xflow = xco; 
	float yflow = yco;
	appendRichText(line.c_str(), &xflow, &yflow, w, lines, style);
	xco = xflow;
	yco = yflow;
}

void TextCanvas::carriageReturn()
{
	xco = 0;
	yco += al_get_font_line_height(activeFont);
}

void TextCanvas::appendImage (ALLEGRO_BITMAP *img)
{
	auto segment = make_shared<ImageSegment>(img);
	carriageReturn();
	segment->setx(xco);
	segment->sety(yco);
	yco += al_get_bitmap_height(img);
	lines.push_back(segment);
}

void TextCanvas::draw(const GraphicsContext &gc)
{
	GraphicsContext gc2 = GraphicsContext();
	gc2.xofst = x;
	gc2.yofst = y - yoffset;

	for (auto seg : lines)
	{
		seg->draw(gc2);

		if ((seg->gety() - yoffset) < -400)
		{
			seg->kill(); // reclaim..
		}
	}
}

void TextCanvas::setStyle(const StyleData &_style) {
	style = _style;
}

void ImageSegment::draw(const GraphicsContext &gc)
{
	if (isVisible())
	{
		if (bmp)
		{
			al_draw_bitmap(bmp, x + gc.xofst, y + gc.yofst, 0);
		}
	}
}
