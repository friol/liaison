
//
// mandatory meaningless header
// (c) friol 2023
//
// liaison is dynamically typed (and variables can't change type)
// we have implicit variable declaration, despite what "crafting interpreters" says
//

// passing parameters to functions byref
// regexs
// if else if come on
// v[x][y][...][z].length bugs
// arr[x][...][z].function()
// check why sub-indexing seems slow
// floats
// circuit breaking if statements with && 
// convert (almost) all the assertions to ifs

// convert function list to function hashmap // DONE
// allow v[x].length // DONE
// globals (yep) (but only if they start with 'glb') // DONE
// check if evaluating a function calls it two times // DONE
// convert arraylist to expression list // DONE
// foreach iterated is an Expression // DONE
// plain return without value // DONE
// monodimensional array sorting // DONE
// conditions with only booleans // DONE
// if (!Expression) // DONE
// one-line comments // DONE
// multi-line comments // DONE
// arr[x][y] gives error if arr[x] is a string // DONE
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
// for cycle (who needs for cycles anyway?)
// foreach v in array (with arrays and strings) // DONE
// readTextFileLineByLine function (returns an array of strings) // DONE
// toInteger function (converts a string to an int) // DONE
// variable increment // DONE
// variable decrement // DONE
// variable *= // DONE
// >>= operator (integer division by 2 AKA right shift) // DONE
// the ability to solve AoC problems, at least up to day 25 // DONE
// many other things that don't come to mind at the moment
//

#include "parser.h"

liaParser::liaParser()
{
	// instantiate your grammar here

	auto grammar (R"(
	LiaProgram <- ( TopLevelStmt )*

	TopLevelStmt <- FuncDeclStmt / MultiLineCommentStmt / SingleLineCommentStmt / GlobalVarDecl 

	Stmt <- IfStmt / ReturnStmt / FuncCallStmt / VarDeclStmt / MultiLineCommentStmt / SingleLineCommentStmt / IncrementStmt / DecrementStmt / VarFuncCallStmt / 
			RshiftStmt / LshiftStmt / MultiplyStmt / LogicalAndStmt / LogicalOrStmt / WhileStmt / ForeachStmt / ArrayAssignmentStmt / 
			DivideStmt / ModuloStmt 

	CodeBlock <- '{' ( Stmt )* '}'

    LineEnd      <-  '\n'
    SingleLineCommentStmt  <-  '//\n' / '//' (!LineEnd .)* LineEnd
	MultiLineCommentStmt <- '/*' [^*/]* '*/' 

	GlobalVarDecl <- VariableName '=' Expression ';'  

	IfStmt <- 'if' '(' Condition ')' CodeBlock ('else' CodeBlock)?

	IncrementStmt <- (ArraySubscript / VariableName) '+=' Expression ';' 
	DecrementStmt <- VariableName '-=' Expression ';' 
	RshiftStmt <- VariableName '>>=' Expression ';' 
	LshiftStmt <- VariableName '<<=' Expression ';'  
	MultiplyStmt <- VariableName '*=' Expression ';'  
	DivideStmt <- VariableName '/=' Expression ';'  
	ModuloStmt <- VariableName '%=' Expression ';'  
	LogicalAndStmt <- VariableName '&=' Expression ';'  
	LogicalOrStmt <- VariableName '|=' Expression ';'  

	WhileStmt <- 'while' '(' Condition ')' CodeBlock
	ForeachStmt <- 'foreach' '(' VariableName 'in' Expression ')' CodeBlock

	Condition <- InnerCondition ( CondOperator Condition )*
	InnerCondition <- Expression (Relop Expression)* / '(' Condition ')'
	CondOperator <- '&&' / '||'

	Relop <- '==' / '<=' / '<' / '>=' / '>' / '!='

	FuncDeclStmt <- 'fn' FuncName '(' ( FuncParamList )* ')' CodeBlock
	FuncParamList <- FuncParam ( ',' FuncParam )*
	FuncParam <- < [a-zA-Z][0-9a-zA-Z_]* >

	VarFuncCallStmt <- VariableName '.' FuncName '(' ( ArgList )* ')' ';' 

	ArrayAssignmentStmt <- ArraySubscript '=' Expression ';' 
	VarDeclStmt <- VariableName '=' Expression ';'  
	VariableName <- < [a-zA-Z][0-9a-zA-Z_]* >

	Expression <- InnerExpression ( ExprOperator InnerExpression )*
	InnerExpression <- BooleanConst / LongNumber / IntegerNumber / StringLiteral / ArrayInitializer / DictInitializer / NotExpression / RFuncCall / 
					   VariableWithProperty / BitwiseNot / MinusExpression / VariableWithFunction / ArraySubscript / VariableName / '(' Expression ')'
	ExprOperator <- '+' / '-' / '*' / '/' / '%' / '^'

	BooleanConst <- < 'true' > / < 'false' >
	IntegerNumber <- < ('-')?[0-9]+ >
	LongNumber <- < ('-')?[0-9]+[L] >
	StringLiteral <- '\"' < [^\r\n\"]* > '\"'
	ArrayInitializer <- '[' ExpressionList? ']'
	DictInitializer <- '{' (DictList)* '}'
	BitwiseNot <- '~' Expression
	MinusExpression <- '-' Expression
	NotExpression <- '!' Expression

	DictList <- KeyValueList
	KeyValueList <- StringLiteral ':' Expression (',' StringLiteral ':' Expression)*
	ExpressionList <- Expression (',' Expression)*

	ArraySubscript <- VariableName '[' Expression ']' ('[' Expression ']')*

	VariableWithProperty <- VariableName '.' Property /  ArraySubscript '.' Property
	Property <- 'length' / 'keys'
	VariableWithFunction <- VariableName '.' FuncName '(' ( ArgList )* ')'

	FuncCallStmt <- FuncName '(' ( ArgList )* ')' ';' 
	RFuncCall <- FuncName '(' ( ArgList )* ')'
	FuncName <- < [a-zA-Z_][0-9a-zA-Z_]* >
	ArgList <- ('byref')? Expression ( ',' ('byref')? Expression )*

	ReturnStmt <- 'return' Expression? ';' 

	%whitespace  <-  [ \t\r\n]*
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
	
	/*(*pegParser)["IntegerNumber"] = [](const peg::SemanticValues& vs)
	{
		//std::cout << "ttn" << vs.token_to_number<int>() << std::endl;
		return vs.token();
	};*/

	//(*pegParser)["IntegerNumber"]=[](const peg::SemanticValues& vs, std::any& dt) 
	//{
	//	std::cout << "action!" << std::endl;
	//};

	//pegParser = ppar;
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
	pegParser->optimize_ast(theAst);

	return 0;
}

liaParser::~liaParser()
{
	delete(pegParser);
}
