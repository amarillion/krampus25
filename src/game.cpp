#include <vector>
#include <map>
#include <string>
#include <sstream>
#include <fstream>
#include <iostream>
#include <assert.h>

#include "game.h"
#include "color.h"
#include <allegro5/allegro.h>
#include <allegro5/allegro_font.h>
#include <allegro5/allegro_audio.h>

#include "engine.h"
#include "strutil.h"
#include "particle.h"
#include "strutil.h"
#include <locale>
#include <stack>

#include "text2.h"
#include "parser.h"
#include "textstyle.h"
#include "mainloop.h"
#include "resources.h"

using namespace std;

class Inventory : public Component {
	map<string, ALLEGRO_BITMAP*> bmps;
	SimpleState &sstate;
public:
	Inventory(SimpleState &sstate) : sstate(sstate) {};
	void init()
	{
		bmps["cap"] = Engine::getResources()->getBitmap("cap");

		bmps["cher"] = Engine::getResources()->getBitmap("cher");
		bmps["ramones"] = Engine::getResources()->getBitmap("ramones");

		bmps["basketorganic"] = Engine::getResources()->getBitmap("organic");
		bmps["basketnormal"] = Engine::getResources()->getBitmap("basket");

		bmps["scarf"] = Engine::getResources()->getBitmap("scarf");
		bmps["sunglasses"] = Engine::getResources()->getBitmap("sunglasses");
		bmps["umbrella"] = Engine::getResources()->getBitmap("umbrella");

		bmps["device"] = Engine::getResources()->getBitmap("device");

		bmps["walletempty"] = Engine::getResources()->getBitmap("walletempty");
		bmps["monez"] = Engine::getResources()->getBitmap("walletfull");
	}

	virtual void draw(const GraphicsContext &gc) override
	{
		list<ALLEGRO_BITMAP*> toDraw;

		if (sstate.gameVariables["monez"] == 0) toDraw.push_back(bmps["walletempty"]);

		for (auto str : vector<string>{ "monez", "cap", "scarf", "sunglasses", "umbrella", "cher", "ramones"})
		{
			if (sstate.gameVariables[str] > 0)
			{
				toDraw.push_back(bmps[str]);
			}
		}

		if (sstate.gameVariables["basket"] == 1 && sstate.gameVariables["organic"] == 0 ) toDraw.push_back(bmps["basketnormal"]);
		if (sstate.gameVariables["basket"] == 1 && sstate.gameVariables["organic"] == 1 ) toDraw.push_back(bmps["basketorganic"]);
		if (sstate.gameVariables["device"] == 1) toDraw.push_back(bmps["device"]);

		int xco = getx();
		int yco = gety();
		draw_text_with_background(Engine::getFont(), WHITE, GREY, xco, yco, ALLEGRO_ALIGN_LEFT, "INVENTORY");

		yco += 32;

		for (ALLEGRO_BITMAP *bmp : toDraw)
		{
			al_draw_bitmap(bmp, xco, yco, 0);
			yco += 48;
		}
	}

};

class Squeak
{
public:
	vector<ALLEGRO_SAMPLE_ID> started; // keep track of looped samples

	void clear()
	{
		// stop any looped samples, leaving play-once samples untouched.
		for (vector<ALLEGRO_SAMPLE_ID>::iterator i = started.begin(); i != started.end(); ++i)
		{
			al_stop_sample(&(*i));
		}
		started.clear();
	}

	void playSample(ALLEGRO_SAMPLE *sample_data)
	{
		bool success = al_play_sample (sample_data, 1.0, 1.0, 1.0, ALLEGRO_PLAYMODE_ONCE, NULL);
		if (!success) {
			cout << "Could not play sample";
		}
	}

	void init()
	{
		bool success = al_reserve_samples(16);
		if (!success)
		{
			cout << "Could not reserve samples";
			return;
		}
	}

