#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <regex>
#include <variant>
#include <iostream>
#include <fstream>
#include <stdlib.h>
#include "include/peglib.h"

enum liaVariableType
{
	boolean,
	integer,
	string,
	array,
	floatingPoint
};

struct liaVariable
{
	std::string name;
	liaVariableType type;
	std::variant<bool, int, std::string> value;
	std::vector<liaVariable> vlist;
};

struct liaEnvironment
{
	std::vector<liaVariable> varList;
};

struct liaFunctionParam
{
	std::string name;
};

struct liaFunction
{
	std::string name;
	std::vector<liaFunctionParam> parameters;
	std::shared_ptr<peg::Ast> functionCodeBlockAst;
};

class liaInterpreter
{
private:

	std::vector<liaFunction> functionList;

	int validateMainFunction(std::shared_ptr<peg::Ast> theAst);

	bool evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	template <typename T> bool primitiveComparison(T leftop, T rightop, std::string relOp);

	liaVariable evaluateExpression(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env);

	liaVariable exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);

	liaVariable exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,int inc);
	void exeCuteRshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteIfStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	
	void exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env);
	liaVariable exeCuteLibFunctionReadFile(std::string fname);

	liaVariable customFunctionCall(std::string fname, std::vector<liaVariable>* parameters, liaEnvironment* env);

	void addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env,size_t curLine);

	void fatalError(std::string err);

public:

	liaInterpreter();
	int validateAst(std::shared_ptr<peg::Ast> theAst);
	void getFunctions(std::shared_ptr<peg::Ast> theAst);
	void dumpFunctions();
	void exeCute(std::shared_ptr<peg::Ast> theAst);
	~liaInterpreter();

};

#endif
