/* lia VM - 2o25 */

#include "vm.h"

liaVM::liaVM()
{
}

void liaVM::fatalError(std::string err)
{
	std::cout << "Error: " << err << std::endl;
	throw vmException();
}

void liaVM::getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos, unsigned int& bytesRead, liaVariable& retvar)
{
	if (chunk.code[pos] == liaOpcode::opConstant)
	{
		retvar = constantPool[chunk.code[pos + 1]];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
	{
		retvar = chunk.env[chunk.code[pos + 1]];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opArrayInitializer)
	{
		unsigned int br;
		unsigned char numElements = chunk.code[pos + 1];
		bytesRead = 2;
		for (unsigned int i = 0;i < numElements;i++)
		{
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br,element);
			retvar.vlist.push_back(element);
			bytesRead += br;
			pos += br;
		}

		retvar.type = liaVariableType::array;
	}
	else if (chunk.code[pos] == liaOpcode::opArraySubscript)
	{
		const unsigned char varId = chunk.code[pos + 1];
		const unsigned char numSubscripts = chunk.code[pos + 2];

		bytesRead = 3;
		liaVariable* pVar=&chunk.env[varId];
		for (unsigned int sub = 0;sub < numSubscripts;sub++)
		{
			unsigned int br = 0;
			// TODO: check bounds
			liaVariable asub;
			getExpressionFromCode(chunk, pos + bytesRead, br,asub);
			pVar = &pVar->vlist[std::get<int>(asub.value)];
			bytesRead += br;
		}

		retvar.type = pVar->type;
		retvar.value = pVar->value;

		if (pVar->type == liaVariableType::array)
		{
			retvar.vlist = pVar->vlist;
		}
	}
	else if (chunk.code[pos] == liaOpcode::opLibFunctionCall)
	{
		unsigned char funId = chunk.code[pos + 1];
		if (funId == StdFunctionId::FunctionGetMillisecondsSinceEpoch)
		{
			long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			retvar.value = now;
			bytesRead = 2;
		}
		else
		{
			fatalError("Unknown lib function called");
		}
	}
	else
	{
		fatalError("Unknown getExpressionFromCode");
	}
}

void liaVM::innerPrint(liaVariable& var)
{
	if (var.type == liaVariableType::array)
	{
		bool first = true;
		std::cout << "[";
		for (auto& el : var.vlist)
		{
			if (!first) std::cout << ",";
			innerPrint(el);
			first = false;
		}
		std::cout << "]";
	}
	else
	{
		std::visit([](const auto& x) { std::cout << x; }, var.value);
		//std::cout << " ";
	}
}

