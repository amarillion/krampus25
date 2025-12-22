#ifndef ENGINE_H
#define ENGINE_H

#include "game.h"
#include "container.h"
#include "menubase.h"
#include "text2.h"
#include "updatechecker.h"

class Resources;

class Engine : public Container {

private:
	static std::shared_ptr<Resources> resources;
	static ALLEGRO_FONT *font;
	static bool debugMode;
	void initMenu();
	MenuScreenPtr mPause;
	MenuScreenPtr mMain;
	std::shared_ptr<UpdateChecker> updates;
public:
	enum { E_NONE = 0, E_RESUME, E_QUIT, E_EXITSCREEN, E_LOAD, E_RESTART, E_TOGGLE_FULLSCREEN, E_EXIT_NOSAVE, E_PAUSE, E_ENDGAME };
	std::shared_ptr<Game> game;

	virtual void draw(const GraphicsContext &gc) {

		/*
		int ScreenW = 800; // TODO: not hardcoded
		int ScreenH = 600; // TODO: not hardcoded
		al_set_clipping_rectangle(0, 0, ScreenW, ScreenH);
		 */
		focus->draw(gc);
	}

	virtual ~Engine();
	virtual void handleEvent(ALLEGRO_EVENT &event) override;
	virtual bool onHandleMessage(ComponentPtr src, int msg) override;

	static bool isDebug()
	{
		return debugMode;
	}

	static std::shared_ptr<Resources> getResources() { return resources; }

	static ALLEGRO_FONT *getFont() { return font; }
	//TODO: Engine has series of global accessors including getFont(), getResources(), and isDebug() flag.

	void init();
};

#endif
