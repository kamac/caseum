#include "FASMgen.h"
#include <sstream>
#include <fstream>
#include "Defines.h"
#include "invoker.h"

FASMGenerator::FASMGenerator() {
	codeSection = new CodeSection();
	CodeLabel *startLabel = new CodeLabel(); // extern label
	codeSection->sectionName = "";
	codeSection->attribs = "";
	codeSection->labels.push_back(startLabel);
	startLabel->name = "";
	startLabel->isFunction = false;
	startLabel = new CodeLabel();
	codeSection->sectionName = "text";
	codeSection->attribs = "code";
	codeSection->labels.push_back(startLabel);
	startLabel->name = "_start";
	startLabel->isFunction = false;
	startLabel->jumpCounter = 32;
	startLabel->vars.push_back(std::map<std::string, AVar>());
	startLabel->instructions.push_back("call main");
	startLabel->instructions.push_back("ret");

	dataCounter = 32;
	callStackSize = 0; // 0 items pushed
	registerCounter = 0; // ebx
	currentLabel = 1; // start
	registers.push_back("esi");
	registers.push_back("edi");
	registers.push_back("ebx");
	registers.push_back("ecx");
	registers.push_back("edx");
	registers.push_back("eax");

	convValTable["int"] = new AVal{ "dword", 4 };
	convValTable["double"] = new AVal{ "qword", 8 };
	convValTable["string"] = new AVal{ "dword", 4 }; // pointer to string, in fact
	convValTable["void"] = new AVal{ "", 0 }; // used in functions only
}

FASMGenerator::~FASMGenerator() {
	for (unsigned int j = 0; j < codeSection->labels.size(); j++)
		delete codeSection->labels[j];
	delete codeSection;
}

void FASMGenerator::safeDecrement(unsigned int &i) {
	if ((int)i - 1 >= 0) i--;
}

FASMGenerator::CodeLabel* FASMGenerator::getLabel() {
	return codeSection->labels.at(currentLabel); // only executable has non-anonymous labels
}

unsigned int FASMGenerator::varStackSize() {
	CodeLabel* label = getLabel();
	std::map<std::string, AVar> *v = &label->vars[label->vars.size() - 1];
	unsigned int sum = 0;
	for (auto it = v->begin(); it != v->end(); it++)
		sum += it->second.type->siz;
	return sum;
}

FASMGenerator::AVar* FASMGenerator::getVar(const std::string &ref, CodeLabel* label) {
	// We start iterating from top to bottom, because the most local variables are the most relevant to us
	for (int i = label->vars.size()-1; i >= 0; i--) {
		auto it = label->vars[i].find(ref);
		if (it != label->vars[i].end())
			return &it->second;
	}
	return 0;
}

void FASMGenerator::repositionEndJMPLabel() {
	CodeLabel* label = getLabel();
	unsigned int i = label->instructions.size() - 1;
	std::stringstream searchForo;
	searchForo << "._j" << afterElse[afterElse.size() - 1] << "e:";
	std::string searchFor = searchForo.str();
	// move our forsaken jmp call to the front, since it was over-taken by condition code being generated
	while (label->instructions.at(i).find(searchFor) == std::string::npos) i--;
	std::string c = label->instructions.at(i);
	label->instructions.erase(label->instructions.begin() + i);
	label->instructions.push_back(c);
}

void FASMGenerator::GenConst(const int &c) {
	CodeLabel *label = getLabel();
	std::stringstream out;
	out << "mov " << registers[registerCounter] << "," << c;
	label->instructions.push_back(out.str());
	registerCounter++;
}

void FASMGenerator::GenConst(const double &c) {
	CodeLabel *label = getLabel();
	std::stringstream out;
	out << "mov " << registers[registerCounter] << "," << c;
	label->instructions.push_back(out.str());
	registerCounter++;
}

void FASMGenerator::GenConst(const std::string &c) {
	std::stringstream out;
	out << "_c" << dataCounter;
	std::string labelName = out.str();
	CodeLabel* dataLabel = new CodeLabel();
	dataLabel->name = out.str();
	dataLabel->instructions.push_back("db \'" + c + "\',0");
	dataLabel->isFunction = false;
	codeSection->labels.push_back(dataLabel);
	out.str("");
	out << "mov " << registers[registerCounter] << "," << labelName;
	getLabel()->instructions.push_back(out.str());
	registerCounter++;
	dataCounter++;
}

