#ifndef COMPONENT2_H
#define COMPONENT2_H

#include <vector>
#include <list>
#include <string>
#include <allegro5/allegro.h>
#include "color.h"
#include "component.h"
#include "widget.h"
#include "richtext.h"

struct ALLEGRO_FONT;
struct ALLEGRO_SAMPLE;

class ImageSegment : public Component {
	ALLEGRO_BITMAP *bmp;
public:
	ImageSegment(ALLEGRO_BITMAP *bmp) : bmp(bmp)
	{
		setVisible(false);
		setFont(NULL);
	}

	virtual void draw(const GraphicsContext &gc) override;
};

class TextCanvas : public Component {
private:
	int busy;
	std::list<ComponentPtr> lines;
	std::list<ComponentPtr>::iterator cursor;
	ALLEGRO_COLOR activeColor; // color used for appending.
	ALLEGRO_FONT *activeFont; // font used for appending
	StyleData style;
	int xco;
	int yco;
	int yoffset;
	int scrollSpeed;
	bool waitForAnimationDone; // signal that we are waiting for a segment to finish appearing
	void carriageReturn();
	void startAnimationAtCursor();
public:
	bool isBusy() { return busy != 0; }
	bool speedUp;
	TextCanvas() : busy(0), lines(), cursor(lines.begin()), activeColor(WHITE), activeFont(NULL), xco(0), yco(0), yoffset(0), scrollSpeed(2), waitForAnimationDone(false), speedUp(false) {}
	void appendLine (const std::string &line);
	void append (const std::string &line, ALLEGRO_COLOR color);
	void appendImage (ALLEGRO_BITMAP *img);
	void appendRich(const std::string &line);
	void setActiveColor(ALLEGRO_COLOR color);
	void setActiveFont(ALLEGRO_FONT *font) { assert (font != NULL); activeFont = font; }
	void setStyle(const StyleData &style);
	void clear();
	ComponentPtr getComponentAt(int x, int y);
	virtual void handleMessage(ComponentPtr src, int msg) override;
	virtual void draw(const GraphicsContext &gc) override;
	virtual void update() override;
	virtual ~TextCanvas() {}
};

#endif
