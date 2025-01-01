/* lia VM - 2o25 */

#include "vm.h"

liaVM::liaVM()
{
}

liaVariable liaVM::getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos, unsigned int& bytesRead)
{
	liaVariable retvar;

	if (chunk.code[pos] == liaOpcode::opConstant)
	{
		const unsigned char constId = chunk.code[pos + 1];
		retvar = constantPool.pool[constId];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
	{
		const unsigned char varId = chunk.code[pos + 1];
		retvar = chunk.env[varId];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opArrayInitializer)
	{
		unsigned int br;
		unsigned char numElements = chunk.code[pos + 1];
		bytesRead = 2;
		for (unsigned int i = 0;i < numElements;i++)
		{
			liaVariable element = getExpressionFromCode(chunk, pos + 2, br);
			retvar.vlist.push_back(element);
			bytesRead += br;
			pos += br;
		}
	}

	return retvar;
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

		if (opcode == liaOpcode::opConstant)
		{
			// push the constant on the stack
			const unsigned char constId = chunk.code[ip + 1];

			vmstack.push(constantPool.pool[constId]);
			ip += 2;
		}
		else if (opcode == liaOpcode::opSetLocalVariable)
		{
			// set variable value to stack pop
			const unsigned char varId = chunk.code[ip + 1];

			unsigned int br;
			liaVariable val2set = getExpressionFromCode(chunk, ip + 2, br);
			chunk.env[varId].value = val2set.value;
			chunk.env[varId].vlist= val2set.vlist;

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
				for (int argNum = 0;argNum < numArgz;argNum++)
				{
					if (chunk.code[pos] == liaOpcode::opConstant)
					{
						unsigned char constId = chunk.code[pos + 1];
						liaVariable c = constantPool.pool[constId];
						if (c.type == liaVariableType::integer)
						{
							std::cout << std::get<int>(c.value);
						}
						else if (c.type == liaVariableType::string)
						{
							std::cout << std::get<std::string>(c.value);
						}
					}
					else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
					{
						unsigned char varId = chunk.code[pos + 1];
						liaVariable v = chunk.env[varId];
						if (v.type == liaVariableType::integer)
						{
							std::cout << std::get<int>(v.value);
						}
						else if (v.type == liaVariableType::string)
						{
							std::cout << std::get<std::string>(v.value);
						}
						else if (v.type == liaVariableType::array)
						{
							innerPrint(v);
						}
					}

					pos += 2;
				}
				std::cout << std::endl;

				ip += 3 + numArgz * 2;
			}

		}
		else if (opcode == liaOpcode::opPostIncrement)
		{
			const unsigned char varId = chunk.code[ip + 1];

			liaVariableType incType = chunk.env[varId].type;
			unsigned int bytesRead;
			liaVariable incAmount = getExpressionFromCode(chunk, ip + 2,bytesRead);

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(chunk.env[varId].value);
				int inc = std::get<int>(incAmount.value);
				chunk.env[varId].value = val+inc;
			}

			ip += 2+bytesRead;
		}
		else if (opcode == liaOpcode::opJumpIfGreaterEqual)
		{
			unsigned short int addrToJump;
			addrToJump = chunk.code[ip + 1];
			addrToJump |= chunk.code[ip + 2] << 8;

			unsigned int br,br2;
			liaVariable lexpr = getExpressionFromCode(chunk, ip + 3, br);
			liaVariable rexpr = getExpressionFromCode(chunk, ip + 3+br, br2);

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
			unsigned short int addrToJump;
			addrToJump = chunk.code[ip + 1];
			addrToJump |= chunk.code[ip + 2] << 8;

			ip = addrToJump;
		}
	}
}

unsigned char liaVM::findOrAddConstantToConstantPool(liaVariable& constz)
{
	ptrdiff_t pos = std::distance(constantPool.pool.begin(), std::find(constantPool.pool.begin(), constantPool.pool.end(), constz));
	if (pos >= (signed int)constantPool.pool.size())
	{
		constantPool.pool.push_back(constz);
		return (unsigned char)(constantPool.pool.size() - 1);
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
		chunk.code.push_back((unsigned char)theAst->nodes[0]->nodes.size());

		if (theAst->nodes[0]->nodes.size() != 0)
		{
			assert(theAst->nodes[0]->iName == grammarElement::ExpressionList);

			for (auto& el : theAst->nodes[0]->nodes)
			{
				compileExpression(el, chunk);
			}
		}

		return liaVariableType::array;
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
	else if ((theAst->iName == grammarElement::InnerExpression) || (theAst->iName == grammarElement::Expression))
	{
		return compileExpression(theAst->nodes[0], chunk);
	}

	// unreachable
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
		// TODO: fatal error
	}

	chunk.code.push_back(liaOpcode::opPostIncrement);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
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
