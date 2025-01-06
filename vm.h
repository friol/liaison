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
	opJumpIfNotEqual = 0x14,
	opJumpIfEqual = 0x15,
	opJumpIfLessEqual = 0x16,
	opNot=0x17,
	opPostMultiply=0x18,
	opPostDivide=0x19,
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
	std::vector<unsigned int> code;
	std::vector<liaVariable> basicEnv;
	unsigned int seq = 0;

	unsigned int getNextSeq()
	{
		seq += 1;
		return seq;
	}

	bool findVar(std::string varName, unsigned int& varId)
	{
		for (unsigned int pos = 0;pos < basicEnv.size();pos++)
		{
			if (basicEnv[pos].name == varName)
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

	liaVariable internalSub;
	unsigned int mainFunId = -1;

	std::vector<liaVariable> constantPool;
	std::vector<liaCodeChunk> chunks;
	std::vector<liaFunction> funList;

	void fatalError(std::string err);

	unsigned int findOrAddConstantToConstantPool(liaVariable& constz);

	void innerPrint(liaVariable& var);
	void getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos,unsigned int& bytesRead,liaVariable* retvar, std::vector<liaVariable>* env);

	unsigned int compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	liaVariableType compileExpression(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostDecrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileMultiplyStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileDivideStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarFunctionCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileReturnStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaCodeChunk& chunk);

	//void executeChunk(liaCodeChunk& chunk,liaVariable& retval,std::vector<liaVariable>* params);
	void executeChunk(liaCodeChunk& chunk, liaVariable& retval,
		unsigned int funId, unsigned int& parmBytesRead,
		liaCodeChunk& callerChunk, unsigned int callerpos, std::vector<liaVariable>* callerEnv);

	void printCompilationStats();

public:

	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList);
	void exeCuteProgram();

	liaVM();
	~liaVM();
};

#endif
