
//
// mandatory meaningless header
// (c) friol 2023
//
// will have to support the following:
// liaison is dynamically typed (and variables can't change type)
// we have implicit variable declaration, despite what "crafting interpreters" says
// check if evaluating a function calls it two times // DONE
// convert (almost) all the assertions to ifs
// convert arraylist to expression list
// circuit breaking if statements with && 
// one-line comments // DONE
// multi-line comments // DONE
// variable initialization -> a=2, b="string", c=-2, d=0.5, e=3*2, f=[]
// dictionaries // DONE
// complex expressions (mathematical) // DONE
// logical expressions (and, or, etc.) // DONE
// strings will have default properties/functions like s.length // DONE
// d.keys for dictionaries // DONE
// main function declaration, in the form of fn main(params) // DONE
// input parameters to main // DONE
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
// foreach v in array (with arrays and strings) // DONE
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

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar=(R"(
	LiaProgram <- ( TopLevelStmt )*

	TopLevelStmt <- FuncDeclStmt / SingleLineCommentStmt / MultiLineCommentStmt / EndLine

	Stmt <- IfStmt / FuncCallStmt / VarDeclStmt / SingleLineCommentStmt / MultiLineCommentStmt / IncrementStmt / DecrementStmt / VarFuncCallStmt / 
			RshiftStmt / LshiftStmt / MultiplyStmt / LogicalAndStmt / LogicalOrStmt / WhileStmt / ForeachStmt / ReturnStmt / ArrayAssignmentStmt / 
			DivideStmt / ModuloStmt / EndLine

	CodeBlock <- [ \t]* '{' ( Stmt )* [ \t]* '}'

	SingleLineCommentStmt <- [ \t]* '//' [^\r\n]* EndLine
	MultiLineCommentStmt <- [ \t]* '/*' [^*/]* '*/' EndLine

	IfStmt <- [ \t]* 'if' '(' Condition ')' [\r\n] CodeBlock '\n' ('else' '\n' CodeBlock)?

	IncrementStmt <- [ \t]* (ArraySubscript / VariableName) '+=' Expression ';' EndLine
	DecrementStmt <- [ \t]* VariableName '-=' Expression ';' EndLine
	RshiftStmt <- [ \t]* VariableName '>>=' Expression ';' EndLine 
	LshiftStmt <- [ \t]* VariableName '<<=' Expression ';' EndLine 
	MultiplyStmt <- [ \t]* VariableName '*=' Expression ';' EndLine 
	DivideStmt <- [ \t]* VariableName '/=' Expression ';' EndLine 
	ModuloStmt <- [ \t]* VariableName '%=' Expression ';' EndLine 
	LogicalAndStmt <- [ \t]* VariableName '&=' Expression ';' EndLine 
	LogicalOrStmt <- [ \t]* VariableName '|=' Expression ';' EndLine 

	WhileStmt <- [ \t]* 'while' '(' Condition ')' [\r\n] CodeBlock
	ForeachStmt <- [ \t]* 'foreach' '(' VariableName 'in' VariableName ')' [\r\n] CodeBlock

	Condition <- InnerCondition ( CondOperator Condition )*
	InnerCondition <- Expression Relop Expression / '(' Condition ')'
	CondOperator <- '&&' / '||'

	Relop <- '==' / '<=' / '<' / '>=' / '>' / '!='

	FuncDeclStmt <- 'fn' FuncName '(' ( FuncParamList )* ')' [\r\n] CodeBlock
	FuncParamList <- FuncParam ( ',' FuncParam )*
	FuncParam <- < [a-zA-Z][0-9a-zA-Z]* >

	VarFuncCallStmt <- [ \t]* VariableName '.' FuncName '(' ( ArgList )* ')' ';' EndLine

	ArrayAssignmentStmt <- [ \t]* ArraySubscript '=' Expression ';' EndLine
	VarDeclStmt <- [ \t]* VariableName '=' Expression ';'  EndLine
	VariableName <- < [a-zA-Z][0-9a-zA-Z]* >

	Expression <- InnerExpression ( ExprOperator InnerExpression )*
	InnerExpression <- BooleanConst / IntegerNumber / StringLiteral / ArrayInitializer / DictInitializer / RFuncCall / 
					   BitwiseNot / ArraySubscript / VariableWithFunction / VariableWithProperty / VariableName / '(' Expression ')'
	ExprOperator <- '+' / '-' / '*' / '/' 

	BooleanConst <- < 'true' > / < 'false' >
	IntegerNumber <- < ('-')?[0-9]+ >
	StringLiteral <- < '\"' [^\r\n\"]* '\"' >
	ArrayInitializer <- '[' (ArrayList)* ']'
	DictInitializer <- '{' (DictList)* '}'
	BitwiseNot <- '~' Expression

	DictList <- KeyValueList
	ArrayList <- IntegerList / StringList
	KeyValueList <- StringLiteral ':' Expression (',' StringLiteral ':' Expression)*
	IntegerList <- IntegerNumber (',' IntegerNumber)*
	StringList <- StringLiteral (',' StringLiteral)*

	ArraySubscript <- VariableName '[' Expression ']' ('[' Expression ']')*

	VariableWithProperty <- VariableName '.' Property
	Property <- 'length' / 'keys'
	VariableWithFunction <- VariableName '.' FuncName '(' ( ArgList )* ')'

	FuncCallStmt <- [ \t]* FuncName '(' ( ArgList )* ')' ';' EndLine
	RFuncCall <- [ \t]* FuncName '(' ( ArgList )* ')'
	FuncName <- < [a-zA-Z_][0-9a-zA-Z_]* >
	ArgList <- ('byref')? Expression ( ',' ('byref')? Expression )*

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
	if (!grammarIsOk)
	{
		std::cout << "Error: grammar loading failed." << std::endl;
	}
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
