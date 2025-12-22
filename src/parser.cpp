#include "parser.h"
#include <vector>
#include <sstream>
#include "strutil.h"
#include "fileutil.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include "color.h"

using namespace std;

class ExpressionHandlerImpl : public ExpressionHandler
{
	std::vector<std::string> errors;
public:
	virtual ~ExpressionHandlerImpl() {}
	ExpressionHandlerImpl() {}

	virtual bool isValid() override
	{
		return errors.size() == 0;
	}

	vector<string> tokenize(const string &test)
	{
		vector<string> tokens;

		int segstart = 0;
		for (size_t pos = 0; pos < test.length(); ++pos)
		{
			if (test[pos] == ' ')
			{
				int len = pos - segstart;
				if (len > 0)
				{
					tokens.push_back(test.substr(segstart, len));
				}
				segstart = pos + 1;
			}
			else if  (test[pos] == '(' || test[pos] == ')')
			{
				int len = pos - segstart;
				if (len > 0)
				{
					tokens.push_back(test.substr(segstart, len));
					segstart = pos;
				}
				tokens.push_back (test.substr(segstart, 1));
				segstart = pos + 1;
			}

		}

		int remain = test.length() - segstart;
		if (remain > 0)
		{
			tokens.push_back (test.substr(segstart, remain));
		}

		return tokens;
	}

	virtual bool evalAsBool(SimpleState &sstate, const string &test) override {
		errors.clear();

		vector<string> tokens = tokenize(test);

		vector<string>::iterator i = tokens.begin();
		bool result = evalExpr(sstate, i, tokens.end());

		if (i != tokens.end())
		{
			stringstream ss;
			ss << "Unhandled remainder in " << test;
			errors.push_back(ss.str());
		}
		return result;
	}

	string getErrors() {
		return join(errors, '\n');
	}

	int parseSingleIntValue(SimpleState &sstate, string &val)
	{
		int result = 0;

		if (isIntLiteral(val))
		{
			result = parseIntLiteral(val);
		}
		else if (sstate.hasVar(val))
		{
			result = getVar (sstate, val);
		}
		else
		{
			errors.push_back("Expected int literal or variable");
		}
		return result;
	}

	virtual void execAssignment(SimpleState &sstate, const string &param) override
	{
		errors.clear();
		vector<string> tokens = tokenize(param);

		vector<string>::iterator i = tokens.begin();

		bool check = sstate.hasVar(*i);
		if (!check) errors.push_back ("LET must be followed by variable");
		string destVar = *i;
		i++;

		check = *i == "=";
		if (!check) errors.push_back ("LET variable must be followed by '='");
		i++;

		int srcVal;
		if (sstate.hasVar(*i))
		{
			srcVal = getVar(sstate, *i);
		}
		else if (isIntLiteral(*i))
		{
			srcVal = parseIntLiteral(*i);
		}
		else
		{
			errors.push_back ("Unexpected value after '='");
		}
		i++;

		if (i != tokens.end())
		{
			stringstream ss;
			ss << "Unhandled remainder in " << param;
			errors.push_back (ss.str());
		}

		setVar(sstate, destVar, srcVal);
	}

	bool evalCompExpr(SimpleState &sstate, vector<string>::iterator &i, vector<string>::iterator end)
	{
		if (i == end)
		{
			errors.push_back ("Unexpected end");
			return false;
		}

		int a1 = parseSingleIntValue(sstate, *i);
		i++;

		if (i == end) { return a1 != 0; }

		string op;
		if (*i == "==" || *i == ">=" || *i == "!=" || *i == "<=" || *i == "<" || *i == ">")
		{
			op = *i;
			i++;
		}
		else
		{
			return a1 != 0;
		}

		if (i == end)
		{
			errors.push_back ("Unexpected end");
			return false;
		}

		int a2 =-0;
		if (isIntLiteral(*i) || sstate.hasVar(*i))
		{
			a2 = parseSingleIntValue(sstate, *i);
			i++;
		}

		if (op == "==")
		{
			return a1 == a2;
		}
		else if (op == ">=")
		{
			return a1 >= a2;
		}
		else if (op == "!=")
		{
			return a1 != a2;
		}
		else if (op == "<=")
		{
			return a1 <= a2;
		}
		else if (op == "<")
		{
			return a1 < a2;
		}
		else if (op == ">")
		{
			return a1 > a2;
		}
		else
		{
			errors.push_back("Unexpected operand");
			return 0;
		}

	}

