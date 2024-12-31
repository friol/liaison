#pragma once

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "include/peglib.h"

enum relopId
{
	RelopEqual=1,
	RelopLess,
	RelopGreater,
	RelopNotEqual,
	RelopLessEqual,
	RelopGreaterEqual
};

enum libMethodId
{
	MethodSplit=1,
	MethodAdd,
	MethodFindkey,
	MethodFind,
	MethodReplace,
	MethodSlice,
	MethodSort,
	MethodClear,
};

enum StdFunctionId
{
	FunctionPrint=1,
	FunctionReadTextFileLineByLine,
	FunctionToInteger,
	FunctionToLong,
	FunctionToString,
	FunctionOrd,
	FunctionLsqrt,
	FunctionChr,
	FunctionRnd,
	FunctionGetMillisecondsSinceEpoch,
};

enum grammarElement
{
	NotUsed=0,
	IntegerNumber=1,
	LongNumber,
	BooleanConst,
	StringLiteral,
	ArrayInitializer,
	DictInitializer,
	FuncParamList,
	ExpressionList,

	TopLevelStmt,
	FuncDeclStmt,
	WhileStmt,
	ForeachStmt,
	IncrementStmt,
	DecrementStmt,
	FuncCallStmt,
	VarDeclStmt,
	GlobalVarDecl,
	BitwiseNot,
	ReturnStmt,
	VarFuncCallStmt,
	ArrayAssignmentStmt,
	RFuncCall,
	IfStmt,
	RshiftStmt,
	LshiftStmt,
	ModuloStmt,
	LogicalAndStmt,
	LogicalOrStmt,
	MultiplyStmt,
	DivideStmt,

	CodeBlock,
	FuncName,
	MethodName,
	ArgList,
	Expression,
	InnerExpression,
	MinusExpression,
	VariableName,
	ArraySubscript,
	Condition,
	InnerCondition,
	VariableWithFunction,
	VariableWithProperty,
	NotExpression,
};

class liaParser
{
private:

	peg::parser* pegParser;

	std::string localCode;

	void postProcessAst(std::shared_ptr<peg::Ast>& ast);

public:

	std::shared_ptr<peg::Ast> theAst;

	liaParser();
	int parseCode(std::string c);
	~liaParser();

};

#endif
