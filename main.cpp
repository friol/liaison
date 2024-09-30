
//
// liaison - interpreter&compiler for the language with the same name
// (c) friol 2023-2024
//

#include <string>
#include <iostream>
#include <fstream>
#include <streambuf>
#include "parser.h"
#include "interpreter.h"
#include "compiler.h"

//
//
//

const std::string appVersion = "0.6c";

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

	std::string sourceFileName;
	if (argc == 2)
	{
		sourceFileName = argv[1];
		std::cout << sourceFileName << std::endl;
	}
	else
	{
		sourceFileName = "d:\\prova\\liaPrograms\\hello.lia";
		//sourceFileName = "d:\\prova\\liaPrograms\\aoc03.2015.lia"; // performance test // res is 2360, elapsed: 4.8s in Debug
		//sourceFileName = "d:\\prova\\liaPrograms\\test.lia";
		//sourceFileName = "d:\\prova\\liaPrograms\\ltest.lia";
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

	//std::cout << "Program read from file: " << std::endl << buffer.str() << std::endl;

	//

	liaParser theLiaParser;
	liaInterpreter theLiaInterpreter;

	if (theLiaParser.parseCode(buffer.str()) == 0)
	{
		if (theLiaInterpreter.validateAst(theLiaParser.theAst) != 0) return 1;
		theLiaInterpreter.getFunctions(theLiaParser.theAst);
		//theLiaInterpreter.dumpFunctions();
		if (theLiaInterpreter.storeGlobalVariables(theLiaParser.theAst) != 0)
		{
			return 1;
		}

		std::vector<std::string> params;
		if (argc > 2)
		{
			for (int p = 2;p < argc;p++)
			{
				std::string prm = argv[p];
				params.push_back(prm);
			}
		}

		liaCompiler theCompiler;
		theCompiler.compile(theLiaParser.theAst, params, theLiaInterpreter.getLiaFunctions());
		theCompiler.exeCute();

		/*
		try
		{
			theLiaInterpreter.exeCute(theLiaParser.theAst, params);
		}
		catch (interpreterException& ex)
		{
			ex = ex;
			return 1;
		}
		*/
	}

	return 0;
}