	bool evalExpr(SimpleState &sstate, vector<string>::iterator &i, vector<string>::iterator end)
	{
		if (i == end)
		{
			errors.push_back("Unexpected end");
			return false;
		}

		bool result = false;

		if (isIntLiteral(*i) || sstate.hasVar(*i))
		{
			result = evalCompExpr (sstate, i, end);
		}
		else if (*i == "(")
		{
			i++;
			result = evalExpr (sstate, i, end);
			if (i == end || (*i) != ")")
			{
				errors.push_back("Unclosed parenthesis");
				return false;
			}
			i++;
		}
		else if (*i == "NOT")
		{
			i++;
			return !evalExpr (sstate, i, end);
		}
		else
		{
			errors.push_back("Unknown token");
			return false;
		}

		if (i == end) {
			return (result != 0);
		}

		if (*i == "AND")
		{
			i++;
			// assign to bool to avoid shortcut operator to kick in.
			bool rightExpr = evalExpr (sstate, i, end);
			return result && rightExpr;
		}

		if (*i == "OR")
		{
			i++;
			// assign to bool to avoid shortcut operator to kick in.
			bool rightExpr = evalExpr (sstate, i, end);
			return result || rightExpr;
		}

		return result;
	}

	int parseIntLiteral (const string &val)
	{
		if (!isIntLiteral(val)) { errors.push_back ("Expected int literal"); }
		int result = stoi(val);
		return result;
	}

	bool isIntLiteral (const string &val)
	{
		if (val.length() == 0) return false;
		unsigned int i = 0;
		if (val[i] == '-') { i++; }
		for (; i < val.length(); ++i)
		{
			if (val[i] < '0' || val[i] > '9') return false;
		}
		return true;
	}

	//TODO: duplicate.
	int getVar(SimpleState &sstate, const string &key)
	{
		if (!sstate.hasVar(key)) {
			std::stringstream ss;
			ss << "Variable: '" << key << "' not found!";
			errors.push_back(ss.str());
		}
		return sstate.gameVariables[key];
	}

	//TODO: duplicate.
	void setVar(SimpleState &sstate, const string &key, int val)
	{
		if (!sstate.hasVar(key)) {
			std::stringstream ss;
			ss << "Variable: '" << key << "' not found!";
			errors.push_back(ss.str());
		}
		sstate.gameVariables[key] = val;
	}

};

unique_ptr<ExpressionHandler> ExpressionHandler::build()
{
	return unique_ptr<ExpressionHandler>(new ExpressionHandlerImpl());
}

unique_ptr<Parser> Parser::build()
{
	return unique_ptr<Parser>(new Parser());
}

Path saveFilePath()
{
	return Path::getUserSettingsPath().join("savedata");
}

//TODO: use temp and rename pattern?
void SimpleState::save()
{
	Path path = saveFilePath();

	ofstream outfile(path.toString(), ios::out | ios::trunc);

	outfile << "NODE=" << currentNodeName << endl;

	for (auto row : gameVariables)
	{
		outfile << row.first << "=" << row.second << endl;
	}

	outfile.close();
}

//TODO: use exceptions instead of bool return value.
bool SimpleState::load()
{
	Path path = saveFilePath();

	cout << path.toString() << endl;

	ifstream infile(path.toString().c_str());

	string line;
	int lineno = 0;

	bool error = false;
	bool foundNode = false;

	gameVariables.clear();

	{
		getline(infile, line);
		lineno++;

		auto fields = split(line, '=');

		if (fields.size() != 2) {
			error = true;
		}
		else if (fields[0] != "NODE") {
			error = true;
		}
		else {
			currentNodeName = fields[1];
		}
	}

	if (error) return false;

	//TODO: add a check that all variables are read, and none are skipped.
	while (getline(infile, line))
	{
		lineno++;
		line = trim(line);

		auto fields = split(line, '=');

		if (fields.size() != 2) {
			error = true;
			break;
		}

		gameVariables[fields[0]] = stoi(fields[1]);
	}

	return !error;
}

bool Interpreter::savedGameExists()
{
	return saveFilePath().fileExists();
}

std::string Story::toString ()
{
	std::stringstream ss;
	for (std::map<std::string, Node>::iterator i = nodes.begin(); i != nodes.end(); ++i)
	{
		ss << i->first << std::endl;
	}
	return ss.str();
}

