
#ifndef COMPILER_H
#define COMPILER_H

#define ASMJIT_STATIC

#include "include/peglib.h"
#include "asmjit/asmjit.h"
#include "interpreter.h"

using namespace asmjit;

//
//
//

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

//
//
//

struct liaCompilerVariable
{
	std::string name;
	liaVariableType type;
	std::variant<bool, int, long long, std::string> value;
	x86::Gp vreg;
};

struct liaCompilerEnvironment
{
	std::unordered_map<std::string, liaCompilerVariable> varMap;
};

//
//
//

class liaCompiler
{
private:

	JitRuntime rt;
	CodeHolder codeChunk;
	Section* dataSection;
	x86::Compiler* jitter;
	FileLogger* logger;

	constantPoolManager constPoolMgr;

	void generatePrintCode(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env);
	void compileFunctionCall(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env);
	void compileVarAssignment(const std::shared_ptr<peg::Ast>& theAst,liaCompilerEnvironment* env);
	void compilePostincrementStmt(const std::shared_ptr<peg::Ast>& theAst,liaCompilerEnvironment* env);
	void compileWhileStmt(const std::shared_ptr<peg::Ast>& theAst,liaCompilerEnvironment* env);

	liaCompilerVariable* addvarOrUpdateEnvironment(liaCompilerVariable* v, liaCompilerEnvironment* env);

	void compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env);
	void compileSimpleCondition(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env,Label& loopLabel);

public:

	int compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::vector<liaFunction>& functionList);
	void exeCute();

	liaCompiler();
	~liaCompiler();
};

#endif