void FASMGenerator::GenVarRef(const std::string &ref) {
	CodeLabel *label = getLabel();
	AVar *var = getVar(ref,label);
	std::stringstream out;
	out << "mov " << registers[registerCounter] << "," << 
		var->type->asmtype << "[ebp" << var->posOp << var->pos << "]";
	label->instructions.push_back(out.str());
	registerCounter++;
}

void FASMGenerator::GenVarDef(const std::string &type, const std::string &name) {
	CodeLabel* label = getLabel();
	AVal *v = convValTable[type];
	label->varCounter += v->siz;
	label->vars[label->vars.size()-1][name] = AVar{ v, label->varCounter, '-' };
}

void FASMGenerator::GenOp(const char &op, const std::string &type) {
	CodeLabel* label = getLabel();
	std::string ptype = type;
	if (op == L_AND || op == L_OR) ptype = L_INT;
	std::stringstream out;
	if (ptype == L_INT) {
		bool easyExpr = true;
		std::string cmd;
		switch (op) {
		case '+':
			cmd = "add";
			break;
		case '-':
			cmd = "sub";
			break;
		case '*':
			cmd = "imul";
			break;
		case '/':
			cmd = "idiv";
			break;
		case L_EQUAL:
			easyExpr = false;
			cmd = "sete";
			break;
		case '<':
			easyExpr = false;
			cmd = "setg"; // not setl because we have switched operators
			break;
		case '>':
			easyExpr = false;
			cmd = "setl";
			break;
		case L_AND:
			cmd = "and";
			break;
		case L_OR:
			cmd = "or";
			break;
		}
		if (easyExpr) {
			out << cmd << " " << registers[registerCounter - 2] << "," << registers[registerCounter - 1];
			label->instructions.push_back(out.str());
		} else {
			out << "cmp " << registers[registerCounter - 1] << "," << registers[registerCounter - 2];
			label->instructions.push_back(out.str());
			out.str(""); out << cmd << " al";
			label->instructions.push_back(out.str());
			out.str(""); out << "movzx " << registers[registerCounter - 2] << ",al";
			label->instructions.push_back(out.str());
		}
	}
	else if (ptype == L_STR) {
		std::string call;
		switch (op) {
		case '+':
			call = FASMGEN_strConcat;
			break;
		case L_EQUAL:
			call = FASMGEN_strCompare;
			break;
		}
		out << "push " << registers[registerCounter - 1];
		label->instructions.push_back(out.str());
		out.str("");
		out << "push " << registers[registerCounter - 2];
		label->instructions.push_back(out.str());
		label->instructions.push_back("call " + call);
		label->instructions.push_back("add esp,8");
		out.str("");
		out << "mov " << registers[registerCounter - 2] << ",eax";
		label->instructions.push_back(out.str());
	}
	registerCounter -= 1;
}

void FASMGenerator::GenStore(const std::string &ref) {
	CodeLabel *label = getLabel();
	AVar *var = getVar(ref,label);
	std::stringstream out;
	safeDecrement(registerCounter);
	out << "mov " << var->type->asmtype << "[ebp-" << var->pos << "]," << registers[registerCounter];
	label->instructions.push_back(out.str());
}

void FASMGenerator::GenFunctionCall(const std::string &function) {
	std::stringstream out;
	out << "call " << function;
	for (unsigned int i = 0; i < externalFunction.size(); i++) {
		if (externalFunction[i] == function) {
			out.str("");
			out << "call [" << function << "]";
		}
	}
	getLabel()->instructions.push_back(out.str());
	out.str("");
	out << "add esp," << callStackSize;
	std::string ss = getFunctionRetVal(function);
	getLabel()->instructions.push_back(out.str());
	AVal* retVal = convValTable[ss];
	if (retVal->siz > 0) {
		out.str("");
		out << "mov " << registers[registerCounter] << ",eax";
		getLabel()->instructions.push_back(out.str());
		registerCounter++;
	}
	callStackSize = 0;
}

void FASMGenerator::GenCallArg() {
	CodeLabel *label = getLabel();
	std::stringstream out;
	safeDecrement(registerCounter);
	out << "push " << registers[registerCounter];
	label->instructions.push_back(out.str());
	callStackSize += 4; // default 32-bit register size. Needs to be changed!
}

