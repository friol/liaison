
//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// liaiason is typeless (will check if a variable can change type or not, probably not)
// one-line comments //
// variable initialization -> a=2, b="string", c=-2, d=0.5, e=3*2, f=[]
// strings will have default properties like s.length
// main function declaration, in the form of fn start(params)
// general function declaration, in the form of fn funName(p1,p2,p3)
// print function included by default
// function call
// if/else if/else statement 
// for cycle
// for v in array
// many other things that don't come to mind at the moment
//


#include "parser.h"

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar=(R"(

	LiaProgram <- ( Stmt )*
	Stmt <- VarDeclStmt / SingleLineCommentStmt / FuncCallStatement / EndLine

	SingleLineCommentStmt <- '//' [^\r\n]* EndLine

	VarDeclStmt <- VariableName '=' Expression ';'  EndLine
	VariableName <- < [a-zA-Z][0-9a-zA-Z]* >
	Expression <- IntegerNumber / VariableName
	IntegerNumber <- < [0-9]+ >

	FuncCallStatement <- FuncName '(' ( ArgList )* ')' ';' EndLine
	FuncName <- < [a-zA-Z][0-9a-zA-Z]* >
	ArgList <- Expression ( ',' Expression )*

	EndLine <- [\r\n]
	%whitespace <- [ \t]*
	)");

	pegParser = new peg::parser;

	pegParser->set_logger([](size_t line, size_t col, const std::string& msg, const std::string& rule) 
	{
		std::cerr << line << ":" << col << ": " << msg << "\n";
	});

	auto grammarIsOk = pegParser->load_grammar(grammar);
	assert(grammarIsOk);
}

int liaParser::parseCode(std::string c)
{
	pegParser->enable_ast();
	std::shared_ptr<peg::Ast> ast;
	if (pegParser->parse(c.c_str(), ast))
	{
		//ast = pegParser->optimize_ast(ast);
		std::cout << peg::ast_to_s(ast);
	}
	else
	{
		std::cout << "Error parsing code." << std::endl;
	}

	return 0;
}

liaParser::~liaParser()
{
	delete(pegParser);
}