	void startLoop(const string &id)
	{
		clear();

		ALLEGRO_SAMPLE_ID sampleid;
		ALLEGRO_SAMPLE *sample_data = Engine::getResources()->getSampleIfExists(id);

		if (!sample_data)
		{
			cout << "Could not find sample";
			return;
		}

		bool success = al_play_sample (sample_data, 1.0, 0.5, 1.0, ALLEGRO_PLAYMODE_LOOP, &sampleid);
		if (!success) {
			 cout << "Could not start sample";
		}

		started.push_back(sampleid);

	}

	void startRain()
	{
		startLoop("ThunderStorm");
	}

	void startSqueak()
	{
		startLoop("BabyCrying");
	}

	void startWind()
	{
		startLoop("Wind");
	}

};

class AnswerComponent : public Component {
public:
	Answer answer;
	bool selected;
	virtual void draw();
};

enum GameState { ANSWERING, PAUSE };

class GameImpl : public Game, StatementHandler {
private:
	TextCanvas text; // currently displayed text component;
	Particles particles;
	Squeak squeak;
	string activeEffect;
	GameState state;
	Story story;
	vector<AnswerComponent>::iterator selectedAnswer;
	vector<AnswerComponent> currentAnswers;
	SimpleState sstate;
	Inventory inventory;
	unique_ptr<Interpreter> interpreter;
	Node *getCurrentNode() { return &(story.nodes[sstate.currentNodeName]); }
	void parse(string fname);

	Answer executeAnswer(vector<Command>::iterator &i, vector<Command>::iterator end);

	void executeStatements(vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end);
	void executeCommands(vector<Command> commands);

	virtual void gameAssert(bool test, const string &data) override;
	virtual void executeSideEffect(Command *i) override;

	void refreshGame();

	void loadGame()
	{
		SimpleState newstate;
		bool ok = newstate.load();
		text.append ("Game loaded", MAGENTA);
		if (!ok) {
			text.append ("Something went wrong while loading!", RED);
			//TODO... recover
		}
		else
		{
			sstate = newstate;
			executeCommands (getCurrentNode()->commands);
		}
	}

	virtual void saveGame() override
	{
		sstate.save();
		text.append ("Game saved", MAGENTA);
	}

	/**
	 * Bring the game in a clean starting state, but don't
	 * execute any node yet.
	 */
	void clearState()
	{
		text.clear();
		particles.setEffect(CLEAR);
		squeak.clear();

		parse("data/BUN.txt");
	//	parse("example.txt");
		setCurrentNode("START");

		sstate.gameVariables.clear();
		for (vector<string>::iterator i = story.flags.begin(); i != story.flags.end(); ++i)
		{
			sstate.gameVariables[*i] = 0;
		}
	}

	//TODO: duplicate code.
	void setCurrentNode(const string &id)
	{
		testNodeExists (id);
		if (Engine::isDebug())
		{
			std::stringstream ss;
			ss << "DEBUG: Going to node: '" << id << "'";
			text.append(ss.str(), GREY);
		}

		sstate.currentNodeName = id;
	}

	//TODO: duplicate code.
	bool testNodeExists (const std::string &id)
	{
		bool result = story.nodes.find (id) != story.nodes.end();
		if (!result)
		{
			std::stringstream ss;
			ss << "Node: '" << id << "' not found!";
			gameAssert(result, ss.str());
		}
		return result;
	}


public:
	virtual void initGame() override
	{
		clearState();
		executeCommands (getCurrentNode()->commands);
		state = PAUSE;
	}

	virtual void reloadGameIfExists() override
	{
		clearState();

		SimpleState newstate;
		bool ok = newstate.load();

		if (ok)
		{
			sstate = newstate;
		}

		executeCommands (getCurrentNode()->commands);
	}

	virtual void update() override;
	virtual void draw(const GraphicsContext &gc) override;
	virtual void handleEvent(ALLEGRO_EVENT &event) override;
	virtual void init() override;
	virtual ~GameImpl() {}
	GameImpl();

	virtual void debugMsg(const string & msg, ALLEGRO_COLOR color) override
	{
		if (Engine::isDebug())
		{
			text.append(msg, color);
		}
	}

};

shared_ptr<Game> Game::newInstance()
{
	return make_shared<GameImpl>();
}

