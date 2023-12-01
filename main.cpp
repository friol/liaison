
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

const std::string appVersion = "0.4a";

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

	std::string sourceFileName = "";
	if (argc == 2)
	{
		sourceFileName = argv[1];
		std::cout << sourceFileName << std::endl;
	}
	else
	{
		sourceFileName = "d:\\prova\\liaPrograms\\aoc01.2023.lia";
		//sourceFileName = "d:\\prova\\liaPrograms\\test.lia";
		//sourceFileName = "d:\\prova\\liaPrograms\\lia01.lia";
	}

	std::ifstream infile(sourceFileName);
	if (!infile)
	{
		std::cout << "Error opening source file." << std::endl;
		return 1;
	}

	std::stringstream buffer;
	buffer << infile.rdbuf();

	std::cout << "Program read from file: " << std::endl << buffer.str() << std::endl;

	//

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
