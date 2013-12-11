#ifndef caseum_PARSER_H
#define caseum_PARSER_H

#include "AST.h"
#include "lexer.h"
#include <map>

class Parser {
	Lexer *lexer;
	std::map<int, int> OpPrecedence;
	int getOpPrecedence();
	PrototypeAST* currentFunc;
	ExprAST* Error(const std::string &msg);
	FunctionAST* ErrorF(const std::string &msg);
	VariableDefAST* ErrorD(const std::string &msg);
	std::string ErrorT(const std::string &msg);

	ExprAST* handleTopLevelExpr();
	ExprAST* parsePrimary();
	ExprAST* parseExpression();
	ExprAST* parseIdentifierExpr();
	ExprAST* parseIntegerExpr();
	ExprAST* parseDoubleExpr();
	ExprAST* parseStringExpr();
	ExprAST* parseParenExpr();
	ExprAST* parseBinOpRHS(int exprPrecedence, ExprAST* LHS);
	ExprAST* handleCondition();
	ExprAST* handleForLoop();

	PrototypeAST* handleProtoDef();
	FunctionAST* handleFunctionDef();
	VariableDefAST* handleVarDef();
	ExternAST* handleExternFunc();
	ReturnAST* handleReturn();

	std::vector<ExprAST*> parseBody(bool &hadErr);
	std::string parseType();
public:
	std::map<std::string, PrototypeAST*> functionDefinitions;

	Parser(Lexer *lexer);
	~Parser();
	CodeGen* Parse();
	std::string GetImportedCode();
};

#endif