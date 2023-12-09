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
	dictionary,
	floatingPoint
};

struct liaVariable
{
	std::string name;
	liaVariableType type;
	std::variant<bool, int, long long, std::string> value;
	std::vector<liaVariable> vlist;
	std::map<std::string,liaVariable> vMap;

	bool operator== (const liaVariable& n1)
	{
		// TODO: fix this
		return n1.value== this->value;
	};
};

struct liaEnvironment
{
	// a map is like 30x faster than a vector here
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

class interpreterException : public std::exception 
{
public:
	char* what() 
	{
		return (char*)"Interpreter exception";
	}
};


class liaInterpreter
{
private:

	std::vector<liaFunction> functionList;

	int validateMainFunction(std::shared_ptr<peg::Ast> theAst);

	bool evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	template <typename T> bool primitiveComparison(T leftop, T rightop, std::string relOp);

	bool arrayComparison(std::vector<liaVariable> leftop, std::vector<liaVariable> rightop, std::string relOp);

	void replaceAll(std::string& str, const std::string& from, const std::string& to);

	liaVariable evaluateExpression(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env);

	liaVariable exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);

	liaVariable exeCuteMethodCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,std::string varName);
	liaVariable exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteArrayAssignmentStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	
	void exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,int inc);
	void exeCuteRshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteLshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteLogicalAndStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteLogicalOrStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteMultiplyStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteDivideStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	void exeCuteModuloStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	
	liaVariable exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	liaVariable exeCuteForeachStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	liaVariable exeCuteIfStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	
	void innerPrint(liaVariable& var);
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
	void exeCute(std::shared_ptr<peg::Ast> theAst,std::vector<std::string> params);
	~liaInterpreter();
};

#endif
