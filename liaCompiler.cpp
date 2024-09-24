
/* friol goes baremetal - 2k24 */

#include "compiler.h"

using namespace asmjit;

liaCompiler::liaCompiler()
{
	logger = new FileLogger(stdout);
	codeChunk.init(rt.environment(), rt.cpuFeatures());
	codeChunk.setLogger(logger);

	jitter = new x86::Compiler(&codeChunk);
}

void liaCompiler::generatePrintCode(const std::shared_ptr<peg::Ast>& theAst)
{
	if (theAst->name == "IntegerNumber")
	{
		std::string numVal;
		numVal += theAst->token;
		
		const char* scpPtr = constPoolMgr.getStringConstantPtr(numVal);

		InvokeNode* invokeNode;
		jitter->invoke(&invokeNode, imm(&puts),FuncSignature::build<int,const char*>());
		invokeNode->setArg(0, scpPtr);
		x86::Gp x = jitter->newUInt32();
		invokeNode->setRet(0, x);
	}
	else if (theAst->name == "StringLiteral")
	{
		std::string strVal;
		strVal += theAst->token;
		std::regex quote_re("\"");
		strVal = std::regex_replace(strVal, quote_re, "");

		const char* scpPtr = constPoolMgr.getStringConstantPtr(strVal);

		InvokeNode* invokeNode;
		jitter->invoke(&invokeNode, imm(&puts), FuncSignature::build<int, const char*>());
		invokeNode->setArg(0, scpPtr);
		x86::Gp x = jitter->newUInt32();
		invokeNode->setRet(0, x);
	}
	else
	{
		// unhandled (for now) print type

	}
}

void liaCompiler::compileFunctionCall(const std::shared_ptr<peg::Ast>& theAst)
{
	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "FuncName")
			{
				if (ch->token == "print")
				{
					// library function "print"
					generatePrintCode(theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0]);
				}
			}
		}
	}
}

void liaCompiler::compileVarAssignment(const std::shared_ptr<peg::Ast>& theAst)
{
	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				//std::cout << ch->token << std::endl;
				// ch->token is variable name
			}
		}
		else
		{
			if (ch->nodes[0]->nodes[0]->name == "IntegerNumber")
			{
				std::string numVal;
				numVal += ch->nodes[0]->nodes[0]->token;

				x86::Gp vReg = jitter->newGpd();
				jitter->mov(vReg,atoi(numVal.c_str()));

				//std::cout << numVal << std::endl;
			}
		}
	}
}

void liaCompiler::exeCute()
{
	auto err = jitter->finalize();
	if (err)
	{
		printf("finalize() failed: %s\b", DebugUtils::errorAsString(err));
		return;
	}

	std::cout << std::endl << "Starting program execution" << std::endl;

	typedef void (*programFunction)(void);
	programFunction fn;
	Error err1 = rt.add(&fn, &codeChunk);
	if (err1)
	{
		printf("AsmJit failed: %s\n", DebugUtils::errorAsString(err1));
		return;
	}

	fn();
	rt.release(fn);

	std::cout << "ExeCution completed." << std::endl;
}

int liaCompiler::compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params,std::vector<liaFunction>& functionList)
{
	// short recipe:
	// this function has to scan all the functions in the program, and create chunks of code for each one.
	// probably, sub-chunks will be needed, because an expression or a conditional expression
	// could be evaluated at runtime with code that is quite complex. So it will map to a chunk.
	// other problem: how to handle loops of code that repeatedly call an inner function (an inner chunk)
	// solution: this should be doable

	for (liaFunction f : functionList)
	{
		if (f.name == "main")
		{
			std::cout << peg::ast_to_s(f.functionCodeBlockAst);
			assert(f.functionCodeBlockAst->name == "CodeBlock");

			FuncNode* funcNode = jitter->addFunc(FuncSignature::build<void>());

			for (auto& stmt : f.functionCodeBlockAst->nodes)
			{
				//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;

				if (stmt->nodes[0]->name == "FuncCallStmt")
				{
					compileFunctionCall(stmt->nodes[0]);
				}
				else if (stmt->nodes[0]->name == "VarDeclStmt")
				{
					//std::cout << "vardecl" << std::endl;
					compileVarAssignment(stmt->nodes[0]);
				}
			}

			jitter->ret();
			jitter->endFunc();
		}
	}

	return 0;
}

liaCompiler::~liaCompiler()
{
	delete(jitter);
}
