#ifndef caseum_CODEGEN_H
#define caseum_CODEGEN_H

#include <string>
#include "Error.h"
#include <vector>
#include <functional>

class CodeGenerator {
public:
	struct Value {
		std::string type, name;
		Value(std::string type, std::string name) : type(type), name(name) { }
	};
	std::function<std::string(const std::string&)> getFunctionRetVal;
	virtual void GenConst(const int &c) { }
	virtual void GenConst(const double &c) { }
	virtual void GenConst(const std::string &c) { }
	virtual void GenVarRef(const std::string &ref) { }
	virtual void GenVarDef(const std::string &type, const std::string &name) { }
	virtual void GenOp(const char &op, const std::string &type) { }
	virtual void GenFunctionCall(const std::string &function) { }
	virtual void GenStore(const std::string &ref) { }
	virtual void GenExtern(const std::string &symbol, const std::string &fname) { }
	virtual void GenCallArg() { }
	virtual void GenFunctionDef(const std::string &fname,const std::vector<Value*> args,const std::string& type) { }
	virtual void EndFunctionDef() { }
	virtual void GenReturn() { }
	virtual void GenConversion(const std::string &oldType, std::string &newType) { }
	virtual void GenCondition() { }
	virtual void GenConditionEnd() { }
	virtual void GenElse() { }
	virtual void GenForInit() { }
	virtual void GenForConditionBegin() { }
	virtual void GenForConditionMid() { }
	virtual void GenForConditionEnd() { }
	virtual void GenForEnd() { }
	virtual void GenContinue() { }
	virtual void GenBreak() { }
	virtual ~CodeGenerator() { }

	virtual void Compile(const std::string &output, const std::vector<std::string> &args) { }
};

class CodeGen {
public:
	virtual void GenerateCode(CodeGenerator *generator, Error* errHandler) { }
};

#endif