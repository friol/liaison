
//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// liaison is typeless (and variables can't change type)
// one-line comments // DONE
// variable initialization -> a=2, b="string", c=-2, d=0.5, e=3*2, f=[]
// strings will have default properties/functions like s.length // DONE
// main function declaration, in the form of fn main(params) // DONE
// general function declaration, in the form of fn funName(p1,p2,p3) // DONE
// print function included by default // DONE
// function call
// if/else statement // else doesn't work
// while statement // DONE
// for cycle
// for v in array
// readTextFileLineByLine function (returns an array of strings)
// variable increment // DONE
// variable decrement
// the ability to solve AOC problems, at least up to day 14
// many other things that don't come to mind at the moment
//

#include "parser.h"

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar=(R"(
	LiaProgram <- ( TopLevelStmt )*

	TopLevelStmt <- FuncDeclStmt / SingleLineCommentStmt / EndLine

	Stmt <- VarDeclStmt / SingleLineCommentStmt / FuncCallStmt / IncrementStmt / IfStmt / WhileStmt / EndLine

	CodeBlock <- [ \t]* '{' ( Stmt )* [ \t]* '}'

	SingleLineCommentStmt <- [ \t]* '//' [^\r\n]* EndLine

	IfStmt <- [ \t]* 'if' '(' Condition ')' [\r\n] CodeBlock ('else' '\n' CodeBlock)?

	IncrementStmt <- [ \t]* VariableName '+=' Expression ';' EndLine

	WhileStmt <- [ \t]* 'while' '(' Condition ')' [\r\n] CodeBlock

	Condition <- Expression Relop Expression

	Relop <- '==' / '<=' / '<' / '>=' / '>' / '!='

	FuncDeclStmt <- 'fn' FuncName '(' ( FuncParamList )* ')' [\r\n] CodeBlock
	FuncParamList <- FuncParam ( ',' FuncParam )*
	FuncParam <- < [a-zA-Z][0-9a-zA-Z]* >

	VarDeclStmt <- [ \t]* VariableName '=' Expression ';'  EndLine
	VariableName <- < [a-zA-Z][0-9a-zA-Z]* >
	Expression <- IntegerNumber / StringLiteral / VariableWithProperty / VariableName
	
	IntegerNumber <- < [0-9]+ >
	StringLiteral <- < '\"' [^\r\n\"]* '\"' >

	VariableWithProperty <- VariableName '.' Property
	Property <- 'length'

	FuncCallStmt <- [ \t]* FuncName '(' ( ArgList )* ')' ';' EndLine
	FuncName <- < [a-zA-Z][0-9a-zA-Z]* >
	ArgList <- Expression ( ',' Expression )*

	EndLine <- [ \t]* [\r\n]
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
		std::cout << "liaison was unable to parse the provided code." << std::endl;
		return 1;
	}

	return 0;
}

liaParser::~liaParser()
{
	delete(pegParser);
}
