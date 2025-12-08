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
	opPostMultiply = 0x18,
	opPostDivide = 0x19,
	opPostModulo=0x2f,
	opPostModuloGlobal=0x30,
	opPostBitwiseOr = 0x36,
	opPostRshift = 0x37,

	opConstant=0x04,
	opGetLocalVariable = 0x09,
	opSetLocalVariable=0x05,
	opFunctionCall=0x06,
	opStdFunctionCall=0x07,
	opLibFunctionCall = 0x10,
	opVarFunctionCall = 0x13,
	opVarFunctionCallGlobal = 0x27,

	opAdd=0x08,
	opSubtract = 0x1a,
	opMultiply = 0x1b,
	opDivide = 0x1c,
	opModulo = 0x34,
	opXor = 0x35,
	opMathExpression=0x2b,
	opRemoveLocalVariables =0x2c,

	opCompareLess=0x0a,
	opCompareLessEqual = 0x16,
	opCompareGreater = 0x20,
	opCompareGreaterEqual = 0x0c,
	opCompareEqual = 0x15,
	opCompareNotEqual = 0x14,

	opJump=0x0b,
	opArrayInitializer=0x0d,
	opArraySubscript=0x0e,
	opArraySubscriptGlobal=0x2a,
	opSetArrayElement=0x0f,
	opSetArrayElementGlobal=0x26,
	opGetObjectLength=0x11,
	opGetObjectLengthGlobal=0x29,
	opGetObjectLengthOfSubscript=0x2d,
	opGetObjectLengthOfSubscriptGlobal=0x2e,
	opDictInitializer = 0x33,
	opGetDictKeys=0x31,
	opGetDictKeysGlobal=0x32,

	opNot=0x17,
	opLogicalCondition=0x1d,
	opLogicalAnd=0x1e,
	opLogicalOr=0x1f,

	opGetObjectType=0x21,
	opGetObjectTypeGlobal=0x28,
	opGetGlobalVariable=0x22,
	opSetGlobalVariable=0x23,

	opPostIncrementGlobal = 0x24,
	opPostDecrementGlobal=0x25,
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

	std::vector<liaVariable> globalEnv;

	void fatalError(std::string err);
	void replaceAll(std::string& str, const std::string& from, const std::string& to);

	unsigned int findOrAddConstantToConstantPool(liaVariable& constz);
	inline bool findGlobalVariable(std::string& vname, unsigned int& varId);

	void innerPrint(liaVariable& var);
	void getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos,unsigned int& bytesRead,liaVariable* retvar, std::vector<liaVariable>* env);

	unsigned int compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileInnerCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);

	liaVariableType compileExpression(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compilePostDecrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileMultiplyStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileDivideStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileModuloStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileBitwiseOrStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileRshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileVarFunctionCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileReturnStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk);
	void compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaCodeChunk& chunk);

	void executeChunk(liaCodeChunk& chunk, liaVariable& retval,
		unsigned int funId, unsigned int& parmBytesRead,
		liaCodeChunk* callerChunk, unsigned int callerpos, std::vector<liaVariable>* callerEnv);

	void printCompilationStats();

public:

	int getGlobalVariables(liaEnvironment& gEnv);
	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList);
	void exeCuteProgram();

	liaVM();
	~liaVM();
};

#endif
