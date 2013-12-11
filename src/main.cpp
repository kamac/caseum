#include <iostream>
#include "lexer.h"
#include "parser.h"
#include "FASMgen.h"
#include <fstream>
#include <iterator>


int main(int argc, char** argv) {
	Error errHandler;
	errHandler.currLine = 0;
	Lexer lexer(&errHandler);
	Parser parser(&lexer);
	CodeGenerator *codeGen = new FASMGenerator();
	codeGen->getFunctionRetVal =
		[&parser](const std::string &s) -> std::string { return parser.functionDefinitions[s]->GetType(); };

	std::string filename;
	std::vector<std::string> compileArgs;
	if (argc <= 1)
		return 0;
	for (unsigned int i = 2; i < argc; i++)
		compileArgs.push_back(argv[i]);
	filename = argv[1];
	//std::cout << "compile> ";
	//std::cin >> filename;

	std::ifstream filehandle;
	filename += ".cas";
	filehandle.open(filename);
	std::string contents((std::istreambuf_iterator<char>(filehandle)), std::istreambuf_iterator<char>());
	unsigned int lines = std::count(contents.begin(), contents.end(), '\n');
	errHandler.files.push_back(new Error::file{ filename, lines+1 });
	filehandle.close();

	lexer.SetCode(&contents);
	lexer.NextToken();
	std::string imported = parser.GetImportedCode();
	imported += contents;
	contents = imported;
	lexer.FirstToken();

	std::vector<CodeGen*> gens;
	unsigned int errors;
	std::cout << "Parsing.." << std::endl;
	while (lexer.token != Lexer::TOK_EOF) {
		errors = errHandler.errCounter;
		CodeGen *gen = parser.Parse();
		if (!gen || errHandler.errCounter > errors) continue;
		gens.push_back(gen);
	}
	std::cout << "Generating code.." << std::endl;
	for (unsigned int i = 0; i < gens.size(); i++)
		gens[i]->GenerateCode(codeGen, &errHandler);

	std::cout << "Compiling.." << std::endl;
	if (errHandler.errCounter <= 0)
		codeGen->Compile(filename,compileArgs);
	else
		std::cout << "Compile failed with " << errHandler.errCounter << " error(s)" << std::endl;
	for (unsigned int i = 0; i < gens.size(); i++)
		delete gens[i];
	delete codeGen;

	std::cout << "Done." << std::endl;
	system("pause");
	return 0;
}