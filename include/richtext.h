#pragma once

#include <allegro5/allegro_color.h>
#include <list>
#include <memory>

class Component;
typedef std::shared_ptr<Component> ComponentPtr;

struct ALLEGRO_FONT;

struct StyleData {
	ALLEGRO_COLOR textColor;
	ALLEGRO_COLOR linkColor;
	ALLEGRO_FONT *normal, *header, *bold, *italic;
};

void appendRichText(const char *s, float *xflow, float *yflow, int iw, std::list<ComponentPtr> &components, const StyleData &style);
