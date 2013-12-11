#ifndef caseum_LEXER_H
#define caseum_LEXER_H

#include <string>
#include "Error.h"

class Lexer {
private:
	int getChar(int add);
	int peekChar();
	void NullValues();
	int lastChar;
	int _NextToken(int add);
	bool reversed;
public:
	Error* errHandler;
	std::string *code;
	unsigned int codeIt;
	enum TOKENS {
		TOK_EOF = -1,
		TOK_IDENTIFIER = -2,
		TOK_INTEGER = -3,
		TOK_DOUBLE = -4,
		TOK_STRING = -5,
		TOK_IMPORT = -6,
		// TOK_LOAD = -7 [removed]
		TOK_EXTERN = -8,
		TOK_RETURN = -9,
		// L_EQUAL = -10
		// L_OR = -11
		// L_AND = -12
		TOK_IF = -13,
		TOK_FOR = -14,
		TOK_ELSE = -15,
		TOK_CONTINUE = -16,
		TOK_BREAK = -17
	};
	std::string tokIdentifier;
	double tokValue_number;
	std::string tokValue_str;
	int token;

	Lexer(Error *errHandler);
	void SetCode(std::string *code);
	void NextToken();
	void PreviousToken();
	void FirstToken();
	std::string TokToStr(int tok);
};

#endif