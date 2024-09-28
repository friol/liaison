
/* friol goes baremetal - 2k24 */

#pragma warning(disable : 4996)

#include "compiler.h"

using namespace asmjit;

liaCompiler::liaCompiler()
{
	logger = new FileLogger(stdout);
	codeChunk.init(rt.environment(), rt.cpuFeatures());
	codeChunk.setLogger(logger);

	jitter = new x86::Compiler(&codeChunk);
}

liaCompilerVariable* liaCompiler::addvarOrUpdateEnvironment(liaCompilerVariable* v, liaCompilerEnvironment* env)
{
	liaCompilerVariable* pVar = nullptr;

	if (env->varMap.find(v->name) != env->varMap.end())
	{
		pVar = &env->varMap[v->name];
		return pVar;
	}
	else
	{
		// else, emit code to assign value to vreg and add it to the env

		v->vreg = jitter->newGpd();
		if (v->type == liaVariableType::integer)
		{
			jitter->mov(v->vreg, std::get<int>(v->value));
		}

		env->varMap[v->name] = *v;
		return &env->varMap[v->name];
	}
}

void liaCompiler::generatePrintCode(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
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
	else if (theAst->name == "VariableName")
	{
		std::string vName;
		vName += theAst->token;
		liaCompilerVariable thevar;
		thevar.name = vName;
		liaCompilerVariable* pVar = addvarOrUpdateEnvironment(&thevar, env);
		//std::cout << std::get<int>(pVar->value) << std::endl;

		if (pVar->type == liaVariableType::integer)
		{
			char tmpStr[16]; // is this persistent to hold the value?

			InvokeNode* itoaInvoker;
			jitter->invoke(&itoaInvoker, &itoa, FuncSignature::build<char*,int,char*,int>());
			itoaInvoker->setArg(0, pVar->vreg);
			itoaInvoker->setArg(1, tmpStr);
			itoaInvoker->setArg(2, 10);

			x86::Gp x = jitter->newUInt32();
			itoaInvoker->setRet(0,x);

			InvokeNode* invokeNode;
			jitter->invoke(&invokeNode, imm(&puts), FuncSignature::build<int, const char*>());
			invokeNode->setArg(0, tmpStr);
			x86::Gp rv = jitter->newUInt32();
			invokeNode->setRet(0, rv);
		}
		else
		{
			// other variable type
		}
	}
	else
	{
		// unhandled (for now) print type

	}
}

void liaCompiler::compileFunctionCall(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
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
					generatePrintCode(theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0],env);
				}
			}
		}
	}
}

void liaCompiler::compileVarAssignment(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
{
	std::string variableName;

	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				//std::cout << "variable name:" << ch->token << std::endl;
				// ch->token is variable name
				variableName = ch->token;
			}
		}
		else
		{
			if (ch->nodes[0]->nodes[0]->name == "IntegerNumber")
			{
				std::string numVal;
				numVal += ch->nodes[0]->nodes[0]->token;
				//std::cout << "variable value: " << numVal << std::endl;

				liaCompilerVariable v;
				v.name = variableName;
				v.type = liaVariableType::integer;
				v.value = atoi(numVal.c_str());

				addvarOrUpdateEnvironment(&v,env);
			}
			else
			{
				// unhandled variable assignment
			}
		}
	}
}

void liaCompiler::compilePostincrementStmt(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
{
	std::string variableName;

	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				variableName = ch->token;
			}
		}
		else
		{
			if (ch->nodes[0]->nodes[0]->name == "IntegerNumber")
			{
				std::string numVal;
				numVal += ch->nodes[0]->nodes[0]->token;

				liaCompilerVariable v;
				v.name = variableName;
				v.type = liaVariableType::integer;

				liaCompilerVariable* pVar=addvarOrUpdateEnvironment(&v, env);
				jitter->add(pVar->vreg, atoi(numVal.c_str()));
				int ival = std::get<int>(pVar->value);
				ival+= atoi(numVal.c_str());
				pVar->value = ival;
			}
			else
			{
				// unhandled postincrement type
			}
		}
	}
}

void liaCompiler::compileSimpleCondition(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env, Label& loopLabel)
{
	// compiles expression relop expression
	// it should optimize out constant conditions, like 1<2 or 0==0 and such
	// (but in another universe)

	auto lexpr = theAst->nodes[0]->nodes[0]->nodes[0]->nodes[0];
	auto relop = theAst->nodes[0]->nodes[1];
	auto rexpr = theAst->nodes[0]->nodes[2]->nodes[0]->nodes[0];

	std::string varname;
	std::string relOpStr;
	relOpStr+=relop->token;
	int ival;

	if (lexpr->name == "VariableName")
	{
		varname = lexpr->token;
	}

	if (rexpr->name == "IntegerNumber")
	{
		std::string stoken;
		stoken += rexpr->token;
		ival = atoi(stoken.c_str());
	}

	liaCompilerVariable thevar;
	thevar.name = varname;
	liaCompilerVariable* pVar = addvarOrUpdateEnvironment(&thevar, env);

	jitter->cmp(pVar->vreg, ival);

	if (relOpStr == "<")
	{
		jitter->jl(loopLabel);
	}
}

void liaCompiler::compileWhileStmt(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
{
	std::shared_ptr<peg::Ast> pCond;
	std::shared_ptr<peg::Ast> pBlock;

	for (auto& ch : theAst->nodes)
	{
		if (ch->name == "Condition")
		{
			pCond = ch;
		}
		else if (ch->name == "CodeBlock")
		{
			pBlock = ch;
		}
	}

	Label L_loop = jitter->newLabel();
	Label L_condLoop = jitter->newLabel();

	jitter->jmp(L_loop);
	jitter->bind(L_condLoop);
	compileCodeBlock(pBlock, env);
	jitter->bind(L_loop);
	compileSimpleCondition(pCond, env,L_condLoop);
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

void liaCompiler::compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
{
	for (auto& stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;

		if (stmt->nodes[0]->name == "FuncCallStmt")
		{
			compileFunctionCall(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "VarDeclStmt")
		{
			//std::cout << "vardecl" << std::endl;
			compileVarAssignment(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "IncrementStmt")
		{
			compilePostincrementStmt(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "WhileStmt")
		{
			compileWhileStmt(stmt->nodes[0], env);
		}
	}
}

int liaCompiler::compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params,std::vector<liaFunction>& functionList)
{
	// short recipe:
	// this function has to scan all the functions in the program, and create chunks of code for each one.
	// probably, sub-chunks will be needed, because an expression or a conditional expression
	// could be evaluated at runtime with code that is quite complex. So it will map to a chunk.
	// other problem: how to handle loops of code that repeatedly call an inner function (an inner chunk)
	// solution: this should be doable defining a function and calling it with asmjit's invoker

	liaCompilerEnvironment compEnv;

	for (liaFunction f : functionList)
	{
		if (f.name == "main")
		{
			std::cout << peg::ast_to_s(f.functionCodeBlockAst);
			assert(f.functionCodeBlockAst->name == "CodeBlock");

			FuncNode* funcNode = jitter->addFunc(FuncSignature::build<void>());

			compileCodeBlock(f.functionCodeBlockAst, &compEnv);

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
