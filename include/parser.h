#ifndef _BUN_PARSER_H_
#define _BUN_PARSER_H_

#include <map>
#include <string>
#include <memory>
#include <vector>
#include <map>

struct ALLEGRO_COLOR;

class SimpleState
{
public:
	std::map<std::string, int> gameVariables;
	std::string currentNodeName;

	bool hasVar (const std::string &key) const
	{
		return gameVariables.find (key) != gameVariables.end();
	}

	void save();
	bool load();
};

enum CommandType {
	TEXT, IF, ELSE, ELSIF, ENDIF, ANSWER, SET, UNSET, TOGGLE, LET, EFFECT, PASS, END, GOTO,
	IMAGE, SAMPLE
};

class Command // a bit of text
{
public:
	Command(CommandType type, std::string parameter, int lineNo) : commandType(type), parameter(parameter), lineno(lineNo) {}

	CommandType commandType;
	std::string parameter;
	int lineno;
};

class Node
{
public:
	Node () {}
	Node (std::string title) : nodeTitle(title), commands() { }

	std::string nodeTitle; // title of a node, e.g.
	std::vector<Command> commands;
};

class Story
{
public:
	std::vector<std::string> flags; // flags, e.g. "flashlight"
	std::map<std::string, Node> nodes;

	std::string toString (); // for debugging
};

class Parser
{
	std::vector<std::string> errors;
public:
	void parseAssert(bool test, std::string str)
	{
#ifdef DEBUG
		if (!test)
		{
			errors.push_back(str);
		}
#endif
	}

	int errorNum() { return errors.size(); }

	std::string getErrors();

	static std::unique_ptr<Parser> build();
	Story doParse(std::string filename);
};

class StatementHandler
{
public:
	virtual void executeSideEffect(Command *cmd) = 0;
	virtual ~StatementHandler() {}
	virtual void gameAssert(bool val, const std::string &msg) = 0;
	virtual void debugMsg(const std::string &msg, ALLEGRO_COLOR col) = 0;
};

/** Runs commands and evaluates boolean expressions*/
class ExpressionHandler
{
public:
	virtual ~ExpressionHandler() {};

	virtual bool evalAsBool(SimpleState &sstate, const std::string &test) = 0;
	virtual void execAssignment(SimpleState &sstate, const std::string &param) = 0;

	virtual bool isValid() = 0;
	virtual std::string getErrors() = 0;

	static std::unique_ptr<ExpressionHandler> build();
};

class Answer
{
public:
	std::string text;
	std::vector<Command> commands;
};

/** Runs commands and evaluates boolean expressions*/
class Interpreter
{
public:
	virtual ~Interpreter() {};

	//TODO: move save functions to Game
	static bool savedGameExists();

	virtual void executeStatement(SimpleState &sstate, std::vector<Answer> &answerResult, std::vector<Command>::iterator &i, std::vector<Command>::iterator end) = 0;
	virtual void executeStatements(SimpleState &sstate, std::vector<Answer> &answerResult, std::vector<Command>::iterator &i, std::vector<Command>::iterator end) = 0;

	virtual Answer executeAnswer(SimpleState &sstate, std::vector<Command>::iterator &i, std::vector<Command>::iterator end) = 0;;

	static std::unique_ptr<Interpreter> build(StatementHandler *handler, Story &story);
};

#endif /* _BUN_PARSER_H_ */
