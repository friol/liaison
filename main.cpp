
//
// liaison - interpreter for the language with the same name
// (c) friol 2023
//

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include "parser.h"
#include "interpreter.h"

//
//
//

const std::string appVersion = "0.1a";

void usage()
{
	std::cout << "liaison v" << appVersion << std::endl;
	std::cout << "Usage: liaison <sourcefile.lia>" << std::endl;
}

int main(int argc,char** argv)
{
	/*if (argc < 2)
	{
		usage();
		return 1;
	}*/

	std::ifstream t("d:\\prova\\prog2.lia");
	std::stringstream buffer;
	buffer << t.rdbuf();

	std::cout << "Program read from file: " << std::endl << buffer.str() << std::endl;

	liaParser theLiaParser;
	liaInterpreter theLiaInterpreter;

	if (theLiaParser.parseCode(buffer.str()) == 0)
	{
		if (theLiaInterpreter.validateAst(theLiaParser.theAst) != 0) return 1;
		theLiaInterpreter.getFunctions(theLiaParser.theAst);
		//theLiaInterpreter.dumpFunctions();
		theLiaInterpreter.exeCute(theLiaParser.theAst);
	}

	return 0;
}
