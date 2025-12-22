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

	initMenu();

	ALLEGRO_PATH *localAppData = al_get_standard_path(ALLEGRO_USER_SETTINGS_PATH);
	string cacheDir = al_path_cstr(localAppData, ALLEGRO_NATIVE_PATH_SEP);
	updates = UpdateChecker::newInstance(cacheDir, "0.1", E_QUIT);
 	add (updates, Container::FLAG_SLEEP);
 	updates->start_check_thread("BUN", "en");

	setFocus(mMain);
 	//setFocus(game);
}

void Engine::initMenu()
{
	mPause = MenuBuilder(this, NULL)
		.push_back (make_shared<ActionMenuItem> (E_RESUME, "Resume", "Resume current game"))
		.push_back (make_shared<ActionMenuItem> (E_RESTART, "Restart from beginning", "Stop progress and restart from the beginning"))
		.push_back (make_shared<ActionMenuItem> (E_EXITSCREEN, "Save & Quit", "Save game and exit"))
		.push_back (make_shared<ActionMenuItem> (E_EXIT_NOSAVE, "Quit", "Quit without saving"))
		.push_back (make_shared<ActionMenuItem> (E_TOGGLE_FULLSCREEN, "Toggle Fullscreen", "Switch fullscreen / windowed mode"))
		.build();
	mPause->setFont(font);
	mPause->add(ClearScreen::build(BLUE).get(), Container::FLAG_BOTTOM);
	add (mPause);

	auto menuLoad = make_shared<ActionMenuItem> (E_LOAD, "Continue game", "Continue the last saved game where you left off");
	mMain = MenuBuilder(this, NULL)
		.push_back (menuLoad)
		.push_back (make_shared<ActionMenuItem> (E_RESTART, "Start from beginning", "Start a new game from the beginning"))
		.push_back (make_shared<ActionMenuItem> (E_EXITSCREEN, "Save & Quit", "Sage game and exit"))
		.push_back (make_shared<ActionMenuItem> (E_EXIT_NOSAVE, "Quit", "Quit without saving"))
		.push_back (make_shared<ActionMenuItem> (E_TOGGLE_FULLSCREEN, "Toggle Fullscreen", "Switch fullscreen / windowed mode"))
		.build();
	mMain->setFont(font);
	mMain->add(ClearScreen::build(BLUE).get(), Container::FLAG_BOTTOM);
	add (mMain);

	if (!Interpreter::savedGameExists()) menuLoad->setEnabled(false);
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
	switch (event)
	{
	case E_ENDGAME:
		setFocus (mMain);
		break;
	case E_PAUSE:
		setFocus (mPause);
		break;
	case E_RESUME:
		setFocus (game);
		break;
	case E_TOGGLE_FULLSCREEN:
		MainLoop::getMainLoop()->toggleWindowed();
		break;
	case E_RESTART:
		game->initGame();
		setFocus(game);
		break;
	case E_LOAD:
		game->reloadGameIfExists();
		setFocus(game);
		break;
	case E_EXITSCREEN:
		game->saveGame();
		setFocus(updates);
		break;
	case E_EXIT_NOSAVE:
		setFocus(updates);
		break;
	case E_QUIT:
		pushMsg(MSG_CLOSE);
		break;
	}

	return true;
}
