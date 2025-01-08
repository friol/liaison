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

#include "parser.h"

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
		if (n1.value == this->value)
		{
			int idx = 0;
			for (auto& el : this->vlist)
			{
				if (el.value != n1.vlist[idx].value)
				{
					return false;
				}
				idx += 1;
			}

			return true;
		}

		return false;
	};
};

struct liaEnvironment
{
	// a map is like 30x faster than a vector here
	// an unordered_map should be even faster
	std::unordered_map<std::string, liaVariable> varMap;
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

	liaEnvironment globalScope;
	std::unordered_map<std::string, liaFunction> functionMap;

	int validateMainFunction(std::shared_ptr<peg::Ast> theAst);

	bool evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env);
	template <typename T> bool primitiveComparison(T& leftop, T& rightop, relopId& relop);

	bool arrayComparison(std::vector<liaVariable> leftop, std::vector<liaVariable> rightop, relopId& relop);

	bool liaVariableArrayComparison(liaVariable& v0, liaVariable& v1);

	void replaceAll(std::string& str, const std::string& from, const std::string& to);

	void inline evaluateExpression(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env,liaVariable& retVar);

	void exeCuteMethodCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,std::string varName,liaVariable& retVal);
	void exeCuteFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,liaVariable& retVal);
	void exeCuteVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	
	void exeCuteIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,int inc);
	void exeCuteRshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteLshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteLogicalAndStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteLogicalOrStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteMultiplyStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteDivideStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	void exeCuteModuloStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env);
	
	bool exeCuteWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, liaVariable& retVal);
	bool exeCuteForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, liaVariable& retVal);
	bool exeCuteIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, liaVariable& retVar);
	bool exeCuteCodeBlock(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,liaVariable& retVal);

	void innerPrint(liaVariable& var);
	void exeCuteLibFunctionPrint(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env);
	void exeCuteLibFunctionReadFile(std::string fname,int linenum, liaVariable& retVar);

	void customFunctionCall(std::string& fname, std::vector<liaVariable>* parameters, liaEnvironment* env,size_t lineNum,liaVariable& retVal);

	void addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env,size_t curLine);

	liaVariable* findVar(std::string& varName, size_t& curLine, liaEnvironment* env);
	void fatalError(std::string err);

public:

	liaInterpreter();
	int validateAst(std::shared_ptr<peg::Ast> theAst);

	int storeGlobalVariables(std::shared_ptr<peg::Ast> theAst);
	void getFunctions(std::shared_ptr<peg::Ast> theAst);
	liaEnvironment& getGlobalScope() { return globalScope; }
	std::unordered_map<std::string, liaFunction>& getLiaFunctions() { return functionMap; }
	void dumpFunctions();
	
	void exeCute(const std::shared_ptr<peg::Ast>& theAst,std::vector<std::string> params);
	~liaInterpreter();
};

#endif
