
//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// liaiason is typeless (will check if a variable can change type or not, probably not)
// one-line comments //
// variable initialization -> a=2, b="string", c=-2, d=0.5, e=3*2, f=[]
// strings will have default properties/functions like s.length
// main function declaration, in the form of fn start(params)
// general function declaration, in the form of fn funName(p1,p2,p3)
// print function included by default
// function call
// if/else if/else statement 
// for cycle
// for v in array
// file open/read commands
// many other things that don't come to mind at the moment
//


#include "parser.h"

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar=(R"(
	LiaProgram <- ( TopLevelStmt )*

	TopLevelStmt <- FuncDeclStmt / SingleLineCommentStmt / EndLine

	Stmt <- VarDeclStmt / SingleLineCommentStmt / FuncCallStatement / EndLine

	SingleLineCommentStmt <- '//' [^\r\n]* EndLine

	CodeBlock <- '{' (Stmt )* '}'

	FuncDeclStmt <- 'fn' FuncName '(' ( FuncParamList )* ')' [\r\n] CodeBlock
	FuncParamList <- FuncParam ( ',' FuncParam )*
	FuncParam <- < [a-zA-Z][0-9a-zA-Z]* >

	VarDeclStmt <- [ \t]* VariableName '=' Expression ';'  EndLine
	VariableName <- < [a-zA-Z][0-9a-zA-Z]* >
	Expression <- IntegerNumber / VariableName
	IntegerNumber <- < [0-9]+ >

	FuncCallStatement <- [ \t]* FuncName '(' ( ArgList )* ')' ';' EndLine
	FuncName <- < [a-zA-Z][0-9a-zA-Z]* >
	ArgList <- Expression ( ',' Expression )*

	EndLine <- [\r\n]
	%whitespace <- [ \t]*
	)");

	pegParser = new peg::parser();

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
	localCode = c;
	if (!pegParser->parse(localCode.c_str(), theAst))
	{
		std::cout << "liaiason was unable to parse the provided code." << std::endl;
		return 1;
	}

	return 0;
}

liaParser::~liaParser()
{
	delete(pegParser);
}