Story Parser::doParse(string fname)
{
	ifstream infile(fname.c_str());

	Story result = Story();
	Node current = Node("");

	string line;
	enum ParseState { HEADER, NODE };
	ParseState state = HEADER;
	int lineno = 0;
	while (getline(infile, line))
	{
		lineno++;
		line = trim(line);
		switch (state)
		{
			case HEADER: {
				// expect DEFINE, whitespace, comment, NODE
				if (startsWith ("DEFINE ", line))
				{
					string flagname = line.substr(string("DEFINE ").size());
					result.flags.push_back(flagname);
				}
				else if (startsWith ("NODE ", line))
				{
					string title = line.substr(string("NODE ").size());
					state = NODE;
					current = Node(title);
				}
				else if (startsWith ("--", line))
				{
					// comment, ignore
				}
				else if (line == "")
				{
					// empty, ignore
				}
				else
				{
					stringstream ss;
					ss << "Expected only DEFINE before first NODE but found something unexpected in line: " << lineno;
					parseAssert(false, ss.str());
				}
				break;
			}

			case NODE: {
				// expect NODE
				if (startsWith ("NODE ", line))
				{
					parseAssert(result.nodes.find(current.nodeTitle) == result.nodes.end(), "Duplicate node");
					result.nodes[current.nodeTitle] = current;
					current = Node(line.substr(string("NODE ").size()));
				}
				else if (startsWith ("EFFECT ", line))
				{
					string param = line.substr(string("EFFECT ").size());
					current.commands.push_back(Command(EFFECT, param, lineno));
				}
				else if (startsWith ("IMAGE ", line))
				{
					string param = line.substr(string("IMAGE ").size());
					current.commands.push_back(Command(IMAGE, param, lineno));
				}
				else if (startsWith ("SAMPLE ", line))
				{
					string param = line.substr(string("SAMPLE ").size());
					current.commands.push_back(Command(SAMPLE, param, lineno));
				}
				else if (startsWith ("ANSWER ", line))
				{
					string param = line.substr(string("ANSWER ").size());
					current.commands.push_back(Command(ANSWER, param, lineno));
				}
				else if (startsWith ("GOTO ", line))
				{
					string param = line.substr(string("GOTO ").size());
					current.commands.push_back(Command(GOTO, param, lineno));
				}
				else if (startsWith ("IF ", line))
				{
					string param = line.substr(string("IF ").size());
					current.commands.push_back(Command(IF, param, lineno));
				}
				else if (startsWith ("ELSIF ", line))
				{
					string param = line.substr(string("ELSIF ").size());
					current.commands.push_back(Command(ELSIF, param, lineno));
				}
				else if (startsWith ("ELSE", line))
				{
					current.commands.push_back(Command(ELSE, "", lineno));
				}
				else if (startsWith ("ENDIF", line))
				{
					current.commands.push_back(Command(ENDIF, "", lineno));
				}
				else if (startsWith ("PASS", line))
				{
					current.commands.push_back(Command(PASS, "", lineno));
				}
				else if (startsWith ("SET ", line))
				{
					string param = line.substr(string("SET ").size());
					current.commands.push_back(Command(SET, param, lineno));
				}
				else if (startsWith ("LET ", line))
				{
					string param = line.substr(string("LET ").size());
					current.commands.push_back(Command(LET, param, lineno));
				}
				else if (startsWith ("UNSET ", line))
				{
					string param = line.substr(string("UNSET ").size());
					current.commands.push_back(Command(UNSET, param, lineno));
				}
				else if (startsWith ("TOGGLE ", line))
				{
					string param = line.substr(string("TOGGLE ").size());
					current.commands.push_back(Command(TOGGLE, param, lineno));
				}
				else if (startsWith ("END", line))
				{
					current.commands.push_back(Command(END, "", lineno));
				}
				else if (startsWith ("----", line))
				{
					// comment, ignore
				}
				else
				{
					// check if we have accidentally a misspelled command
					int pos = line.find_first_of(' ');
					string firstword = (pos < 0) ? line : line.substr(0, pos);
					locale loc;
					if (toUpper(firstword) == firstword && toLower(firstword) != firstword && firstword.length() >= 2)
					{
						stringstream ss;
						ss << "Warning: Uppercase word " << firstword << ", which is not a command in line:" << lineno;
						parseAssert(false, ss.str());
					}

					if (line != "") line = line + " "; // separator between consecutive sentences,
					current.commands.push_back(Command(TEXT, line, lineno));
					// text node
				}
				break;
			}
		}
	}
	// insert last node...
	result.nodes[current.nodeTitle] = current;
	return result;
}

string Parser::getErrors()
{
	return join(errors, '\n');
}

class InterpreterImpl : public Interpreter
{
private:
	StatementHandler *statementHandler;
	shared_ptr<ExpressionHandler> expressionHandler;
	Story &story;

