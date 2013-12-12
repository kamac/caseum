#ifndef caseum_AST_H
#define caseum_AST_H

#include <string>
#include <vector>
#include <map>
#include "CodeGenerator.h"
#include "Defines.h"

class ExprAST : public CodeGen {
public:
	virtual ~ExprAST() { }
	virtual std::string GetType() const { return "";  }
};

//////////////////////////////////////////
// VALUES
//////////////////////////////////////////

class NumIntAST : public ExprAST {
	int value;
public:
	NumIntAST(int value) : value(value) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenConst(value);
	}
	std::string GetType() const {
		return L_INT;
	}
};

class NumDoubleAST : public ExprAST {
	double value;
public:
	NumDoubleAST(double value) : value(value) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenConst(value);
	}
	std::string GetType() const {
		return L_DOUBLE;
	}
};

class StrAST : public ExprAST {
	std::string value;
public:
	StrAST(const std::string &value) : value(value) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenConst(value);
	}
	std::string GetType() const {
		return L_STR;
	}
};

//////////////////////////////////////////
// VARIABLE RELATED
//////////////////////////////////////////

class VariableExprAST : public ExprAST {
	std::string name;
	std::string type;
public:
	VariableExprAST(const std::string &name, const std::string &type) : name(name), type(type) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenVarRef(name);
	}
	std::string& GetName() {
		return name;
	}
	std::string GetType() const {
		return type;
	}
};

//////////////////////////////////////////
// EXPRESSION RELATED
//////////////////////////////////////////

class BinaryExprAST : public ExprAST {
	ExprAST *lhs, *rhs;
	char op;
public:
	BinaryExprAST(ExprAST *lhs, ExprAST *rhs, char op) : lhs(lhs), rhs(rhs), op(op) { }
	~BinaryExprAST() {
		delete lhs;
		delete rhs;
	}
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		if (op != '=') {
			lhs->GenerateCode(generator, errHandler);
			rhs->GenerateCode(generator, errHandler);
			std::string lhsType = lhs->GetType();
			std::string rhsType = rhs->GetType();
			if (op != L_AND && op != L_OR && lhsType != rhsType)
				generator->GenConversion(rhsType,lhsType);
			generator->GenOp(op,lhsType);
		} else {
			VariableExprAST* LHS = dynamic_cast<VariableExprAST*>(lhs);
			if (!LHS) {
				errHandler->Throw("Left side of assigment operator must be an identifier");
				return;
			}
			rhs->GenerateCode(generator, errHandler);
			std::string rhsType = rhs->GetType(), lhsType = lhs->GetType();
			if (lhsType != rhsType)
				generator->GenConversion(rhsType, lhsType);
			generator->GenStore(LHS->GetName());
		}
	}
	std::string GetType() const {
		return lhs->GetType();
	}
};

class VariableDefAST : public ExprAST {
	std::string type;
	std::string name;
public:
	VariableDefAST(const std::string &type, const std::string &name) : type(type), name(name) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenVarDef(type, name);
	}
	std::string GetType() const { return type; }
	std::string GetName() const { return name; }
};

class PrototypeAST : public CodeGen {
	std::string name, type;
	bool isExtern;
public:
	std::vector<VariableDefAST*> args;
	std::map<std::string, std::string> associatedVars; // used by parser only
	PrototypeAST(const std::string &name, std::vector<VariableDefAST*> args,
		const std::string& type, bool isExtern) : name(name), args(args), type(type), isExtern(isExtern) { }
	~PrototypeAST() {
		unsigned int s = args.size();
		for (unsigned int i = 0; i < s; i++)
			delete args[i];
	}
	void GenerateCode(CodeGenerator *generator, Error* errHandler) { }
	std::string GetName() const { return name; }
	std::string GetType() const { return type; }
	bool IsExtern() const { return isExtern; }
};

class CallExprAST : public ExprAST {
	std::string callee;
	std::vector<ExprAST*> args;
	const PrototypeAST* proto;
public:
	CallExprAST(const std::string &callee, const PrototypeAST* proto, std::vector<ExprAST*> args)
		: callee(callee), args(args), proto(proto) { }
	~CallExprAST() {
		unsigned int s = args.size();
		for (unsigned int i = 0; i < s; i++)
			delete args[i];
	}
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		for (int i = args.size() - 1; i >= 0; i--) {
			args[i]->GenerateCode(generator, errHandler);
			if (i < proto->args.size()) {
				std::string thisType = args[i]->GetType(), wantedType = proto->args[i]->GetType();
				if (thisType != wantedType)
					generator->GenConversion(thisType, wantedType);
			}
			generator->GenCallArg();
		}
		generator->GenFunctionCall(callee);
	}
	std::string GetType() const {
		if (!proto) return "void";
		return proto->GetType();
	}
};