void liaVM::exeCuteProgram()
{
	liaCodeChunk chunk = chunks[0];
	unsigned int ip = 0;

	while (ip < chunk.code.size())
	{
		unsigned char opcode = chunk.code[ip];

		if (opcode == liaOpcode::opSetLocalVariable)
		{
			// set variable value to stack pop
			const unsigned char varId = chunk.code[ip + 1];
			unsigned int br;
			getExpressionFromCode(chunk, ip + 2, br, chunk.env[varId]);

			ip += 2+br;
		}
		else if (opcode == liaOpcode::opStdFunctionCall)
		{
			// library function call
			const unsigned char funId = chunk.code[ip + 1];
			if (funId == StdFunctionId::FunctionPrint)
			{
				unsigned char numArgz = chunk.code[ip + 2];
				unsigned int pos = ip + 3;

				unsigned int bytesRead = 0;
				for (int argNum = 0;argNum < numArgz;argNum++)
				{
					if (chunk.code[pos] == liaOpcode::opConstant)
					{
						unsigned char constId = chunk.code[pos + 1];
						innerPrint(constantPool[constId]);
						pos += 2;
						bytesRead += 2;
					}
					else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
					{
						unsigned char varId = chunk.code[pos + 1];
						innerPrint(chunk.env[varId]);
						pos += 2;
						bytesRead += 2;
					}
					else if (chunk.code[pos] == liaOpcode::opArraySubscript)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br,v);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
				}
				std::cout << std::endl;

				ip += 3 + bytesRead;
			}
		}
		else if (opcode == liaOpcode::opPostIncrement)
		{
			const unsigned char varId = chunk.code[ip + 1];

			liaVariableType incType = chunk.env[varId].type;
			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead,incAmount);

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(chunk.env[varId].value);
				int inc = std::get<int>(incAmount.value);
				chunk.env[varId].value = val+inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(chunk.env[varId].value);
				long long inc = std::get<long long>(incAmount.value);
				chunk.env[varId].value = val + inc;
			}
			else if (incType == liaVariableType::string)
			{
				std::string val = std::get<std::string>(chunk.env[varId].value);
				std::string inc = std::get<std::string>(incAmount.value);
				chunk.env[varId].value = val + inc;
			}

			ip += 2+bytesRead;
		}
		else if (opcode == liaOpcode::opPostDecrement)
		{
			const unsigned char varId = chunk.code[ip + 1];

			liaVariableType incType = chunk.env[varId].type;
			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, incAmount);

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(chunk.env[varId].value);
				int inc = std::get<int>(incAmount.value);
				chunk.env[varId].value = val - inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(chunk.env[varId].value);
				long long inc = std::get<long long>(incAmount.value);
				chunk.env[varId].value = val - inc;
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opJumpIfGreaterEqual)
		{
			unsigned short int addrToJump;
			addrToJump = chunk.code[ip + 1];
			addrToJump |= chunk.code[ip + 2] << 8;

			unsigned int br,br2;
			liaVariable lexpr;
			getExpressionFromCode(chunk, ip + 3, br,lexpr);
			liaVariable rexpr;
			getExpressionFromCode(chunk, ip + 3 + br, br2,rexpr);

			if (lexpr.value >= rexpr.value)
			{
				ip = addrToJump;
			}
			else
			{
				ip += 3 + br + br2;
			}
		}
		else if (opcode == liaOpcode::opJump)
		{
			unsigned short int addrToJumpTo;
			addrToJumpTo = chunk.code[ip + 1];
			addrToJumpTo |= chunk.code[ip + 2] << 8;

			ip = addrToJumpTo;
		}
		else if (opcode == liaOpcode::opSetArrayElement)
		{
			const unsigned char arrId = chunk.code[ip + 1];
			unsigned int numSubscripts = chunk.code[ip + 2];

			liaVariable* pVar = &chunk.env[arrId];
			unsigned int bytesRead = 0;
			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				// TODO: check bounds
				unsigned int br=0;
				liaVariable asub;
				getExpressionFromCode(chunk, ip + 3 + bytesRead, br,asub);
				pVar = &pVar->vlist[std::get<int>(asub.value)];
				bytesRead += br;
			}

			unsigned int br2 = 0;
			getExpressionFromCode(chunk, ip + 3 + bytesRead, br2,*pVar);

			ip += 3 + bytesRead+br2;
		}
		else
		{
			fatalError("Unknown opcode");
		}
	}
}

unsigned char liaVM::findOrAddConstantToConstantPool(liaVariable& constz)
{
	ptrdiff_t pos = std::distance(constantPool.begin(), std::find(constantPool.begin(), constantPool.end(), constz));
	if (pos >= (signed int)constantPool.size())
	{
		constantPool.push_back(constz);
		return (unsigned char)(constantPool.size() - 1);
	}
	else
	{
		return (unsigned char)pos;
	}
}

