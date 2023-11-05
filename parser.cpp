//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// laiason 

#include "parser.h"

liaParser::liaParser()
{
	// instantiate your grammar here

	pegParser=new peg::parser(R"(
    Additive    <- Multitive '+' Additive / Multitive
    Multitive   <- Primary '*' Multitive / Primary
    Primary     <- '(' Additive ')' / IntegerNumber
    IntegerNumber  <- < [0-9]+ >

    %whitespace <- [ \t]*
	)");

	assert(static_cast<bool>(*pegParser) == true);
}

int liaParser::parseCode(std::string c)
{
	pegParser->enable_ast();
	std::shared_ptr<peg::Ast> ast;
	if (pegParser->parse("2+2+5+(3*4)", ast))
	{
		//std::cout << peg::ast_to_s(ast);
		ast = pegParser->optimize_ast(ast);
		std::cout << peg::ast_to_s(ast);
	}

	return 0;
}

liaParser::~liaParser()
{
	delete(pegParser);
}
