#ifndef caseum_FASMGEN_H
#define caseum_FASMGEN_H

#include <vector>
#include <map>
#include "CodeGenerator.h"

#define FASMGEN_strConcat "_cas_strConcat"
#define FASMGEN_strCompare "_cas_strCompare"
#define FASMGEN_intToStr "_cas_intToStr"

class FASMGenerator : public CodeGenerator
{
private:
	struct AVal {
		std::string asmtype;
		unsigned int siz;
	};
	struct AVar {
		AVal *type;
		unsigned int pos;
		char posOp;
	};
	struct CodeLabel {
		std::string name;
		std::vector<std::string> instructions;
		std::vector<std::map<std::string, AVar>> vars;
		unsigned int varCounter, vargCounter, savedLabel;
		unsigned int jumpCounter; // for jumps, like loops / conditionals
		bool isExternal; // used for generating .o files that can be reused as separate libs
		std::vector<unsigned int> jumpStack; // for closing jumps
		bool isFunction;
	};
	struct CodeSection {
		std::string sectionName, attribs;
		std::vector<CodeLabel*> labels;
	};
	unsigned int dataCounter; // for naming const data (like strings)
	unsigned int callStackSize;
	unsigned int currentLabel;
	unsigned int registerCounter; // ranged between eax and edi
	std::vector<unsigned int> afterElse;
	CodeSection *codeSection;
	std::vector<std::string> registers;
	std::map<std::string, AVal*> convValTable;
	std::vector<std::string> externalFunction;
	std::vector<unsigned int> loopLabels;
	CodeLabel* getLabel();
	void repositionEndJMPLabel(); // used by else cases
	void safeDecrement(unsigned int &i);
	AVar* getVar(const std::string &ref, CodeLabel* label);
	unsigned int varStackSize(); // returns the amount of bytes being held by the latest stack
public:
	FASMGenerator();
	~FASMGenerator();
	void GenConst(const int &c);
	void GenConst(const double &c);
	void GenConst(const std::string &c);
	void GenVarRef(const std::string &ref);
	void GenVarDef(const std::string &type, const std::string &name);
	void GenStore(const std::string &ref);
	void GenOp(const char &op, const std::string &type);
	void GenFunctionCall(const std::string &function);
	void GenExtern(const std::string &symbol, const std::string &fname, bool isDeclspec);
	void GenCallArg();
	void GenFunctionDef(const std::string &fname, const std::vector<Value*> args, const std::string &type, bool isExtern);
	void EndFunctionDef();
	void GenReturn();
	void GenConversion(const std::string &oldType, std::string &newType);
	void GenCondition();
	void GenConditionEnd();
	void GenElse();
	void GenForInit();
	void GenForConditionBegin();
	void GenForConditionMid();
	void GenForConditionEnd();
	void GenForEnd();
	void GenContinue();
	void GenBreak();
	void Compile(const std::string &output, const std::vector<std::string> &args);
};

#endif