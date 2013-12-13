#include "parser.h"
#include <fstream>
#include <iterator>
#include "Defines.h"

Parser::Parser(Lexer* lexer) : lexer(lexer) {
	OpPrecedence['=']     =  2;
	OpPrecedence[L_AND] = 5;
	OpPrecedence[L_OR] = 5;
	OpPrecedence['<']     = 10;
	OpPrecedence['>']     = 10;
	OpPrecedence[L_EQUAL] = 10;
	OpPrecedence['+']     = 20;
	OpPrecedence['-']     = 20;
	OpPrecedence['*']     = 30;
	OpPrecedence['/']     = 30;
	currentFunc = 0;
}

Parser::~Parser() {
	for (auto it = functionDefinitions.begin(); it != functionDefinitions.end(); it++)
		delete it->second;
}

int Parser::getOpPrecedence() {
	if (OpPrecedence.find(lexer->token) == OpPrecedence.end())
		return -1;
	int prec = OpPrecedence[lexer->token];
	if (prec <= 0) return -1;
	return prec;
}

ExprAST* Parser::Error(const std::string &msg) {
	lexer->errHandler->Throw(msg);
	lexer->NextToken();
	return 0;
}

FunctionAST* Parser::ErrorF(const std::string &msg) {
	Error(msg);
	return 0;
}

VariableDefAST* Parser::ErrorD(const std::string &msg) {
	Error(msg);
	return 0;
}

std::string Parser::ErrorT(const std::string &msg) {
	Error(msg);
	return "";
}

CodeGen* Parser::Parse() {
	switch (lexer->token) {
	case '#':
		return handleFunctionDef();
	case Lexer::TOK_EOF:
		return 0;
	case Lexer::TOK_EXTERN:
		return handleExternFunc();
	case Lexer::TOK_IMPORT:
		lexer->NextToken(); // eat import, point to string
		lexer->NextToken(); // eat string
		return 0;
	default:
		return handleTopLevelExpr();
	}
}

std::string Parser::GetImportedCode() {
	std::string code = "";
	while (lexer->token == Lexer::TOK_IMPORT) {
		lexer->NextToken();
		if (lexer->token != Lexer::TOK_STRING) {
			ErrorT("Expected file to import"); continue;
		}
		std::ifstream filehandle;
		std::string filename = lexer->tokValue_str + ".cas";
		filehandle.open(filename);
		std::string filecontents((std::istreambuf_iterator<char>(filehandle)), std::istreambuf_iterator<char>());
		filehandle.close();
		unsigned int lines = std::count(filecontents.begin(), filecontents.end(), '\n');
		lexer->errHandler->files.insert(lexer->errHandler->files.begin(), new Error::file{ filename, lines+1 });
		
		Lexer tempLex(lexer->errHandler);
		Parser tempParser(&tempLex);
		tempLex.SetCode(&filecontents);
		tempLex.NextToken();
		std::string imported = tempParser.GetImportedCode();
		if (imported.length() > 0) imported += "\n";
		imported += filecontents;
		filecontents = imported;
		if (filecontents.length() > 0) filecontents += "\n";
		code += filecontents;
	}
	return code;
}

ExprAST* Parser::parseExpression() {
	ExprAST* LHS = parsePrimary();
	if (!LHS) return 0;

	return parseBinOpRHS(0, LHS);
}

ExprAST* Parser::parsePrimary() {
	switch (lexer->token) {
	case Lexer::TOK_IDENTIFIER:
		return parseIdentifierExpr();
	case Lexer::TOK_INTEGER:
		return parseIntegerExpr();
	case Lexer::TOK_DOUBLE:
		return parseDoubleExpr();
	case Lexer::TOK_STRING:
		return parseStringExpr();
	case '(':
		return parseParenExpr();
	default:
		return Error("Unknown token '" + lexer->TokToStr(lexer->token) + "' when expecting an expression");
	}
}

ExprAST* Parser::parseIdentifierExpr() {
	// Either variable reference or function call
	std::string id = lexer->tokIdentifier;
	lexer->NextToken();
	if (lexer->token != '(')
		return new VariableExprAST(id, currentFunc->associatedVars[id]);

	lexer->NextToken(); // eat (
	std::vector<ExprAST*> args;
	if (lexer->token != ')') {
		while (1) {
			ExprAST* Arg = parseExpression();
			if (!Arg) return 0;
			args.push_back(Arg);

			if (lexer->token == ')') break;
			if (lexer->token != ',') return Error("Expected ')' or ',' in function call");
			lexer->NextToken(); // eat ,
		}
	}
	lexer->NextToken(); // eat )
	//if (functionDefinitions.find(id) == functionDefinitions.end())
	//	return Error("Call to undefined function " + id);
	return new CallExprAST(id, functionDefinitions[id], args);
}