void FASMGenerator::GenExtern(const std::string &symbol, const std::string &fname) {
	CodeLabel* label = codeSection->labels.at(0); // extern label
	std::stringstream out;
	out << "extrn '" << symbol << "' as " << fname << ":dword";
	label->instructions.push_back(out.str());
	externalFunction.push_back(fname);
}

void FASMGenerator::GenFunctionDef(const std::string &fname, const std::vector<Value*> args, const std::string &type) {
	CodeLabel *newLabel = new CodeLabel();
	codeSection->labels.push_back(newLabel);
	newLabel->name = fname;
	newLabel->instructions.push_back("push ebp");
	newLabel->instructions.push_back("mov ebp,esp");
	newLabel->instructions.push_back("push edi");
	newLabel->instructions.push_back("push esi");
	newLabel->vargCounter = 8;
	newLabel->varCounter = 0;
	newLabel->isFunction = true;
	newLabel->jumpCounter = 32;
	newLabel->vars.push_back(std::map<std::string, AVar>());
	registerCounter = 0; // eax 
	for (unsigned int i = 0; i < args.size(); i++)
	{
		AVal *v = convValTable[args[i]->type];
		newLabel->vars[0][args[i]->name] = AVar{ v, newLabel->vargCounter, '+' };
		newLabel->vargCounter += v->siz;
	}
	newLabel->savedLabel = currentLabel;
	currentLabel = codeSection->labels.size() - 1;
}

void FASMGenerator::EndFunctionDef() {
	currentLabel = codeSection->labels[currentLabel]->savedLabel;
}

void FASMGenerator::GenCondition() {
	CodeLabel* label = getLabel();
	int d = 0;
	if (afterElse.size()) {
		d = 1;
		repositionEndJMPLabel();
	}
	std::stringstream out;
	out << "cmp " << registers[registerCounter - 1] << ",0";
	label->instructions.insert(label->instructions.begin() + label->instructions.size() - d, out.str());
	out.str("");
	out << "je ._j" << label->jumpCounter;
	label->instructions.insert(label->instructions.begin() + label->instructions.size() - d, out.str());
	label->vars.push_back(std::map<std::string, AVar>());
	label->jumpStack.push_back(label->jumpCounter);
	label->jumpCounter++;
	safeDecrement(registerCounter);
}

void FASMGenerator::GenConditionEnd() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	unsigned int v = label->jumpStack.at(label->jumpStack.size() - 1);
	label->jumpStack.erase(label->jumpStack.begin() + label->jumpStack.size() - 1);
	out << "._j" << v;
	if (!afterElse.size()) {
		label->instructions.push_back("jmp " + out.str() + "e");
	} else {
		std::stringstream pout;
		pout << "jmp ._j" << afterElse[afterElse.size() - 1] << "e";
		label->instructions.push_back(pout.str());
	}
	label->instructions.push_back(out.str() + ":");
	if (!afterElse.size())
		label->instructions.push_back(out.str() + "e:");
	else {
		repositionEndJMPLabel();
		afterElse.erase(afterElse.begin() + afterElse.size() - 1);
	}
	label->varCounter -= varStackSize(); // reduce number of occupied bytes
	label->vars.erase(label->vars.begin() + label->vars.size() - 1); // pop all local variable definitions
}

void FASMGenerator::GenElse() {
	CodeLabel* label = getLabel();
	std::string instruction = label->instructions.at(label->instructions.size() - 3);
	size_t a = instruction.find_first_of('j',1) + 1;
	size_t b = instruction.find_last_of('e');
	unsigned int number = atoi(instruction.substr(a, b).c_str());
	afterElse.push_back(number);
	label->vars.push_back(std::map<std::string, AVar>());
}

void FASMGenerator::GenReturn() {
	CodeLabel *label = getLabel();
	std::stringstream out;
	safeDecrement(registerCounter);
	out << "mov eax," << registers[registerCounter];
	label->instructions.push_back(out.str());
	out.str("");
	out << "jmp ._j31";
	label->instructions.push_back(out.str());
}

void FASMGenerator::GenConversion(const std::string &oldType, std::string &newType) {
	CodeLabel *label = getLabel();
	std::stringstream out;
	safeDecrement(registerCounter);
	if (newType == L_STR) {
		if (oldType == L_INT) {
			label->instructions.push_back("push " + registers[registerCounter]);
			label->instructions.push_back("call " FASMGEN_intToStr);
			label->instructions.push_back("add esp,4");
			label->instructions.push_back("mov " + registers[registerCounter] + ",eax");
		}
	}
	registerCounter++;
}

