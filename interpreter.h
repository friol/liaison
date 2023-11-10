#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <regex>
#include <variant>
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
	int line; // oh gosh, please fix this
	std::string name;
	liaVariableType type;
	std::variant<bool, int, std::string> value;
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

	void exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);

	void exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env);

	void addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env);

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