ExprAST* Parser::parseIntegerExpr() {
	ExprAST* val = new NumIntAST((int)lexer->tokValue_number);
	lexer->NextToken();
	return val;
}

ExprAST* Parser::parseDoubleExpr() {
	ExprAST* val = new NumDoubleAST(lexer->tokValue_number);
	lexer->NextToken();
	return val;
}

ExprAST* Parser::parseStringExpr() {
	ExprAST* val = new StrAST(lexer->tokValue_str);
	lexer->NextToken();
	return val;
}

ExprAST* Parser::parseParenExpr() {
	lexer->NextToken(); // eat (
	ExprAST* V = parseExpression();
	if (!V) return 0;

	if (lexer->token != ')') return Error("Expected ')' in paren expression");
	lexer->NextToken(); // eat )
	return V;
}

ExprAST* Parser::parseBinOpRHS(int exprPrecedence, ExprAST* LHS) {
	while (1) {
		int TokPrec = getOpPrecedence();
		if (TokPrec < exprPrecedence)
			return LHS;

		int binOp = lexer->token;
		lexer->NextToken();

		ExprAST* RHS = parsePrimary();
		if (!RHS) return 0;

		int NextPrec = getOpPrecedence();
		if (TokPrec < NextPrec) {
			RHS = parseBinOpRHS(TokPrec + 1, RHS);
			if (!RHS) return 0;
		}

		LHS = new BinaryExprAST(LHS, RHS, binOp);
	}
}

std::string Parser::parseType() {
	if (lexer->token != '[') return ErrorT("Expected type declaration, got '" + lexer->TokToStr(lexer->token) + "'");
	lexer->NextToken(); // eat [
	if (lexer->token != Lexer::TOK_IDENTIFIER) return ErrorT("Expected type");
	std::string id = lexer->tokIdentifier;
	lexer->NextToken();
	if (lexer->token != ']') return ErrorT("Expected ']' closing type declaration");
	lexer->NextToken(); // eat ]
	return id;
}

ExprAST* Parser::handleTopLevelExpr() {
	switch (lexer->token) {
	case '[':
		return handleVarDef();
	case Lexer::TOK_RETURN:
		return handleReturn();
	case Lexer::TOK_IF:
		return handleCondition();
	case Lexer::TOK_FOR:
		return handleForLoop();
	case Lexer::TOK_BREAK:
		lexer->NextToken();
		return new BreakAST();
	case Lexer::TOK_CONTINUE:
		lexer->NextToken();
		return new ContinueAST();
	default:
		return parseExpression();
	}
}

VariableDefAST* Parser::handleVarDef() {
	std::string type = parseType();
	if (type == "") return 0;
	if (lexer->token != Lexer::TOK_IDENTIFIER) return ErrorD("Expected variable name, got '" + lexer->TokToStr(lexer->token) + "'");
	std::string id = lexer->tokIdentifier;
	lexer->NextToken();
	if (lexer->token == '=')
		lexer->PreviousToken(); // now pointing at the variable identifier
	if (currentFunc != 0)
		currentFunc->associatedVars[id] = type;
	return new VariableDefAST(type, id);
}

PrototypeAST* Parser::handleProtoDef() {
	lexer->NextToken(); // eat #
	// type
	std::string type = parseType();
	if (type == "") return 0;
	// name
	if (lexer->token != Lexer::TOK_IDENTIFIER) {
		Error("Expected function name in declaration");
		return 0;
	}
	std::string id = lexer->tokIdentifier;
	lexer->NextToken();

	if (lexer->token != '(') {
		Error("Expected ( in function declaration");
		return 0;
	}
	lexer->NextToken(); // eat (
	std::vector<VariableDefAST*> params;
	while (lexer->token != ')') {
		VariableDefAST* def = handleVarDef();
		if (!def) return 0;
		params.push_back(def);
		if (lexer->token != ',' && lexer->token != ')') {
			Error("Expected \',\'");
			return 0;
		}
		if (lexer->token == ')') break;
		lexer->NextToken(); // eat ,
	}
	lexer->NextToken(); // eat )
	if (functionDefinitions.find(id) != functionDefinitions.end()) {
		Error("Function " + id + " has already been declared");
		return 0;
	}
	bool isExtern = false;
	if (lexer->token == Lexer::TOK_EXTERN) {
		lexer->NextToken();
		isExtern = true;
	}
	PrototypeAST *proto = new PrototypeAST(id, params, type, isExtern);
	functionDefinitions[id] = proto;
	return proto;
}