void FASMGenerator::GenForInit() {
	CodeLabel* label = getLabel();
	label->vars.push_back(std::map<std::string, AVar>());
}

void FASMGenerator::GenForConditionBegin() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	out << "jmp ._j" << label->jumpCounter << "m";
	label->instructions.push_back(out.str());
	out.str("");
	out << "._j" << label->jumpCounter << ":";
	label->instructions.push_back(out.str());
	loopLabels.push_back(label->jumpCounter);
	label->jumpStack.push_back(label->jumpCounter);
	label->jumpCounter++;
}

void FASMGenerator::GenForConditionMid() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	out << "._j" << label->jumpCounter - 1 << "m:";
	label->instructions.push_back(out.str());
}

void FASMGenerator::GenForConditionEnd() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	out << "cmp " << registers[registerCounter - 1] << ",0";
	label->instructions.push_back(out.str());
	out.str("");
	out << "je ._j" << label->jumpCounter-1 << "e";
	label->instructions.push_back(out.str());
	safeDecrement(registerCounter);
}

void FASMGenerator::GenForEnd() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	unsigned int v = label->jumpStack.at(label->jumpStack.size() - 1);
	label->jumpStack.erase(label->jumpStack.begin() + label->jumpStack.size() - 1);
	out << "._j" << v;
	label->instructions.push_back("jmp " + out.str());
	label->instructions.push_back(out.str() + "e:");
	label->varCounter -= varStackSize(); // reduce number of local variables
	label->vars.erase(label->vars.begin() + label->vars.size() - 1); // pop all local variable definitions
	loopLabels.erase(loopLabels.begin() + loopLabels.size() - 1);
}

void FASMGenerator::GenContinue() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	out << "jmp ._j" << loopLabels[loopLabels.size()-1];
	label->instructions.push_back(out.str());
}

void FASMGenerator::GenBreak() {
	CodeLabel* label = getLabel();
	std::stringstream out;
	out << "jmp ._j" << loopLabels[loopLabels.size() - 1] << "e";
	label->instructions.push_back(out.str());
}

void FASMGenerator::Compile(const std::string &output, const std::vector<std::string> &args) {
	std::string code = ""
		"format MS COFF\n"
		"extrn " FASMGEN_strConcat "\n"
		"extrn " FASMGEN_intToStr "\n"
		"extrn " FASMGEN_strCompare "\n";
	for (unsigned int i = 0; i < codeSection->labels[0]->instructions.size(); i++)
		code += codeSection->labels[0]->instructions[i] + "\n";
	code += "section \'." + codeSection->sectionName + "\' " + codeSection->attribs + "\n";
	for (unsigned int j = 1; j < codeSection->labels.size(); j++) {
		CodeLabel *label = codeSection->labels[j];
		if (label->name != "") {
			code += label->name + ":\n";
			if (label->isFunction) {
				std::stringstream out;
				if (label->varCounter > 0) {
					out << "sub esp," << label->varCounter;
					label->instructions.insert(label->instructions.begin() + 4, out.str());
				}
				out.str("");
				out << "add esp," << label->varCounter;
				label->instructions.push_back("._j31:");
				label->instructions.push_back(out.str());
				label->instructions.push_back("pop esi");
				label->instructions.push_back("pop edi");
				label->instructions.push_back("mov esp,ebp");
				label->instructions.push_back("pop ebp");
				label->instructions.push_back("ret");
			}
		}
		for (unsigned int n = 0; n < label->instructions.size(); n++) {
			code += label->instructions.at(n) + "\n";
		}
	}
	std::ofstream out;
	out.open("tmp.asm");
	out.write(code.c_str(), code.length());
	out.close();
	std::stringstream c;
	c << "FASM/FASM tmp.asm tmp.o";
	Invoke(c.str());
	//Delete("tmp.asm");
	c.str("");
	c << "ld tmp.o -o" << output.substr(0,output.find_last_of('.')) << APPEXT <<
		" ../lib/CAScore/CAScore.o ../lib/libkernel32.a ../lib/libmsvcrt.a ../lib/libuser32.a";
	for (unsigned int i = 0; i < args.size(); i++)
		c << " ../lib/" << args[i];
	Invoke(c.str());
}