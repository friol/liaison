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
	opArraySubscript=0x0e,
	opSetArrayElement=0x0f,
	opLibFunctionCall=0x10,
	opGetObjectLength=0x11,
	opRemoveLocalVariables=0x12,
	opVarFunctionCall=0x13,
};

class vmException : public std::exception
{
public:
	char* what()
	{
		return (char*)"VM exception!";
	}
};


struct liaCodeChunk
{
	std::vector<unsigned short int> code;
	std::vector<liaVariable> env;
	unsigned int seq = 0;

	unsigned int getNextSeq()
	{
		seq += 1;
		return seq;
	}

	bool findVar(std::string varName, unsigned short int& varId)
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

	std::vector<liaVariable> constantPool;
	std::stack<liaVariable> vmstack;
	std::vector<liaCodeChunk> chunks;

	void fatalError(std::string err);

	unsigned short int findOrAddConstantToConstantPool(liaVariable& constz);

	void innerPrint(liaVariable& var);
	void getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos,unsigned int& bytesRead,liaVariable& retvar);

	unsigned short int compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	liaVariableType compileExpression(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostDecrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarFunctionCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);

	void compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaCodeChunk& chunk);

public:

	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList);
	void exeCuteProgram();

	liaVM();
	~liaVM();
};

#endif
