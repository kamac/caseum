#include "lexer.h"
#include <algorithm>
#include <stdlib.h>
#include "Defines.h"
#include <sstream>

Lexer::Lexer(Error* errHandler) {
	code = 0;
	this->errHandler = errHandler;
	NullValues();
	reversed = false;
}

void Lexer::NullValues() {
	tokIdentifier = "";
	tokValue_number = 0;
	tokValue_str = "";
	token = TOK_EOF;
	codeIt = 0;
}

void Lexer::SetCode(std::string *code) {
	NullValues();
	this->code = code;
	// TODO. Change all \r\n to \n
}

int Lexer::getChar(int add) {
	if (add < 0 && codeIt == 0) return -1; // this'd be a big mistake if this happened
	if (codeIt < code->length()) {
		codeIt += add;
		char c = code->at(codeIt - 1);
		if (c == '\n')
			errHandler->currLine += add;
		return c;
	}
	return -1;
}

int Lexer::peekChar() {
	if (codeIt < code->length())
		return code->at(codeIt);
	return -1;
}

void Lexer::FirstToken() {
	codeIt = 0;
	NextToken();
}

int Lexer::_NextToken(int add) {
	if (add > 0) {
		if (codeIt + add > code->length()) return TOK_EOF;
	}
	while (isspace(lastChar = getChar(add)));
	if (lastChar == -1) return TOK_EOF;
	if (lastChar == '/' && peekChar() == '/') {
		// comment line. Just skip to the next line
		lastChar = getChar(add);
		while ((lastChar = getChar(add)) != '\n');
		lastChar = getChar(add);
		_NextToken(add);
	}
	if (isalpha(lastChar)) {
		// tokenize an identifier
		tokIdentifier = "";
		do {
			tokIdentifier += lastChar;
			lastChar = getChar(add);
		} while (isalnum(lastChar));
		if (lastChar != '\n') codeIt += -add;
		if (add < 0) // reverse the string if we are iterating backwards
			tokIdentifier = std::string(tokIdentifier.rbegin(), tokIdentifier.rend());
		if (tokIdentifier == "import")
			return TOK_IMPORT;
		else if (tokIdentifier == "extern")
			return TOK_EXTERN;
		else if (tokIdentifier == "return")
			return TOK_RETURN;
		else if (tokIdentifier == "if")
			return TOK_IF;
		else if (tokIdentifier == "for")
			return TOK_FOR;
		else if (tokIdentifier == "else")
			return TOK_ELSE;
		else if (tokIdentifier == "continue")
			return TOK_CONTINUE;
		else if (tokIdentifier == "break")
			return TOK_BREAK;
		return TOK_IDENTIFIER;
	}
	if (isalnum(lastChar)) {
		// tokenize a number
		bool hadDot = false; // a double/float
		bool hadX = false; // a hex number
		tokIdentifier = "";
		do {
			if (lastChar == '.') {
				if (!hadDot && !hadX)
					hadDot = true;
				else {
					errHandler->Throw("Unexpected '.'");
					tokValue_number = 0.0;
					return TOK_DOUBLE;
				}
			} else if (lastChar == 'x') {
				if (!hadX && !hadDot)
					hadX = true;
				else {
					errHandler->Throw("Unexpected 'x'");
					tokValue_number = 0;
					return TOK_INTEGER;
				}
			}
			tokIdentifier += lastChar;
			lastChar = getChar(add);
		} while (isalnum(lastChar) || (lastChar == '.' && !hadDot) || (lastChar == 'x' && !hadX));
		if (lastChar != '\n') codeIt += -add;
		if (add < 0) // reverse the string if we are iterating backwards
			tokIdentifier = std::string(tokIdentifier.rbegin(), tokIdentifier.rend());
		if (!hadX) {
			tokValue_number = atof(tokIdentifier.c_str());
			if (hadDot)
				return TOK_DOUBLE;
			else
				return TOK_INTEGER;
		} else {
			tokValue_number = strtol(tokIdentifier.c_str(), 0, 0);
			return TOK_INTEGER;
		}
	}
	if (lastChar == '\"') {
		// tokenize a string
		tokIdentifier = "";
		while ((lastChar = getChar(add)) != '\"') {
			tokIdentifier += lastChar;
		}
		if (add < 0) // reverse the string if we are iterating backwards
			tokIdentifier = std::string(tokIdentifier.rbegin(), tokIdentifier.rend());
		tokValue_str = tokIdentifier;
		return TOK_STRING;
	}
	if (lastChar == '=' && peekChar() == '=') {
		lastChar = getChar(add);
		return L_EQUAL;
	}
	if (lastChar == '|' && peekChar() == '|') {
		lastChar = getChar(add);
		return L_OR;
	}
	if (lastChar == '&' && peekChar() == '&') {
		lastChar = getChar(add);
		return L_AND;
	}
	// no idea what it might be. Return by value.
	return lastChar;
}

void Lexer::PreviousToken() {
	token = _NextToken(-1);
	reversed = true;
}

void Lexer::NextToken() {
	if (reversed) {
		token = _NextToken(1);
		reversed = false;
	}
	token = _NextToken(1);
}

std::string Lexer::TokToStr(int tok) {
	if (tok == TOK_EOF)
		return "EOF";
	if (tok == TOK_DOUBLE)
		return "double";
	if (tok == TOK_EXTERN)
		return "extern";
	if (tok == TOK_IDENTIFIER)
		return "identifier";
	if (tok == TOK_IMPORT)
		return "import";
	if (tok == TOK_INTEGER)
		return "int";
	if (tok == TOK_RETURN)
		return "return";
	if (tok == TOK_STRING)
		return "string";
	if (tok == L_AND)
		return "&&";
	if (tok == L_OR)
		return "||";
	if (tok == L_EQUAL)
		return "==";
	if (tok == TOK_IF)
		return "if";
	if (tok == TOK_FOR)
		return "for";
	if (tok == TOK_ELSE)
		return "else";
	if (tok == TOK_CONTINUE)
		return "continue";
	if (tok == TOK_BREAK)
		return "break";
	std::stringstream out;
	if (isprint(tok)) {
		out << (char)tok;
	} else {
		out << tok;
	}
	return out.str();
}