#ifndef GAME_H
#define GAME_H

#include "text2.h"
#include <memory>
#include "state.h"

class Game : public State
{
public:
	virtual ~Game() {};
	virtual void update() override = 0;
	virtual void draw(const GraphicsContext &gc) override = 0;
	virtual void init() = 0;
	virtual void handleEvent(ALLEGRO_EVENT &event) override = 0;

	virtual void initGame() = 0;
	virtual void reloadGameIfExists() = 0;
	virtual void saveGame() = 0;
	static std::shared_ptr<Game> newInstance();

	virtual std::string const className() const override { return "Game"; }
};


#endif
