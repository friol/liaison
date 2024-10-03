
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
		jitter->invoke(&invokeNode, imm(&fputs),FuncSignature::build<int,const char*, FILE*>());
		invokeNode->setArg(0, scpPtr);
		invokeNode->setArg(1, stdout);
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
		//jitter->invoke(&invokeNode, imm(&puts), FuncSignature::build<int, const char*>());
		jitter->invoke(&invokeNode, imm(&fputs), FuncSignature::build<int, const char*,FILE*>());
		invokeNode->setArg(0, scpPtr);
		invokeNode->setArg(1, stdout);
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
			tmpStr[0] = '\0';

			InvokeNode* itoaInvoker;
			jitter->invoke(&itoaInvoker, &itoa, FuncSignature::build<char*,int,char*,int>());
			itoaInvoker->setArg(0, pVar->vreg);
			itoaInvoker->setArg(1, tmpStr);
			itoaInvoker->setArg(2, 10);

			x86::Gp x = jitter->newUInt32();
			itoaInvoker->setRet(0,x);

			InvokeNode* invokeNode;
			jitter->invoke(&invokeNode, imm(&fputs), FuncSignature::build<int, const char*, FILE*>());
			invokeNode->setArg(0, tmpStr);
			invokeNode->setArg(1, stdout);
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

void liaCompiler::generatePrintNewline()
{
	InvokeNode* invokeNode;
	jitter->invoke(&invokeNode, imm(&puts), FuncSignature::build<int, const char*>());
	invokeNode->setArg(0, "");
	x86::Gp x = jitter->newUInt32();
	invokeNode->setRet(0, x);
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
					for (auto& ch : theAst->nodes[1]->nodes)
					{
						generatePrintCode(ch->nodes[0]->nodes[0], env);
					}
					generatePrintNewline();
				}
				else
				{
					std::string funName;
					funName += ch->token;

					//std::cout << "other function invoked: " << funName << std::endl;

					bool found = false;
					unsigned int idx = 0;
					while ((!found) && (idx < listOfCompiledFunctions.size()))
					{
						if (listOfCompiledFunctions[idx].name == funName) found = true;
						else idx += 1;
					}

					if (!found)
					{
						std::cout << "Error: function call to unknown function " << funName << std::endl;
						throw("Error");
					}

					InvokeNode* invokeNode;
					jitter->invoke(&invokeNode, listOfCompiledFunctions[idx].nodePtr->label(), FuncSignature::build<void>());
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

void liaCompiler::compilePostincrementStmt(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env, int inc)
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
				if (inc>0) jitter->add(pVar->vreg, atoi(numVal.c_str()));
				else jitter->sub(pVar->vreg, atoi(numVal.c_str()));
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

void liaCompiler::compileMultiplyDivideStmt(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env, bool isMultiply)
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

				liaCompilerVariable* pVar = addvarOrUpdateEnvironment(&v, env);

				int coeff = atoi(numVal.c_str());
				x86::Gp tmpReg = jitter->newGpd();
				jitter->mov(tmpReg, coeff);

				if (isMultiply) jitter->imul(pVar->vreg, tmpReg);
				else
				{
					x86::Gp dummy = jitter->newGpd();
					jitter->xor_(dummy, dummy);
					jitter->idiv(dummy,pVar->vreg, tmpReg);
				}

				int ival = std::get<int>(pVar->value);
				if (isMultiply) ival *= atoi(numVal.c_str());
				else ival /= atoi(numVal.c_str());
				pVar->value = ival;
			}
			else
			{
				std::cout << "unhandled post-operator operand type " << ch->nodes[0]->nodes[0]->name << std::endl;
			}
		}
	}
}

void liaCompiler::compileShiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env, int verse)
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

				liaCompilerVariable* pVar = addvarOrUpdateEnvironment(&v, env);

				int coeff = atoi(numVal.c_str());
				x86::Gp tmpReg = jitter->newGpd();
				jitter->mov(tmpReg, coeff);

				if (verse>0) jitter->shr(pVar->vreg, tmpReg);
				else jitter->shl(pVar->vreg, tmpReg);

				int ival = std::get<int>(pVar->value);
				if (verse > 0) ival >>= atoi(numVal.c_str());
				else ival <<= atoi(numVal.c_str());
				pVar->value = ival;
			}
			else
			{
				std::cout << "unhandled shift post-operator operand type " << ch->nodes[0]->nodes[0]->name << std::endl;
			}
		}
	}
}

