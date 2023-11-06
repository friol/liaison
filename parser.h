#pragma once

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "include/peglib.h"


class liaParser
{
private:

	peg::parser* pegParser;

	std::string localCode;

public:

	std::shared_ptr<peg::Ast> theAst;

	liaParser();
	int parseCode(std::string c);
	~liaParser();

};

#endif