class FunctionAST : public CodeGen {
	PrototypeAST* proto;
	std::vector<ExprAST*> body;
public:
	FunctionAST(PrototypeAST *proto, std::vector<ExprAST*> &body) : proto(proto), body(body) { }
	~FunctionAST() {
		delete proto;
		for (unsigned int i = 0; i < body.size(); i++)
			delete body[i];
	}
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		std::vector<CodeGenerator::Value*> vargs;
		for (unsigned int i = 0; i < proto->args.size(); i++)
			vargs.push_back(new CodeGenerator::Value{ proto->args[i]->GetType(), proto->args[i]->GetName() });
		generator->GenFunctionDef(proto->GetName(), vargs, proto->GetType(), proto->IsExtern());
		for (unsigned int i = 0; i < proto->args.size(); i++)
			delete vargs[i];
		for (unsigned int i = 0; i < body.size(); i++)
			body[i]->GenerateCode(generator, errHandler);
		generator->EndFunctionDef();
	}
};

class ReturnAST : public ExprAST {
	ExprAST* rhs;
public:
	ReturnAST(ExprAST* rhs) : rhs(rhs) { }
	~ReturnAST() { delete rhs; }
	void GenerateCode(CodeGenerator* generator, Error* errHandler) {
		rhs->GenerateCode(generator, errHandler);
		generator->GenReturn();
	}
};

class ConditionAST : public ExprAST {
	ExprAST* condition;
	std::vector<ExprAST*> body;
	ConditionAST* elseCondition;
public:
	ConditionAST(ExprAST* condition, std::vector<ExprAST*> &body)
		: condition(condition), body(body), elseCondition(0) { }
	ConditionAST(ExprAST* condition, std::vector<ExprAST*> &body, ConditionAST* elseCondition)
		: condition(condition), body(body), elseCondition(elseCondition) { }
	~ConditionAST() {
		delete condition;
		delete elseCondition;
		for (unsigned int i = 0; i < body.size(); i++)
			delete body[i];
	}
	void GenerateCode(CodeGenerator* generator, Error* errHandler) {
		condition->GenerateCode(generator, errHandler);
		generator->GenCondition();
		for (unsigned int i = 0; i < body.size(); i++)
			body[i]->GenerateCode(generator, errHandler);
		generator->GenConditionEnd();
		if (elseCondition) {
			generator->GenElse();
			elseCondition->GenerateCode(generator,errHandler);
		}
	}
};

class ForLoopAST : public ExprAST {
	ExprAST* init, *condition, *loop;
	VariableDefAST* vinit;
	std::vector<ExprAST*> body;
public:
	ForLoopAST(VariableDefAST* vinit, ExprAST* init, ExprAST* condition,
		ExprAST* loopAction, std::vector<ExprAST*> &body) :
		vinit(vinit), init(init), condition(condition), loop(loopAction), body(body) { }
	~ForLoopAST() {
		delete vinit;
		delete init;
		delete condition;
		delete loop;
		for (unsigned int i = 0; i < body.size(); i++)
			delete body[i];
	}
	void GenerateCode(CodeGenerator* generator, Error* errHandler) {
		generator->GenForInit();
		if (vinit) vinit->GenerateCode(generator, errHandler); // we allow to do: for([int]myvar, ...)
		if (init) init->GenerateCode(generator, errHandler);
		generator->GenForConditionBegin();
		loop->GenerateCode(generator, errHandler); // what happens each loop
		generator->GenForConditionMid();
		condition->GenerateCode(generator, errHandler);
		generator->GenForConditionEnd();
		for (unsigned int i = 0; i < body.size(); i++)
			body[i]->GenerateCode(generator, errHandler);
		generator->GenForEnd();
	}
};

class ContinueAST : public ExprAST {
public:
	ContinueAST() { }
	void GenerateCode(CodeGenerator* generator, Error* errHandler) {
		generator->GenContinue();
	}
};

class BreakAST : public ExprAST {
public:
	BreakAST() { }
	void GenerateCode(CodeGenerator* generator, Error* errHandler) {
		generator->GenBreak();
	}
};


//////////////////////////////////////////
// OTHER
//////////////////////////////////////////

class ExternAST : public CodeGen {
	std::string symbol, fname;
	bool isDeclspec;
public:
	ExternAST(const std::string &symbol, const std::string &fname, bool isDeclspec)
		: symbol(symbol), fname(fname) , isDeclspec(isDeclspec) { }
	void GenerateCode(CodeGenerator *generator, Error* errHandler) {
		generator->GenExtern(symbol,fname,isDeclspec);
	}
};

#endif