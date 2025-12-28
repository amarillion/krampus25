#include "engine.h"
#include "util.h"
#include "menubase.h"
#include "DrawStrategy.h"
#include "parser.h"
#include "resources.h"

std::shared_ptr<Resources> Engine::resources = nullptr;
ALLEGRO_FONT *Engine::font = NULL;
bool Engine::debugMode = false;

using namespace std;

void Engine::init() {
	resources = Resources::newInstance();

	resources->addDir("data");

	font = resources->getFont("DejaVuSans")->get(16);
	if (!font) {
		allegro_message("Error loading \"data/fixed_font.tga\".\n");
		exit(1);
	}

	game = Game::newInstance();
	game->init(resources);

	ALLEGRO_PATH *localAppData = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	string cacheDir = al_path_cstr(localAppData, ALLEGRO_NATIVE_PATH_SEP);

	game->initGame();
}

Engine::~Engine() {
	// clear resources /before/ allegro is uninstalled to prevent sigsegv.
	resources = nullptr;
}

void Engine::handleEvent(ALLEGRO_EVENT &event) {
	//TODO: implement F11 as a child of mainloop that send action events on press
#ifdef DEBUG
	if (event.type == ALLEGRO_EVENT_KEY_CHAR &&
		event.keyboard.keycode == ALLEGRO_KEY_F11)
	{
		debugMode = !debugMode;
		return; // consume event
	}
#endif
	game->handleEvent(event);
}

bool Engine::update() {
	bool result = true;
	game->update();
	while(game->hasMsg()) {
		int msg = game->popMsg();
		printf("On Handle Message called %i", msg);

		if (msg == Engine::E_QUIT) {
			result = false;
		}
	}
	return result;
}
