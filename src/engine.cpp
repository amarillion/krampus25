#include "engine.h"
#include "util.h"
#include "mainloop.h"
#include "menubase.h"
#include "DrawStrategy.h"
#include "parser.h"
#include "resources.h"

std::shared_ptr<Resources> Engine::resources = nullptr;
ALLEGRO_FONT *Engine::font = NULL;
bool Engine::debugMode = false;

using namespace std;

void Engine::init()
{
	resources = Resources::newInstance();

	resources->addDir("data");

	font = resources->getFont("DejaVuSans")->get(16);
	if (!font) {
		allegro_message("Error loading \"data/fixed_font.tga\".\n");
		exit(1);
	}

	setFont(font);

	game = Game::newInstance();
	game->init(resources);
	add(game, Container::FLAG_SLEEP);

	ALLEGRO_PATH *localAppData = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	string cacheDir = al_path_cstr(localAppData, ALLEGRO_NATIVE_PATH_SEP);

	game->initGame();
 	setFocus(game);
}

Engine::~Engine() {}

void Engine::handleEvent(ALLEGRO_EVENT &event)
{
	//TODO: implement F11 as a child of mainloop that send action events on press
#ifdef DEBUG
	if (event.type == ALLEGRO_EVENT_KEY_CHAR &&
		event.keyboard.keycode == ALLEGRO_KEY_F11)
	{
		debugMode = !debugMode;
		return; // consume event
	}
#endif
	Container::handleEvent(event);
}


bool Engine::onHandleMessage(ComponentPtr src, int event)
{
	return true;
}