liaVariableType liaVM::compileExpression(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	if (theAst->iName == grammarElement::IntegerNumber)
	{
		// numeric constant - add it to or find it in the constant pool
		int constVal = theAst->iNumber;

		liaVariable theConst;
		theConst.type = liaVariableType::integer;
		theConst.value = constVal;
		unsigned char constantId = findOrAddConstantToConstantPool(theConst);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(constantId);
		
		return liaVariableType::integer;
	}
	else if (theAst->iName == grammarElement::StringLiteral)
	{
		std::string constVal = theAst->token_to_string();

		liaVariable theConst;
		theConst.type = liaVariableType::string;
		theConst.value = constVal;
		unsigned char constantId = findOrAddConstantToConstantPool(theConst);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(constantId);

		return liaVariableType::string;
	}
	else if (theAst->iName == grammarElement::ArrayInitializer)
	{
		chunk.code.push_back(liaOpcode::opArrayInitializer);

		if (theAst->nodes.size() == 0)
		{
			// empty array 
			chunk.code.push_back(0);
		}
		else
		{
			chunk.code.push_back((unsigned char)theAst->nodes[0]->nodes.size());
			if (theAst->nodes[0]->nodes.size() != 0)
			{
				assert(theAst->nodes[0]->iName == grammarElement::ExpressionList);

				for (auto& el : theAst->nodes[0]->nodes)
				{
					compileExpression(el, chunk);
				}
			}
		}

		return liaVariableType::array;
	}
	else if (theAst->iName == grammarElement::ArraySubscript)
	{
		//std::cout << peg::ast_to_s(theAst);

		chunk.code.push_back(liaOpcode::opArraySubscript);

		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;
		chunk.findVar(varName, varId);

		chunk.code.push_back(varId);

		unsigned int numSubscripts = (unsigned int)(theAst->nodes.size() - 1);
		chunk.code.push_back(numSubscripts);

		for (auto& sub : theAst->nodes)
		{
			if (sub->iName == grammarElement::Expression)
			{
				compileExpression(sub, chunk);
			}
		}
	}
	else if (theAst->iName == grammarElement::VariableName)
	{
		std::string varName = theAst->token_to_string();
		unsigned int varId;
		chunk.findVar(varName, varId);

		chunk.code.push_back(liaOpcode::opGetLocalVariable);
		chunk.code.push_back(varId);

		return chunk.env[varId].type;
	}
	else if (theAst->iName == grammarElement::RFuncCall)
	{
		//std::cout << peg::ast_to_s(theAst);

		if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionGetMillisecondsSinceEpoch)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionGetMillisecondsSinceEpoch);
			return liaVariableType::longint;
		}
		else
		{
			fatalError("Unsupported Rfunction call:" + theAst->nodes[0]->token_to_string());
		}
	}
	else if ((theAst->iName == grammarElement::InnerExpression) || (theAst->iName == grammarElement::Expression))
	{
		return compileExpression(theAst->nodes[0], chunk);
	}
	else
	{
		std::cout << "Error: unknown expression type:" << theAst->name << std::endl;
	}

	// "unreachable"
	return liaVariableType::integer;
}

void liaVM::compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName,varId))
	{
		liaVariable newVar;
		newVar.name = varName;
		chunk.env.push_back(newVar);
		varId = (unsigned int)(chunk.env.size() - 1);
	}

	assert(theAst->nodes[1]->iName == grammarElement::Expression);

	chunk.code.push_back(liaOpcode::opSetLocalVariable);
	chunk.code.push_back(varId);

	liaVariableType vartype=compileExpression(theAst->nodes[1], chunk);
	chunk.env[varId].type = vartype;
}

void liaVM::compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string funcName = theAst->nodes[0]->token_to_string();

	if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionPrint)
	{
		chunk.code.push_back(liaOpcode::opStdFunctionCall);
		chunk.code.push_back(StdFunctionId::FunctionPrint);
		
		// num. arguments
		unsigned char numArgs = (unsigned char)theAst->nodes[1]->nodes.size();
		chunk.code.push_back(numArgs);
		
		// args
		for (auto& arg : theAst->nodes[1]->nodes)
		{
			compileExpression(arg, chunk);
		}
	}


}