	Node *getCurrentNode(SimpleState &sstate) { return &(story.nodes[sstate.currentNodeName]); }
	int getVar(const SimpleState &sstate, const std::string &key);
	void setVar(SimpleState &sstate, const std::string &key, int val);
	void setCurrentNode(SimpleState &sstate, const std::string &id);

	// skip a section of an IF statement until the first occurrence
	// of ELSIF, ENDIF or ELSE.
	// when calling, i must be on the first line after IF
	void skipIfBlock(vector<Command>::iterator &i, vector<Command>::iterator end)
	{
		while (i != end)
		{
			switch (i->commandType)
			{
				case IF:
					// skip recursively
					i++;
					skipUntilEndif(i, end);
					break;
				case ELSE: case ELSIF: case ENDIF:
					return;
				default:
					// nothing, skipping.
					break;
			}
			if (i != end) i++;
		}

		statementHandler->gameAssert (false, "Missing ENDIF (skipIf)");
	}

	/**
	 * Evaluate a block from IF to corresponding ENDIF and everything in between.
	 * On the return, i points to the corresponding ENDIF
	 */
	void evaluateIf(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end)
	{
		bool test = (expressionHandler->evalAsBool(sstate, i->parameter));
		statementHandler->gameAssert (expressionHandler->isValid(), expressionHandler->getErrors());

		i++;

		if (test)
		{
			executeIfBlock(sstate, answerResult, i, end);
			switch (i->commandType)
			{
			case ELSE: case ELSIF:
				i++;
				skipUntilEndif(i, end);
				break;
			case ENDIF: break;
			default:
				assert (false);
				break;
			}
		}
		else
		{
			skipIfBlock(i, end);
			switch (i->commandType)
			{
			case ELSE:
				i++;
				executeIfBlock(sstate, answerResult, i, end);
				// if we have an ELSE again,
				statementHandler->gameAssert (i->commandType != ELSE, "Two ELSE in a row");
				break;
			case ELSIF:
				evaluateIf(sstate, answerResult, i, end);
				break;
			case ENDIF: break;
			default:
				assert (false);
				break;
			}
		}

		statementHandler->gameAssert (i->commandType == ENDIF, "Expected ENDIF but found something else");
	}

	// completely skip an IF...ENDIF statement disregarding ELSIF / ELSE
	// When calling, must be on the first command after IF
	void skipUntilEndif(vector<Command>::iterator &i, vector<Command>::iterator end)
	{
		while (i != end)
		{
			skipIfBlock(i, end);
			switch (i->commandType)
			{
			case ENDIF:
				// finished.
				return;
			case ELSE:
				// continue
				break;
			case ELSIF:
				// continue
				break;
			default:
				statementHandler->gameAssert (false, "skipIfBlock must leave cursor at ENDIF, ELSE or ELSIF");
				break;
			}
			if (i != end) i++;
		}

		statementHandler->gameAssert (false, "Reached END before ENDIF (skipIf)");
	}

	// execute until endif, then return.
	// when calling, i must be on the first line after IF
	void executeIfBlock(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end)
	{
		// skip until ELSE
		while (i != end)
		{
			switch (i->commandType)
			{
			case IF:
				// found nested if.
				evaluateIf(sstate, answerResult, i, end);
				break;
			case ENDIF: case ELSE: case ELSIF:
				return;
			case ANSWER: {
				Answer a = executeAnswer (sstate, i, end);
				answerResult.push_back(a);
				break;
			}
			default:
				executeStatement(sstate, answerResult, i, end);
				break;
			}
			if (i != end) i++;
		}

		statementHandler->gameAssert (false, "Reached END before ENDIF (executeIfBlock)");
	}

	bool testNodeExists (const string &id)
	{
		bool result = story.nodes.find (id) != story.nodes.end();
		if (!result)
		{
			std::stringstream ss;
			ss << "Node: '" << id << "' not found!";
			statementHandler->gameAssert(result, ss.str());
		}
		return result;
	}

public:
	InterpreterImpl(StatementHandler *handler, Story &story) : statementHandler(handler), story(story)
	{
		expressionHandler = ExpressionHandler::build();
	}

	virtual ~InterpreterImpl() {}
	virtual void executeStatement(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end) override;
	virtual void executeStatements(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end) override;

	virtual Answer executeAnswer(SimpleState &sstate, vector<Command>::iterator &i, vector<Command>::iterator end) override;
};

unique_ptr<Interpreter> Interpreter::build(StatementHandler *handler, Story &story)
{
	return unique_ptr<Interpreter>(new InterpreterImpl(handler, story));
}

