#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <map>
#include <chrono>
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
	longint,
	string,
	array,
	floatingPoint
};

struct liaVariable
{
	std::string name;
	liaVariableType type;
	std::variant<bool, int, long long, std::string> value;
	std::vector<liaVariable> vlist;

	bool operator== (const liaVariable& n1)
	{
		return n1.value== this->value;
	};
};

struct liaEnvironment
{
	// a map in this context is like 30x faster than a vector
	std::map<std::string, liaVariable> varMap;
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

	liaVariable exeCuteMethodCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,std::string varName);
	liaVariable exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,int inc);
	void exeCuteRshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteMultiplyStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	liaVariable exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	liaVariable exeCuteIfStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	
	void exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env);
	liaVariable exeCuteLibFunctionReadFile(std::string fname,int linenum);

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
