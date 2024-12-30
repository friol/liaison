#pragma once

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "include/peglib.h"

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

	FuncDeclStmt,
	WhileStmt,
	ForeachStmt,
	IncrementStmt,
	DecrementStmt,
	FuncCallStmt,
	VarDeclStmt,
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
