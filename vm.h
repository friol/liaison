#pragma once

#ifndef VM_H
#define VM_H

#include <vector>
#include <stack>

#include "interpreter.h"

//
//
//

enum liaOpcode
{
	opReturn=0x01,
	opPostIncrement=0x02,
	opPostDecrement=0x03,
	opConstant=0x04,
	opSetLocalVariable=0x05,
	opFunctionCall=0x06,
	opStdFunctionCall=0x07,
	opAdd=0x08,
	opGetLocalVariable=0x09,
	opJumpIfLess=0x0a,
	opJump=0x0b,
	opJumpIfGreaterEqual=0x0c,
	opArrayInitializer=0x0d,
};

struct liaConstantPool
{
	std::vector<liaVariable> pool;
};

struct liaVMVariable
{
	unsigned int id;
	std::string name;
	liaVariableType type;
	std::variant<bool, int, long long, std::string> value;
};

struct liaCodeChunk
{
	std::vector<unsigned char> code;
	std::vector<liaVariable> env;

	bool findVar(std::string varName, unsigned int& varId)
	{
		for (unsigned int pos = 0;pos < env.size();pos++)
		{
			if (env[pos].name == varName)
			{
				varId = pos;
				return true;
			}
		}

		return false;
	}
};

class liaVM
{
private:

	liaConstantPool constantPool;
	std::stack<liaVariable> vmstack;
	std::vector<liaCodeChunk> chunks;

	unsigned char findOrAddConstantToConstantPool(liaVariable& constz);

	void innerPrint(liaVariable& var);
	void compileJumpTo(unsigned short int addr, liaCodeChunk& chunk);
	liaVariable getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos,unsigned int& bytesRead);

	unsigned short int compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	liaVariableType compileExpression(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);

	void compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaCodeChunk& chunk);

public:

	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList);
	void exeCuteProgram();

	liaVM();
	~liaVM();
};

#endif