void GameImpl::gameAssert(bool test, const string &value)
{
	if (!test) cout << value << endl;
	if (!test) text.append("ERROR: " + value + "\n", RED);
}

GameImpl::GameImpl() : activeEffect("clear"), state(PAUSE), sstate(), inventory(sstate)
{
	// layout
	text.setLocation(80, 80, 540, 200);
	inventory.setLocation(680, 80, 100, 480);
	particles.setLocation(0, 0, MAIN_WIDTH, MAIN_HEIGHT);
}

void GameImpl::update()
{
	particles.update();

	text.speedUp = Engine::isDebug();
	text.update();
	bool textUpdating = text.isBusy();

	switch (state)
	{
		case PAUSE: {
			if (!textUpdating) state = ANSWERING;
			break;
		}
		case ANSWERING: {
			if (textUpdating) state = PAUSE;
			break;
		}
	}
}

void GameImpl::handleEvent(ALLEGRO_EVENT &event)
{
	// the following events are handled in all modes.
	if (event.type == ALLEGRO_EVENT_KEY_CHAR)
	{
		switch (event.keyboard.keycode)
		{
		case ALLEGRO_KEY_F5:
			if (Engine::isDebug())
			{
				refreshGame();
			}
			break;
		case ALLEGRO_KEY_F6:
			if (Engine::isDebug())
			{
				loadGame();
			}
			break;
		case ALLEGRO_KEY_F7:
			if (Engine::isDebug())
			{
				saveGame();
			}
			break;
		case ALLEGRO_KEY_ESCAPE:
			pushMsg (Engine::E_PAUSE);
			break;
		}
	}

	if (state != ANSWERING) return; // ignore key events
	// the following events are only handled in ANSWERING mode
	// TODO: break out into sub-component.

	if (event.type == ALLEGRO_EVENT_KEY_CHAR)
	{
		switch (event.keyboard.keycode)
		{
		case ALLEGRO_KEY_UP:
			selectedAnswer->selected = false;
			if (selectedAnswer == currentAnswers.begin())
			{
				selectedAnswer = currentAnswers.begin() + currentAnswers.size() - 1;
			}
			else
			{
				selectedAnswer--;
			}
			selectedAnswer->selected = true;
			break;
		case ALLEGRO_KEY_DOWN:
			selectedAnswer->selected = false;
			selectedAnswer++;
			if (selectedAnswer == currentAnswers.end())
			{
				selectedAnswer = currentAnswers.begin();
			}
			selectedAnswer->selected = true;
			break;
		case ALLEGRO_KEY_ENTER:
			// execute associated commands
			executeCommands (selectedAnswer->answer.commands);
			break;
		}
	}
}

