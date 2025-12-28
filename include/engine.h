#pragma once

#include "game.h"
#include "container.h"
#include "text2.h"
#include "simpleloop.h"

class Resources;

class Engine : public Simple::IApp {

private:
	static std::shared_ptr<Resources> resources;
	static ALLEGRO_FONT *font;
	static bool debugMode;
public:
	enum { E_NONE = 0, E_START, E_QUIT };
	std::shared_ptr<Game> game;

	virtual void draw(const GraphicsContext &gc) override {

		/*
		int ScreenW = 800; // TODO: not hardcoded
		int ScreenH = 600; // TODO: not hardcoded
		al_set_clipping_rectangle(0, 0, ScreenW, ScreenH);
		 */
		game->draw(gc);
	}

	virtual ~Engine();
	virtual void handleEvent(ALLEGRO_EVENT &event) override;

	static bool isDebug()
	{
		return debugMode;
	}

	static std::shared_ptr<Resources> getResources() { return resources; }

	static ALLEGRO_FONT *getFont() { return font; }
	//TODO: Engine has series of global accessors including getFont(), getResources(), and isDebug() flag.

	void init();
	virtual bool update() override;
};
