
//
// the interpreter
// liaiason - project for bored times
// (c) friol 2k23
//

#include "interpreter.h"

liaInterpreter::liaInterpreter()
{
}

int liaInterpreter::validateAst(std::shared_ptr<peg::Ast> theAst)
{
	// check0: the parsed ast should be a "LiaProgram"
	
	auto name = theAst->original_name;

	if (name != "LiaProgram")
	{
		std::cout << "Not a valid LiaProgram" << std::endl;
		return 1;
	}

	// check1: check that the provided code has a main function
	// with the correct "params" parameter

	for (auto node : theAst->nodes)
	{
		if (node->name == "TopLevelStmt")
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->name == "FuncDeclStmt")
				{
					for (auto funcNode : innerNode->nodes)
					{
						const auto& ast = *funcNode;
						if (ast.is_token)
						{
							std::string s;
							s += ast.token;
							std::cout << "[" << s << "]" << std::endl;
						}
					}
				}
			}
		}
	}



	return 0;
}

liaInterpreter::~liaInterpreter()
{

}
