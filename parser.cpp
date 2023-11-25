
//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// liaison is typeless (and variables can't change type)
// we have implicit variable declaration, despite what "crafting interpreters" says
// one-line comments // DONE
// variable initialization -> a=2, b="string", c=-2, d=0.5, e=3*2, f=[]
// complex expressions (mathematical)
// logical expressions (and, or, etc.)
// strings will have default properties/functions like s.length // DONE
// main function declaration, in the form of fn main(params) // DONE
// general function declaration, in the form of fn funName(p1,p2,p3) // DONE
// print function included by default // DONE
// variadic print function // DONE
// add (add element to array) function // DONE
// find (find in array) function // DONE
// split function // DONE
// function call with parameters // DONE
// if/else statement // DONE
// return statement // DONE
// while statement // DONE
// for cycle
// for v in array
// readTextFileLineByLine function (returns an array of strings) // DONE
// toInteger function (converts a string to an int) // DONE
// variable increment // DONE
// variable decrement // DONE
// variable *= // DONE
// >>= operator (integer division by 2 AKA right shift) // DONE
// the ability to solve AOC problems, at least up to day 14
// many other things that don't come to mind at the moment
//

#include "parser.h"
/*
Expression < -Term(TERM_OPERATOR Term) *
	Term < -Factor(FACTOR_OPERATOR Factor) *
	Factor < -IntegerNumber / StringLiteral / ArrayInitializer / RFuncCall / ArraySubscript / VariableWithProperty / VariableName / '(' Expression ')'
	TERM_OPERATOR < -<[-+] >
	FACTOR_OPERATOR < -< [/*] >
*/

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar=(R"(
	LiaProgram <- ( TopLevelStmt )*

	TopLevelStmt <- FuncDeclStmt / SingleLineCommentStmt / EndLine

	Stmt <- IfStmt / FuncCallStmt / VarDeclStmt / SingleLineCommentStmt / IncrementStmt / DecrementStmt / VarFuncCallStmt / 
			RshiftStmt / MultiplyStmt / WhileStmt / ReturnStmt / EndLine

	CodeBlock <- [ \t]* '{' ( Stmt )* [ \t]* '}'

	SingleLineCommentStmt <- [ \t]* '//' [^\r\n]* EndLine

	IfStmt <- [ \t]* 'if' '(' Condition ')' [\r\n] CodeBlock '\n' ('else' '\n' CodeBlock)?

	IncrementStmt <- [ \t]* VariableName '+=' Expression ';' EndLine
	DecrementStmt <- [ \t]* VariableName '-=' Expression ';' EndLine
	RshiftStmt <- [ \t]* VariableName '>>=' Expression ';' EndLine 
	MultiplyStmt <- [ \t]* VariableName '*=' Expression ';' EndLine 

	WhileStmt <- [ \t]* 'while' '(' Condition ')' [\r\n] CodeBlock

	Condition <- Expression Relop Expression

	Relop <- '==' / '<=' / '<' / '>=' / '>' / '!='

	FuncDeclStmt <- 'fn' FuncName '(' ( FuncParamList )* ')' [\r\n] CodeBlock
	FuncParamList <- FuncParam ( ',' FuncParam )*
	FuncParam <- < [a-zA-Z][0-9a-zA-Z]* >

	VarFuncCallStmt <- [ \t]* VariableName '.' FuncName '(' ( ArgList )* ')' ';' EndLine

	VarDeclStmt <- [ \t]* VariableName '=' Expression ';'  EndLine
	VariableName <- < [a-zA-Z][0-9a-zA-Z]* >

	Expression <- BooleanConst / IntegerNumber / StringLiteral / ArrayInitializer / RFuncCall / 
				  ArraySubscript / VariableWithFunction / VariableWithProperty / VariableName

	BooleanConst <- < 'true' > / < 'false' >
	IntegerNumber <- < ('-')?[0-9]+ >
	StringLiteral <- < '\"' [^\r\n\"]* '\"' >
	ArrayInitializer <- '[' (ArrayList)* ']'

	ArrayList <- IntegerList / StringList
	IntegerList <- IntegerNumber (',' IntegerNumber)*
	StringList <- StringLiteral (',' StringLiteral)*

	ArraySubscript <- VariableName '[' Expression ']'

	VariableWithProperty <- VariableName '.' Property
	Property <- 'length'
	VariableWithFunction <- VariableName '.' FuncName '(' ( ArgList )* ')'

	FuncCallStmt <- [ \t]* FuncName '(' ( ArgList )* ')' ';' EndLine
	RFuncCall <- [ \t]* FuncName '(' ( ArgList )* ')'
	FuncName <- < [a-zA-Z][0-9a-zA-Z]* >
	ArgList <- Expression ( ',' Expression )*

	ReturnStmt <- [ \t]* 'return' Expression ';' EndLine

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