void liaVM::compileJumpTo(unsigned short int addr, liaCodeChunk& chunk)
{
	chunk.code.push_back((unsigned char)addr & 0xff);
	chunk.code.push_back((unsigned char)(addr>>8) & 0xff);
}

unsigned short int liaVM::compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::shared_ptr<peg::Ast> lExpr = theAst->nodes[0]->nodes[0];
	int relopid = theAst->nodes[0]->nodes[1]->tokenId;
	std::shared_ptr<peg::Ast> rExpr = theAst->nodes[0]->nodes[2];

	if (relopid == relopId::RelopLess)
	{
		chunk.code.push_back(liaOpcode::opJumpIfGreaterEqual);
	}

	unsigned short int jumpPtr = (unsigned short int)chunk.code.size();
	chunk.code.push_back(0); // space for jump address
	chunk.code.push_back(0);

	compileExpression(lExpr, chunk);
	compileExpression(rExpr, chunk);

	return jumpPtr;
}

void liaVM::compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::shared_ptr<peg::Ast> pCond = theAst->nodes[0];
	std::shared_ptr<peg::Ast> pCodeBlock = theAst->nodes[1];

	unsigned short int loopAddress = (unsigned short int)chunk.code.size();

	// if condition of while is false, we must skip the code block (to skipLoopAddress)
	unsigned short int exitLoopPtr = compileCondition(pCond, chunk);

	compileCodeBlock(pCodeBlock, chunk);

	chunk.code.push_back(liaOpcode::opJump);
	compileJumpTo(loopAddress, chunk);

	unsigned short int skipLoopAddress = (unsigned short int)chunk.code.size();
	chunk.code[exitLoopPtr] = skipLoopAddress & 0xff;
	chunk.code[exitLoopPtr+1] = (skipLoopAddress>>8) & 0xff;
}

void liaVM::compilePostIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to postincrement undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostIncrement);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compilePostDecrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to postdecrement undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostDecrement);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compileArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// TODO: non-existant array assignment
		fatalError("Array named " + varName + " not declared at line " + std::to_string(theAst->line));
	}

	assert(theAst->nodes[1]->iName == grammarElement::Expression);

	chunk.code.push_back(liaOpcode::opSetArrayElement);
	chunk.code.push_back(varId);

	unsigned int numSubscripts = (unsigned int)(theAst->nodes[0]->nodes.size() - 1);
	chunk.code.push_back(numSubscripts);

	for (unsigned int sub = 0;sub < numSubscripts;sub++)
	{
		compileExpression(theAst->nodes[0]->nodes[sub+1], chunk);
	}

	liaVariableType vartype = compileExpression(theAst->nodes[1], chunk);
	//chunk.env[varId].type = vartype;
}

void liaVM::compileCodeBlock(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	for (auto& stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;

		if (stmt->nodes[0]->iName == grammarElement::VarDeclStmt)
		{
			compileVarDeclStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::FuncCallStmt)
		{
			compileFuncCallStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::WhileStmt)
		{
			compileWhileStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::IncrementStmt)
		{
			compilePostIncrementStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::DecrementStmt)
		{
			compilePostDecrementStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::ArrayAssignmentStmt)
		{
			compileArrayAssignmentStatement(stmt->nodes[0], chunk);
		}
		else if ((stmt->nodes[0]->iName == grammarElement::SingleLineComment) || (stmt->nodes[0]->iName == grammarElement::MultiLineComment))
		{
			// ignore comments
		}
		else
		{
			fatalError("Unknown statement type " + stmt->nodes[0]->name);
		}
	}
}

int liaVM::compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList)
{
	for (auto& f : functionList)
	{
		if (f.second.name == "main")
		{
			liaCodeChunk chunk;
			compileCodeBlock(f.second.functionCodeBlockAst,chunk);
			chunks.push_back(chunk);
		}
	}

	return 0;
}

liaVM::~liaVM()
{
}
