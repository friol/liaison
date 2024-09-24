
#ifndef COMPILER_H
#define COMPILER_H

#define ASMJIT_STATIC

#include "include/peglib.h"
#include "asmjit/asmjit.h"
#include "interpreter.h"

using namespace asmjit;

#define CONSTANT_POOL_SIZE 32

class constantPoolManager
{
private:

	unsigned int stringCpIdx = 0;
	std::string stringConstantPool[CONSTANT_POOL_SIZE];

public:

	constantPoolManager() { };
	~constantPoolManager() { };

	const char* getStringConstantPtr(std::string& s)
	{
		unsigned int idx = 0;
		bool found = false;
		while ((!found) && (idx < CONSTANT_POOL_SIZE))
		{
			if (stringConstantPool[idx] == s)
			{
				found = true;
				return stringConstantPool[idx].c_str();
			}

			idx += 1;
		}

		if (!found)
		{
			// TODO: check bounds!!!
			stringConstantPool[stringCpIdx] = s;
			stringCpIdx += 1;
			return stringConstantPool[stringCpIdx - 1].c_str();
		}

		return NULL; // unreacheable code (I hope)
	}
};

class liaCompiler
{
private:

	JitRuntime rt;
	CodeHolder codeChunk;
	Section* dataSection;
	x86::Compiler* jitter;
	FileLogger* logger;

	constantPoolManager constPoolMgr;

	void compileFunctionCall(const std::shared_ptr<peg::Ast>& theAst);
	void generatePrintCode(const std::shared_ptr<peg::Ast>& theAst);
	void compileVarAssignment(const std::shared_ptr<peg::Ast>& theAst);

public:

	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::vector<liaFunction>& functionList);
	void exeCute();

	liaCompiler();
	~liaCompiler();
};

#endif
