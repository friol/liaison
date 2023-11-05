
//
// liaison - interpreter for the language with the same name
// (c) friol 2023
//

#include <string>
#include <iostream>
#include "parser.h"

//
//
//

const std::string appVersion = "0.1b";

void usage()
{
	std::cout << "Laiason v" << appVersion << std::endl;
	std::cout << "Usage: laiason <sourcefile.lia>" << std::endl;
}

int main(int argc,char** argv)
{
	/*if (argc < 2)
	{
		usage();
		return 1;
	}*/

	liaParser theLiaParser;

	return 0;
}
