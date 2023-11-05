#pragma once

#ifndef PARSER_H
#define PARSER_H

#include <string>
#include "include/peglib.h"


class liaParser
{
private:

	peg::parser* pegParser;

public:

	liaParser();
	int parseCode(std::string c);
	~liaParser();

};

#endif