void InterpreterImpl::executeStatement(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end)
{
	switch (i->commandType)
	{
	case END: case TEXT: case IMAGE: case SAMPLE: case EFFECT:
		statementHandler->executeSideEffect(&(*i));
		break;
	case SET:
		setVar(sstate, i->parameter, 1);
		break;
	case LET:
		expressionHandler->execAssignment(sstate, i->parameter);
		statementHandler->gameAssert (expressionHandler->isValid(), expressionHandler->getErrors());
		break;
	case UNSET: {
		if (i->parameter == "ALL")
		{
			for (map<string, int>::iterator i = sstate.gameVariables.begin(); i != sstate.gameVariables.end(); ++i)
			{
				i->second = 0;
			}
		}
		else
		{
			setVar(sstate, i->parameter, 0);
		}
		break;
	}
	case TOGGLE:
		setVar (sstate, i->parameter, (getVar(sstate, i->parameter) != 0) ? 0 : 1);
		break;
	case GOTO: {
		testNodeExists (i->parameter);
		setCurrentNode(sstate, i->parameter);
		vector<Command>::iterator j = getCurrentNode(sstate)->commands.begin();
		executeStatements(sstate, answerResult, j, getCurrentNode(sstate)->commands.end());
//		i = end; // GOTO is really GOSUB?
		break;
	}
	default:
		// ignore
		break;
	}
}


int InterpreterImpl::getVar(const SimpleState &sstate, const string &key)
{
	std::stringstream ss;
	ss << "Variable: '" << key << "' not found!";
	statementHandler->gameAssert (sstate.hasVar(key), ss.str());
	return sstate.gameVariables.at(key);
}

void InterpreterImpl::setVar(SimpleState &sstate, const string &key, int val)
{
	std::stringstream ss;
	ss << "Variable: '" << key << "' not found!";
	statementHandler->gameAssert (sstate.hasVar(key), ss.str());
	sstate.gameVariables[key] = val;
}

void InterpreterImpl::setCurrentNode(SimpleState &sstate, const string &id)
{
	testNodeExists (id);

	std::stringstream ss;
	ss << "DEBUG: Going to node: '" << id << "'";
	statementHandler->debugMsg(ss.str(), GREY);

	sstate.currentNodeName = id;
}

// execute statements, collecting answers...
void InterpreterImpl::executeStatements(SimpleState &sstate, vector<Answer> &answerResult, vector<Command>::iterator &i, vector<Command>::iterator end)
{
	while (i != end)
	{
		switch (i->commandType)
		{
		case ANSWER: {
			Answer a = executeAnswer (sstate, i, end);
			answerResult.push_back(a);
			break;
		}
		case IF: {
			evaluateIf (sstate, answerResult, i, end);
			break;
		}
		case PASS:
			statementHandler->gameAssert (false, "PASS without ANSWER");
			break;
		case ENDIF: case ELSE: case ELSIF:
			statementHandler->gameAssert (false, "BUG, ELSE / ELSIF / ENDIF without IF"); // must have been handled up ahead.
			break;
		default:
			executeStatement(sstate, answerResult, i, end);
			break;
		}
		if (i != end) i++; // next command
	}
}


Answer InterpreterImpl::executeAnswer(SimpleState &sstate, vector<Command>::iterator &i, vector<Command>::iterator end)
{
	Answer currentAnswer;
	currentAnswer.text = i->parameter;
	i++;

	while (i != end)
	{
		switch (i->commandType)
		{
			case ANSWER:
				// fill in default command //TODO: not sure currentNode is always correct here!!!
				currentAnswer.commands.push_back(Command(GOTO, getCurrentNode(sstate)->nodeTitle, -1));
				i--;
				return currentAnswer;
			case PASS:
				// pass ends an answer and takes you back to the current node...
				// fill in default command //TODO: not sure currentNode is always correct here!!!
				currentAnswer.commands.push_back(Command(GOTO, getCurrentNode(sstate)->nodeTitle, -1));
				return currentAnswer;
			case END: case GOTO:
				// end and goto end an asnwer
				currentAnswer.commands.push_back(*i);
				return currentAnswer;
			case IF: { // not allowed to have ifs nested inside answers...
				stringstream ss;
				ss << "Not allowed to have an IF inside an ANSWER block in line: " << i->lineno;
				statementHandler->gameAssert (false, ss.str());
				i = end;
				break;
			}
			default:
				currentAnswer.commands.push_back(*i);
				break;
		}
		i++;
	}

	return currentAnswer;
}