FunctionAST* Parser::handleFunctionDef() {
	PrototypeAST* proto = handleProtoDef();
	currentFunc = proto;
	for (unsigned int i = 0; i < proto->args.size(); i++)
		currentFunc->associatedVars[proto->args[i]->GetName()] = proto->args[i]->GetType();
	if (!proto) return 0;
	// parse body
	bool hadErr;
	std::vector<ExprAST*> body = parseBody(hadErr);
	if (hadErr) return 0;
	return new FunctionAST(proto, body);
}

ExternAST* Parser::handleExternFunc() {
	lexer->NextToken(); // eat extern directive
	bool isDeclspec = false;
	if (lexer->token == Lexer::TOK_IDENTIFIER && lexer->tokIdentifier == "__dll") {
		isDeclspec = true;
		lexer->NextToken();
	}
	if (lexer->token != Lexer::TOK_STRING) {
		this->Error("Expected external function symbol"); return 0;
	}
	std::string symbol = lexer->tokValue_str;
	lexer->NextToken();
	if (lexer->token != ',') {
		this->Error("Expected ,"); return 0;
	}
	lexer->NextToken(); // eat ,
	std::string type = parseType();
	if (type == "") return 0;
	if (lexer->token != Lexer::TOK_IDENTIFIER) {
		this->Error("Expected new function name"); return 0;
	}
	std::string name = lexer->tokIdentifier;
	lexer->NextToken();
	if (lexer->token != '(') {
		Error("Expected '('"); return 0;
	}
	lexer->NextToken(); // eat (
	std::vector<VariableDefAST*> argTypes;
	while (lexer->token != ')') {
		std::string t = parseType();
		if (t == "") return 0;
		argTypes.push_back(new VariableDefAST("",t));
		if (lexer->token != ',' && lexer->token != ')') {
			Error("Expected ','"); return 0;
		}
		if (lexer->token == ',') lexer->NextToken();
	}
	lexer->NextToken(); // eat )
	functionDefinitions[name] = new PrototypeAST(name, argTypes, type, true);
	return new ExternAST(symbol,name,isDeclspec);
}

ExprAST* Parser::handleCondition() {
	std::vector<ExprAST*> body;
	ExprAST* condition = 0;
	if (lexer->token == Lexer::TOK_ELSE) lexer->NextToken(); // eat "else"
	if (lexer->token == Lexer::TOK_IF) {
		lexer->NextToken(); // eat if
		if (lexer->token != '(') return Error("Expected '('");
		condition = parseParenExpr();
		if (!condition) return 0;
	}
	if (lexer->token != '{') return Error("Expected '{' opening condition body");
	lexer->NextToken(); // eat {
	while (lexer->token != '}') {
		body.push_back(handleTopLevelExpr());
		if (lexer->token == Lexer::TOK_EOF) return Error("Expected }, got EOF");
	}
	lexer->NextToken(); // eat }
	if (!condition)
		condition = new NumIntAST(1); // if no condition, assume if(1) { } [always true]
	if (lexer->token == Lexer::TOK_ELSE)
		return new ConditionAST(condition, body, (ConditionAST*)handleCondition());
	return new ConditionAST(condition, body);
}

ReturnAST* Parser::handleReturn() {
	lexer->NextToken(); // eat "return" keyword
	return new ReturnAST(parseExpression());
}

ExprAST* Parser::handleForLoop() {
	lexer->NextToken(); // eat 'for' keyword
	if (lexer->token != '(') return Error("Expected '('");
	lexer->NextToken(); // eat (
	VariableDefAST* varDef = 0;
	ExprAST* varRef = 0;
	if (lexer->token == '[') varDef = handleVarDef();
	if (lexer->token == Lexer::TOK_IDENTIFIER) varRef = parseExpression();
	else return Error("Expected variable identifier");
	if (lexer->token != ',') return Error("Expected ','");
	lexer->NextToken(); // eat ','
	ExprAST* untilCondition = parseExpression();
	if (lexer->token != ',') return Error("Expected ','");
	lexer->NextToken(); // eat ','
	ExprAST* loopAction = parseExpression();
	if (lexer->token != ')') return Error("Expected ')'");
	lexer->NextToken(); // eat ')'
	bool hadErr;
	std::vector<ExprAST*> body = parseBody(hadErr);
	if (hadErr) return 0;
	return new ForLoopAST(varDef, varRef, untilCondition, loopAction, body);
}

std::vector<ExprAST*> Parser::parseBody(bool &hadErr) {
	hadErr = false;
	std::vector<ExprAST*> body;
	if (lexer->token != '{') {
		Error("Expected '{' opening for-loop body");
		hadErr = true;
		return body;
	}
	lexer->NextToken(); // eat '{'
	while (lexer->token != '}') {
		body.push_back(handleTopLevelExpr());
		if (lexer->token == Lexer::TOK_EOF) {
			Error("Expected }, got EOF");
			hadErr = true;
			return body;
		}
	}
	lexer->NextToken(); // eat '}'
	return body;
}