#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include <regex>
#include "include/peglib.h"

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
	void exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst);
	void exeCuteStatement(std::shared_ptr<peg::Ast> theAst);

	void exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst);

public:

	liaInterpreter();
	int validateAst(std::shared_ptr<peg::Ast> theAst);
	void getFunctions(std::shared_ptr<peg::Ast> theAst);
	void dumpFunctions();
	void exeCute(std::shared_ptr<peg::Ast> theAst);
	~liaInterpreter();

};

#endif