void liaCompiler::compileSimpleCondition(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env, Label& loopLabel, bool invert)
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

	if ((relOpStr == "<")&&(!invert)) jitter->jl(loopLabel);
	else if ((relOpStr == "<") && invert) jitter->jge(loopLabel);
	else if ((relOpStr == ">") && (!invert)) jitter->jg(loopLabel);
	else if ((relOpStr == ">") && invert) jitter->jle(loopLabel);
	else
	{
		std::cout << "condition::Unsupported relop" << std::endl;
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
	compileSimpleCondition(pCond, env,L_condLoop, false);
}

void liaCompiler::compileIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaCompilerEnvironment* env)
{
	liaVariable retVar;

	std::shared_ptr<peg::Ast> pCond;
	std::shared_ptr<peg::Ast> pBlock;
	std::shared_ptr<peg::Ast> pBlock2;

	pCond = theAst->nodes[0];
	pBlock = theAst->nodes[1];

	if (theAst->nodes.size() == 2)
	{
		// simple if

		Label L_loop = jitter->newLabel();
		compileSimpleCondition(pCond, env, L_loop, true);
		compileCodeBlock(pBlock, env);
		jitter->bind(L_loop);
	}
	else if (theAst->nodes.size() == 3)
	{
		// if/else
		pBlock2 = theAst->nodes[2];

		Label L_loop = jitter->newLabel();
		Label L_elseloop = jitter->newLabel();

		compileSimpleCondition(pCond, env, L_loop, true);
		compileCodeBlock(pBlock, env);
		jitter->jmp(L_elseloop);
		jitter->bind(L_loop);
		compileCodeBlock(pBlock2, env);
		jitter->bind(L_elseloop);
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
		else if ((stmt->nodes[0]->name == "IncrementStmt")|| (stmt->nodes[0]->name == "DecrementStmt"))
		{
			int inc = 1;
			if (stmt->nodes[0]->name == "DecrementStmt") inc = -1;
			compilePostincrementStmt(stmt->nodes[0], env, inc);
		}
		else if (stmt->nodes[0]->name == "WhileStmt")
		{
			compileWhileStmt(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "IfStmt")
		{
			compileIfStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "MultiplyStmt")
		{
			compileMultiplyDivideStmt(stmt->nodes[0], env, true);
		}
		else if (stmt->nodes[0]->name == "DivideStmt")
		{
			compileMultiplyDivideStmt(stmt->nodes[0], env, false);
		}
		else if (stmt->nodes[0]->name == "RshiftStmt")
		{
			compileShiftStatement(stmt->nodes[0], env, 1);
		}
		else if (stmt->nodes[0]->name == "LshiftStmt")
		{
			compileShiftStatement(stmt->nodes[0], env, -1);
		}
	}
}

int liaCompiler::compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params,std::vector<liaFunction>& functionList)
{
	// first pass: create a list of functions (to be called - or not called - later)

	for (liaFunction f : functionList)
	{
		//if (f.name != "main")
		{
			std::cout << "First pass: adding function " << f.name << std::endl;
			
			liaCompiledFunction newPhun;
			newPhun.name = f.name;
			newPhun.nodePtr = jitter->newFunc(FuncSignature::build<void>());
			//jitter->newFuncNode(&newPhun.nodePtr, FuncSignature::build<void>());

			listOfCompiledFunctions.push_back(newPhun);
		}
	}

	// second pass: create main caller + chunks of code for main and other functions

	bool found = false;
	unsigned int idx = 0;
	while ((!found) && (idx < listOfCompiledFunctions.size()))
	{
		if (listOfCompiledFunctions[idx].name == "main") found = true;
		else idx += 1;
	}

	FuncNode* funcNode = jitter->addFunc(FuncSignature::build<void>());
	InvokeNode* invokeNode;
	jitter->invoke(&invokeNode, listOfCompiledFunctions[idx].nodePtr->label(), FuncSignature::build<void>());
	jitter->endFunc();

	//

	liaCompilerEnvironment compEnv;
	for (liaFunction f : functionList)
	{
		std::cout << "compiling code for function " << f.name << std::endl;

		bool found = false;
		unsigned int idx = 0;
		while ((!found)&&(idx<listOfCompiledFunctions.size()))
		{
			if (listOfCompiledFunctions[idx].name == f.name) found = true;
			else idx += 1;
		}

		if (!found) throw("Error: unknown function name");

		jitter->addFunc(listOfCompiledFunctions[idx].nodePtr);

		compileCodeBlock(f.functionCodeBlockAst, &compEnv);

		//jitter->ret();
		jitter->endFunc();
	}

	return 0;
}

liaCompiler::~liaCompiler()
{
	delete(jitter);
	delete(logger);
}
