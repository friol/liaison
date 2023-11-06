#pragma once

#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "include/peglib.h"

class liaInterpreter
{
private:

	
public:

	liaInterpreter();
	int validateAst(std::shared_ptr<peg::Ast> theAst);
	~liaInterpreter();

};

#endif
