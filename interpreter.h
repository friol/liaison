#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <string>
#include <vector>
#include "include/peglib.h"

struct liaFunctionParam
{
	std::string name;
};

struct liaFunction
{
	std::string name;
	std::vector<liaFunctionParam> parameters;
};

class liaInterpreter
{
private:

	std::vector<liaFunction> functionList;

	int validateMainFunction(std::shared_ptr<peg::Ast> theAst);
	
public:

	liaInterpreter();
	int validateAst(std::shared_ptr<peg::Ast> theAst);
	void getFunctions(std::shared_ptr<peg::Ast> theAst);
	void dumpFunctions();
	~liaInterpreter();

};

#endif