void GameImpl::executeSideEffect(Command *i)
{
	switch (i->commandType)
	{
	case END:
		pushMsg (Engine::E_ENDGAME);
//		i = end;
		return;
	case TEXT:
		if (i->parameter == "")
		{
			text.appendLine("\n"); // paragraph break
		}
		else
		{
			text.appendLine(i->parameter);
		}
		break;
	case IMAGE: {
		ALLEGRO_BITMAP *img = Engine::getResources()->getBitmapIfExists(i->parameter);
		if (img)
		{
			text.appendImage(img);
		}
		else
		{
			stringstream ss;
			ss << "Could not find image: " << i->parameter;
			gameAssert (false, ss.str());
		}
		break;
	}
	case SAMPLE: {
		ALLEGRO_SAMPLE *sam = Engine::getResources()->getSampleIfExists(i->parameter);
		if (sam)
		{
			squeak.playSample(sam);
		}
		else
		{
			stringstream ss;
			ss << "Could not find sample: " << i->parameter;
			gameAssert (false, ss.str());
		}
		break;
		}
	case EFFECT:
		//TODO: ignore repeated invocations of same effect...
		if (activeEffect == i->parameter) { break; }
		activeEffect = i->parameter;
		if (i->parameter == "RAIN")
		{
			particles.setEffect(RAIN);
			squeak.startRain();
		}
		else if (i->parameter == "SNOW")
		{
			particles.setEffect(SNOW);
			squeak.clear();
		}
		else if (i->parameter == "STARS")
		{
			particles.setEffect(STARS);
			squeak.clear();
		}
		else if (i->parameter == "METEOR")
		{
			particles.setEffect(METEOR);
			squeak.clear();
		}
		else if (i->parameter == "ANTIGRAV")
		{
			particles.setEffect(ANTIGRAV);
			squeak.clear();
		}
		else if (i->parameter == "CONFETTI")
		{
			particles.setEffect(CONFETTI);
			squeak.clear();
		}
		else if (i->parameter == "CLEAR")
		{
			particles.setEffect(CLEAR);
			squeak.clear();
		}
		else if (i->parameter == "SQUEAK")
		{
			squeak.startSqueak();
		}
		else if (i->parameter == "WIND")
		{
			squeak.startWind();
			particles.setEffect(WIND);
		}
		else if (i->parameter == "POW")
		{
			particles.setEffect(POW);
			squeak.clear();
		}
		else if (i->parameter == "VORTEX")
		{
			particles.setEffect(VORTEX);
			squeak.clear();
		}
		else
		{
			stringstream ss;
			ss << "Unknown effect '" << i->parameter << "' in line: " << i->lineno;
			gameAssert (false, ss.str());
		}
		break;
	default:
		stringstream ss;
		ss << "Unknown side effect '" << i->parameter << "' in line: " << i->lineno;
		gameAssert (false, ss.str());
		break;
	}
}



void GameImpl::refreshGame()
{
	parse("data/BUN.txt");
	bool nodeValid = (story.nodes.find(sstate.currentNodeName) != story.nodes.end());
	stringstream ss;
	ss << "Could not return to same node '" << sstate.currentNodeName + "'";

	gameAssert (nodeValid, ss.str());
	if (!nodeValid)
	{
		setCurrentNode("START");
	}

	executeCommands (getCurrentNode()->commands);
}

void GameImpl::draw(const GraphicsContext &gc)
{
	al_clear_to_color(BLACK);
	if (Engine::isDebug())
	{
		al_draw_text(Engine::getFont(), LIGHT_BLUE, 0, geth() - 16, ALLEGRO_ALIGN_LEFT, "DEBUG ON.     F5: REFRESH. F6: LOAD. F7: SAVE. F11: DEBUG OFF");
	}
	particles.draw(gc);
	text.draw(gc);
	inventory.draw(gc);

	if (state == ANSWERING)
	{
		for (AnswerComponent &comp : currentAnswers)
		{
			comp.draw();
		}
	}
}

void AnswerComponent::draw ()
{
	int xco = x;
	int yco = y;
	ALLEGRO_COLOR color = selected ? CYAN : LIGHT_GREY;
	al_draw_text(Engine::getFont(), color, xco, yco, ALLEGRO_ALIGN_LEFT, answer.text.c_str());
	if (selected) al_draw_text(Engine::getFont(), color, xco - 30, yco, ALLEGRO_ALIGN_LEFT, ">");
}

void GameImpl::parse(string fname)
{
	auto parser = Parser::build();
	story = parser->doParse(fname);
	interpreter = Interpreter::build(this, story);

	gameAssert (parser->errorNum() == 0, parser->getErrors());
}

void GameImpl::init()
{
	text.setActiveFont(Engine::getFont());
	squeak.init();
	inventory.init();
}


void GameImpl::executeCommands(vector<Command> commands)
{
	currentAnswers.clear();

	vector<Command>::iterator i = commands.begin();
	// go through all the actions
	vector<Answer> answerResult;
	interpreter->executeStatements(sstate, answerResult, i, commands.end());

	int xco = 100;
	int yco = 400;
	bool first = true;
	for (Answer a : answerResult)
	{
		AnswerComponent comp;
		comp.answer = a;
		comp.setx(xco);
		comp.sety(yco);
		yco += 20;
		comp.selected = first;
		first = false;
		currentAnswers.push_back(comp);
	}
	selectedAnswer = currentAnswers.begin();

}
