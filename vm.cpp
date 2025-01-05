/* lia VM - 2o25 */

#include "vm.h"

liaVM::liaVM()
{
}

//
// exeCute
//

void liaVM::fatalError(std::string err)
{
	std::cout << "Error: " << err << std::endl;
	throw vmException();
}

void liaVM::getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos, unsigned int& bytesRead, liaVariable& retvar,std::vector<liaVariable>& env)
{
	if (chunk.code[pos] == liaOpcode::opConstant)
	{
		retvar = constantPool[chunk.code[pos + 1]];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
	{
		retvar = env[chunk.code[pos + 1]];
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opArrayInitializer)
	{
		unsigned int br;
		unsigned int numElements = chunk.code[pos + 1];
		bytesRead = 2;
		retvar.vlist.clear();
		for (unsigned int i = 0;i < numElements;i++)
		{
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, element,env);
			retvar.vlist.push_back(element);
			bytesRead += br;
			pos += br;
		}

		retvar.type = liaVariableType::array;
	}
	else if (chunk.code[pos] == liaOpcode::opArraySubscript)
	{
		bytesRead = 3;
		liaVariable* pVar = &env[chunk.code[pos + 1]];
		for (unsigned int sub = 0;sub < chunk.code[pos + 2];sub++)
		{
			unsigned int br = 0;
			liaVariable asub;
			getExpressionFromCode(chunk, pos + bytesRead, br, asub,env);

			int idx = std::get<int>(asub.value);
			if (pVar->type == liaVariableType::array)
			{
				if (idx >= pVar->vlist.size())
				{
					fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
				}
				pVar = &pVar->vlist[idx];
				if (sub == (chunk.code[pos + 2] - 1))
				{
					retvar.type = pVar->type;
					retvar.value= pVar->value;
					retvar.vlist = pVar->vlist;
				}
			}
			else if (pVar->type == liaVariableType::string)
			{
				if (idx >= std::get<std::string>(pVar->value).size())
				{
					fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
				}
				retvar.type = liaVariableType::string;
				char c = std::get<std::string>(pVar->value).at(idx);
				std::string sc;
				sc += c;
				retvar.value = sc;
			}
			bytesRead += br;
		}
	}
	else if (chunk.code[pos] == liaOpcode::opLibFunctionCall)
	{
		unsigned int funId = chunk.code[pos + 1];
		if (funId == StdFunctionId::FunctionGetMillisecondsSinceEpoch)
		{
			long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			retvar.value = now;
			bytesRead = 2;
		}
		else if (funId == StdFunctionId::FunctionToInteger)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, element,env);
			retvar.type = liaVariableType::integer;
			retvar.value = element.value;
			bytesRead = br+2;
		}
		else if (funId == StdFunctionId::FunctionReadTextFileLineByLine)
		{
			unsigned int br = 0;
			liaVariable fname;
			getExpressionFromCode(chunk, pos + 2, br, fname,env);
			retvar.type = liaVariableType::array;

			std::string line;
			std::ifstream file(std::get<std::string>(fname.value));
			if (file.is_open())
			{
				while (std::getline(file, line))
				{
					liaVariable l;
					l.type = liaVariableType::string;
					l.value = line;
					retvar.vlist.push_back(l);
				}
				file.close();
			}
			else
			{
				std::string err;
				err += "Could not open file [" + std::get<std::string>(fname.value) + ".Terminating.";
				fatalError(err);
			}

			bytesRead= br + 2;
		}
		else
		{
			fatalError("Unknown lib function called");
		}
	}
	else if (chunk.code[pos] == liaOpcode::opVarFunctionCall)
	{
		unsigned int varId = chunk.code[pos + 1];
		unsigned int funId = chunk.code[pos + 2];

		if (funId == libMethodId::MethodSplit)
		{
			retvar.type = liaVariableType::array;

			unsigned int br = 0;
			liaVariable deLim;
			getExpressionFromCode(chunk, pos + 3, br, deLim,env);

			std::string string2split = std::get<std::string>(env[varId].value);
			std::string delimiter = std::get<std::string>(deLim.value);

			size_t pos = 0;
			std::string token;
			while ((pos = string2split.find(delimiter)) != std::string::npos)
			{
				token = string2split.substr(0, pos);
				liaVariable varEl;
				varEl.type = liaVariableType::string;
				varEl.value = token;
				retvar.vlist.push_back(varEl);
				string2split.erase(0, pos + delimiter.length());
			}

			token = string2split.substr(0, pos);
			liaVariable varEl;
			varEl.type = liaVariableType::string;
			varEl.value = token;
			retvar.vlist.push_back(varEl);

			bytesRead = 3 + br;
		}
		else
		{
			fatalError("Unknown lib method called");
		}
	}
	else if (chunk.code[pos] == liaOpcode::opGetObjectLength)
	{
		retvar.type = liaVariableType::integer;
		liaVariable obj = env[chunk.code[pos + 1]];

		if (obj.type == liaVariableType::array)
		{
			retvar.value = (int)obj.vlist.size();
		}
		else if (obj.type == liaVariableType::string)
		{
			retvar.value = (int)std::get<std::string>(obj.value).size();
		}
		else
		{
			fatalError("Asking length of object with unsupported type.");
		}

		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opFunctionCall)
	{
		unsigned int funId = chunk.code[pos + 1];
		unsigned int br = 0;
		unsigned int totBr = 0;
		std::vector<liaVariable> parmz;
		for (auto& parm : funList[funId].parameters)
		{
			liaVariable theParm;
			getExpressionFromCode(chunk, pos + 2 + totBr, br, theParm, env);
			theParm.name = parm.name;
			parmz.push_back(theParm);
			totBr += br;
		}

		//std::vector<liaVariable> savedEnv;
		//for (auto& v : chunks[funId].basicEnv)
		//{
		//	savedEnv.push_back(v);
		//}

		executeChunk(chunks[funId], retvar,parmz);

		//retvar.type = returnValue.type;
		//retvar.value = returnValue.value;

		//chunks[funId].basicEnv.clear();
		//for (auto& v : savedEnv)
		//{
		//	chunks[funId].basicEnv.push_back(v);
		//}

		bytesRead = 2 + totBr;
	}
	else
	{
		fatalError("Unknown getExpressionFromCode:"+std::to_string(chunk.code[pos]));
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
	else if (var.type == liaVariableType::boolean)
	{
		if (std::get<bool>(var.value) == true) std::cout << "true";
		else std::cout << "false";
	}
	else
	{
		std::visit([](const auto& x) { std::cout << x; }, var.value);
		//std::cout << " ";
	}
}

void liaVM::executeChunk(liaCodeChunk& chunk, liaVariable& retval, std::vector<liaVariable>& params)
{
	unsigned int ip = 0;
	std::vector<liaVariable> runtimeEnv;

	// init runtime env
	unsigned int idx = 0;
	for (liaVariable& v : chunk.basicEnv)
	{
		runtimeEnv.push_back(v);
		if (idx < params.size())
		{
			runtimeEnv[idx] = params[idx];
		}
		idx += 1;
	}

	while (ip < chunk.code.size())
	{
		unsigned int opcode = chunk.code[ip];

		if (opcode == liaOpcode::opSetLocalVariable)
		{
			// set variable value to stack pop
			const unsigned int varId = chunk.code[ip + 1];
			unsigned int br;
			getExpressionFromCode(chunk, ip + 2, br, runtimeEnv[varId], runtimeEnv);

			ip += 2 + br;
		}
		else if (opcode == liaOpcode::opSetArrayElement)
		{
			const unsigned int arrId = chunk.code[ip + 1];
			const unsigned int numSubscripts = chunk.code[ip + 2];

			liaVariable* pVar = &runtimeEnv[arrId];
			unsigned int bytesRead = 0;
			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				// TODO: check bounds
				unsigned int br = 0;
				liaVariable asub;
				getExpressionFromCode(chunk, ip + 3 + bytesRead, br, asub, runtimeEnv);
				pVar = &pVar->vlist[std::get<int>(asub.value)];
				bytesRead += br;
			}

			unsigned int br2 = 0;
			getExpressionFromCode(chunk, ip + 3 + bytesRead, br2, *pVar, runtimeEnv);

			ip += 3 + bytesRead + br2;
		}
		else if (opcode == liaOpcode::opPostIncrement)
		{
			const unsigned int varId = chunk.code[ip + 1];

			//liaVariableType incType = runtimeEnv[varId].type;
			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, incAmount, runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(runtimeEnv[varId].value);
				int inc = std::get<int>(incAmount.value);
				runtimeEnv[varId].value = val + inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(runtimeEnv[varId].value);
				long long inc = std::get<long long>(incAmount.value);
				runtimeEnv[varId].value = val + inc;
			}
			else if (incType == liaVariableType::string)
			{
				std::string val = std::get<std::string>(runtimeEnv[varId].value);
				std::string inc = std::get<std::string>(incAmount.value);
				runtimeEnv[varId].value = val + inc;
			}
			else
			{
				fatalError("Unhandled postdecrement type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opPostDecrement)
		{
			const unsigned int varId = chunk.code[ip + 1];

			//liaVariableType incType = runtimeEnv[varId].type;
			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, incAmount, runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(runtimeEnv[varId].value);
				int inc = std::get<int>(incAmount.value);
				runtimeEnv[varId].value = val - inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(runtimeEnv[varId].value);
				long long inc = std::get<long long>(incAmount.value);
				runtimeEnv[varId].value = val - inc;
			}
			else
			{
				fatalError("Unhandled postdecrement type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opJumpIfGreaterEqual)
		{
			unsigned int br, br2;
			liaVariable lexpr;
			getExpressionFromCode(chunk, ip + 2, br, lexpr, runtimeEnv);
			liaVariable rexpr;
			getExpressionFromCode(chunk, ip + 2 + br, br2, rexpr, runtimeEnv);

			if (lexpr.value >= rexpr.value)
			{
				ip = chunk.code[ip + 1];
			}
			else
			{
				ip += 2 + br + br2;
			}
		}
		else if (opcode == liaOpcode::opJumpIfNotEqual)
		{
			unsigned int br, br2;
			liaVariable lexpr;
			getExpressionFromCode(chunk, ip + 2, br, lexpr, runtimeEnv);
			liaVariable rexpr;
			getExpressionFromCode(chunk, ip + 2 + br, br2, rexpr, runtimeEnv);

			if (lexpr.value != rexpr.value)
			{
				ip = chunk.code[ip + 1];
			}
			else
			{
				ip += 2 + br + br2;
			}
		}
		else if (opcode == liaOpcode::opJump)
		{
			ip = chunk.code[ip + 1];
		}
		else if (opcode == liaOpcode::opVarFunctionCall)
		{
			unsigned int varId = chunk.code[ip + 1];
			unsigned int funId = chunk.code[ip + 2];

			unsigned int bytesRead = 0;
			if (funId == libMethodId::MethodAdd)
			{
				liaVariable* pVar = &runtimeEnv[varId];
				unsigned int br = 0;
				liaVariable itemToAdd;
				getExpressionFromCode(chunk, ip + 3, br, itemToAdd, runtimeEnv);
				pVar->vlist.push_back(itemToAdd);
				bytesRead += br;
			}
			else
			{
				fatalError("Unknown method called");
			}

			ip += 3 + bytesRead;
		}
		else if (opcode == liaOpcode::opFunctionCall)
		{
			unsigned int funId = chunk.code[ip + 1];
			unsigned int bytesRead = 0;
			unsigned int br = 0;
			std::vector<liaVariable> parmz;
			for (auto& parm : funList[funId].parameters)
			{
				liaVariable theParm;
				getExpressionFromCode(chunk, ip + 2+ bytesRead,br, theParm, runtimeEnv);
				theParm.name = parm.name;
				parmz.push_back(theParm);
				bytesRead += br;
			}

			//std::vector<liaVariable> savedEnv;
			//for (auto& v : chunks[funId].basicEnv)
			//{
			//	savedEnv.push_back(v);
			//}

			liaVariable retVal;
			executeChunk(chunks[funId], retVal,parmz);

			//chunks[funId].basicEnv.clear();
			//for (auto& v : savedEnv)
			//{
			//	chunks[funId].basicEnv.push_back(v);
			//}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opReturn)
		{
			unsigned int numParamz = chunk.code[ip + 1];
			if (numParamz == 0)
			{
				return;
			}
			else
			{
				unsigned int br2 = 0;
				getExpressionFromCode(chunk, ip + 2, br2, retval, runtimeEnv);
				return;
			}
		}
		else if (opcode == liaOpcode::opStdFunctionCall)
		{
			// library function call
			const unsigned int funId = chunk.code[ip + 1];
			if (funId == StdFunctionId::FunctionPrint)
			{
				unsigned int numArgz = chunk.code[ip + 2];
				unsigned int pos = ip + 3;

				unsigned int bytesRead = 0;
				for (unsigned int argNum = 0;argNum < numArgz;argNum++)
				{
					if (chunk.code[pos] == liaOpcode::opConstant)
					{
						unsigned int constId = chunk.code[pos + 1];
						innerPrint(constantPool[constId]);
						pos += 2;
						bytesRead += 2;
					}
					else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
					{
						unsigned int varId = chunk.code[pos + 1];
						innerPrint(runtimeEnv[varId]);
						pos += 2;
						bytesRead += 2;
					}
					else if (chunk.code[pos] == liaOpcode::opArraySubscript)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, v, runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opGetObjectLength)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, v, runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opLibFunctionCall)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, v, runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else
					{
						fatalError("Unhandled parameter passed to print [" + std::to_string(chunk.code[pos]) + "]");
					}
				}
				std::cout << std::endl;

				ip += 3 + bytesRead;
			}
		}
		else
		{
			fatalError("Runtime error: Unknown opcode [" + std::to_string(opcode) + "]");
		}
	}
}

void liaVM::exeCuteProgram()
{
	unsigned int funId = 0;
	for (auto& fun : funList)
	{
		if (fun.name == "main")
		{
			std::vector<liaVariable> dummyParms; // TODO: pass params to main
			liaVariable dummyVal;
			executeChunk(chunks[funId],dummyVal,dummyParms);
		}

		funId += 1;
	}
}

// 
// compile
//

unsigned int liaVM::findOrAddConstantToConstantPool(liaVariable& constz)
{
	ptrdiff_t pos = std::distance(constantPool.begin(), std::find(constantPool.begin(), constantPool.end(), constz));
	if (pos >= (signed int)constantPool.size())
	{
		constantPool.push_back(constz);
		return (unsigned int)(constantPool.size() - 1);
	}
	else
	{
		return (unsigned int)pos;
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
		unsigned int constantId = findOrAddConstantToConstantPool(theConst);

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
		unsigned int constantId = findOrAddConstantToConstantPool(theConst);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(constantId);

		return liaVariableType::string;
	}
	else if (theAst->iName == grammarElement::BooleanConst)
	{
		//std::cout << peg::ast_to_s(theAst);

		bool constVal = (theAst->token_to_string() == "true") ? true : false;

		liaVariable theConst;
		theConst.type = liaVariableType::boolean;
		theConst.value = constVal;
		unsigned int constantId = findOrAddConstantToConstantPool(theConst);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(constantId);

		return liaVariableType::boolean;
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
			chunk.code.push_back((unsigned int)theAst->nodes[0]->nodes.size());
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

		return chunk.basicEnv[varId].type;
	}
	else if (theAst->iName == grammarElement::VariableWithProperty)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;
		chunk.findVar(varName, varId);

		std::string prop = theAst->nodes[1]->token_to_string();

		if (prop == "length")
		{
			chunk.code.push_back(liaOpcode::opGetObjectLength);
			chunk.code.push_back(varId);
		}
		else
		{
			fatalError("Unhandled variable property "+prop);
		}

		return liaVariableType::integer;
	}
	else if (theAst->iName == grammarElement::VariableWithFunction)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;

		if (!chunk.findVar(varName, varId))
		{
			// TODO: non-existant var function call
			fatalError("Variable named " + varName + " not declared at line " + std::to_string(theAst->line));
		}

		unsigned int funId = theAst->nodes[1]->tokenId;
		if (funId == libMethodId::MethodSplit)
		{
			chunk.code.push_back(liaOpcode::opVarFunctionCall);
			chunk.code.push_back(varId);
			chunk.code.push_back(funId);
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
			return liaVariableType::array;
		}
		else
		{
			fatalError("Unknown lib method ["+std::to_string(funId)+"]");
		}
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
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionToInteger)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionToInteger);
			compileExpression(theAst->nodes[1]->nodes[0], chunk);
			return liaVariableType::integer;
		}
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionReadTextFileLineByLine)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionReadTextFileLineByLine);
			compileExpression(theAst->nodes[1]->nodes[0], chunk);
			return liaVariableType::array;
		}
		else if (theAst->nodes[0]->tokenId == 0)
		{
			std::string funName = theAst->nodes[0]->token_to_string();
			unsigned int funId = -1;
			unsigned int idx = 0;
			for (auto& f : funList)
			{
				if (f.name == funName)
				{
					funId = idx;
				}
				idx += 1;
			}

			if (funId == -1)
			{
				fatalError("Call to unknown function [" + funName + "]");
			}

			chunk.code.push_back(liaOpcode::opFunctionCall);
			chunk.code.push_back(funId);

			// args
			if (theAst->nodes.size() > 1)
			{
				for (auto& arg : theAst->nodes[1]->nodes)
				{
					compileExpression(arg, chunk);
				}
			}

			// TODO: solve this
			return liaVariableType::integer;
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
		fatalError("Error: unknown expression type:"+theAst->name);
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
		chunk.basicEnv.push_back(newVar);
		varId = (unsigned int)(chunk.basicEnv.size() - 1);
	}

	assert(theAst->nodes[1]->iName == grammarElement::Expression);

	chunk.code.push_back(liaOpcode::opSetLocalVariable);
	chunk.code.push_back(varId);

	liaVariableType vartype=compileExpression(theAst->nodes[1], chunk);
	chunk.basicEnv[varId].type = vartype;
}

void liaVM::compileFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string funcName = theAst->nodes[0]->token_to_string();

	if (theAst->nodes[0]->tokenId == 0)
	{
		// custom function called
		std::string funName = theAst->nodes[0]->token_to_string();
		unsigned int funId = -1;
		unsigned int idx = 0;
		for (auto& f : funList)
		{
			if (f.name == funName)
			{
				funId = idx;
			}
			idx += 1;
		}

		if (funId == -1)
		{
			fatalError("Call to unknown function [" + funName+"]");
		}

		chunk.code.push_back(liaOpcode::opFunctionCall);
		chunk.code.push_back(funId);

		// args
		if (theAst->nodes.size() > 1)
		{
			for (auto& arg : theAst->nodes[1]->nodes)
			{
				compileExpression(arg, chunk);
			}
		}
	}
	else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionPrint)
	{
		chunk.code.push_back(liaOpcode::opStdFunctionCall);
		chunk.code.push_back(StdFunctionId::FunctionPrint);
		
		// num. arguments
		unsigned int numArgs = (unsigned int)theAst->nodes[1]->nodes.size();
		chunk.code.push_back(numArgs);
		
		// args
		for (auto& arg : theAst->nodes[1]->nodes)
		{
			compileExpression(arg, chunk);
		}
	}
	else
	{
		fatalError("Unknown function called:"+std::to_string(theAst->nodes[0]->tokenId));
	}
}

unsigned int liaVM::compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::shared_ptr<peg::Ast> lExpr = theAst->nodes[0]->nodes[0];
	int relopid = theAst->nodes[0]->nodes[1]->tokenId;
	std::shared_ptr<peg::Ast> rExpr = theAst->nodes[0]->nodes[2];

	if (relopid == relopId::RelopLess)
	{
		chunk.code.push_back(liaOpcode::opJumpIfGreaterEqual);
	}
	else if (relopid == relopId::RelopEqual)
	{
		chunk.code.push_back(liaOpcode::opJumpIfNotEqual);
	}
	else
	{
		fatalError("Unhandled relop ["+std::to_string(relopid)+"]");
	}

	unsigned int jumpPtr = (unsigned int)chunk.code.size();
	chunk.code.push_back(0); // space for jump address

	compileExpression(lExpr, chunk);
	compileExpression(rExpr, chunk);

	return jumpPtr;
}

void liaVM::compileWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::shared_ptr<peg::Ast> pCond = theAst->nodes[0];
	std::shared_ptr<peg::Ast> pCodeBlock = theAst->nodes[1];

	unsigned int loopAddress = (unsigned int)chunk.code.size();

	// if condition of while is false, we must skip the code block (to skipLoopAddress)
	unsigned int exitLoopPtr = compileCondition(pCond, chunk);

	compileCodeBlock(pCodeBlock, chunk);

	chunk.code.push_back(liaOpcode::opJump);
	chunk.code.push_back(loopAddress);

	unsigned int skipLoopAddress = (unsigned int)chunk.code.size();
	chunk.code[exitLoopPtr] = skipLoopAddress;
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

void liaVM::compileForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);
	// hand-crafted pseudo-assembly follows

	// idx=0;
	// while (idx<arr.length)
	// {
	//	tmpvar=arr[idx];
	//  idx+=1;
	// }

	std::string tmpVarName = theAst->nodes[0]->token_to_string();
	std::string iteratedName = theAst->nodes[1]->nodes[0]->nodes[0]->token_to_string();

	unsigned int iteratedId;
	if (!chunk.findVar(iteratedName, iteratedId))
	{
		fatalError("Variable named " + iteratedName + " not declared previously, at line " + std::to_string(theAst->line));
	}

	// idx=0
	liaVariable idxVar;
	idxVar.name = "idx_internal"+std::to_string(chunk.getNextSeq());
	idxVar.type = liaVariableType::integer;
	chunk.basicEnv.push_back(idxVar);
	unsigned int idxVarId = (unsigned int)(chunk.basicEnv.size()-1);

	chunk.code.push_back(liaOpcode::opSetLocalVariable);
	chunk.code.push_back(idxVarId);

	liaVariable zero;
	zero.type = liaVariableType::integer;
	zero.value = 0;
	constantPool.push_back(zero);
	const unsigned int zeroConstId = (unsigned int)(constantPool.size() - 1);

	chunk.code.push_back(liaOpcode::opConstant);
	chunk.code.push_back(zeroConstId);

	// while (idx<arr.length)
	std::shared_ptr<peg::Ast> pCodeBlock = theAst->nodes[2];
	unsigned int loopAddress = (unsigned int)chunk.code.size();

	chunk.code.push_back(liaOpcode::opJumpIfGreaterEqual);
	unsigned int exitLoopPtr = (unsigned int)chunk.code.size();
	chunk.code.push_back(0); // space for jump address

	chunk.code.push_back(liaOpcode::opGetLocalVariable);
	chunk.code.push_back(idxVarId);
	chunk.code.push_back(liaOpcode::opGetObjectLength);
	chunk.code.push_back(iteratedId);

	// tmpVar=arr[idx]
	liaVariable tmpVar;
	tmpVar.name = tmpVarName;
	chunk.basicEnv.push_back(tmpVar);
	unsigned int tmpVarId = (unsigned int)(chunk.basicEnv.size() - 1);

	chunk.code.push_back(liaOpcode::opSetLocalVariable);
	chunk.code.push_back(tmpVarId);

	chunk.code.push_back(liaOpcode::opArraySubscript);
	chunk.code.push_back(iteratedId);
	chunk.code.push_back(1);
	chunk.code.push_back(liaOpcode::opGetLocalVariable);
	chunk.code.push_back(idxVarId);

	compileCodeBlock(pCodeBlock, chunk);

	// idx+=1
	liaVariable oneConstant;
	oneConstant.type = liaVariableType::integer;
	oneConstant.value = 1;
	constantPool.push_back(oneConstant);
	const unsigned int oneConstId = (unsigned int)(constantPool.size() - 1);

	chunk.code.push_back(liaOpcode::opPostIncrement);
	chunk.code.push_back(idxVarId);
	chunk.code.push_back(liaOpcode::opConstant);
	chunk.code.push_back(oneConstId);

	chunk.code.push_back(liaOpcode::opJump);
	chunk.code.push_back(loopAddress);

	chunk.code[exitLoopPtr] = (unsigned int)chunk.code.size();
}

void liaVM::compileVarFunctionCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// TODO: non-existant var function call
		fatalError("Variable named " + varName + " not declared at line " + std::to_string(theAst->line));
	}

	unsigned int funId = theAst->nodes[1]->tokenId;

	chunk.code.push_back(liaOpcode::opVarFunctionCall);
	chunk.code.push_back(varId);
	chunk.code.push_back(funId);
	compileExpression(theAst->nodes[2]->nodes[0], chunk);
}

void liaVM::compileIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	if (theAst->nodes.size() == 2)
	{
		// simple if
		unsigned int exitLoopPtr = compileCondition(theAst->nodes[0], chunk);

		compileCodeBlock(theAst->nodes[1], chunk);

		unsigned int skipLoopAddress = (unsigned int)chunk.code.size();
		chunk.code[exitLoopPtr] = skipLoopAddress;
	}
	else if (theAst->nodes.size() == 3)
	{
		// if/else
		unsigned int exitLoopPtr = compileCondition(theAst->nodes[0], chunk);

		compileCodeBlock(theAst->nodes[1], chunk);

		chunk.code.push_back(liaOpcode::opJump);
		chunk.code.push_back(0);

		unsigned int jumpElsePtr = (unsigned int)(chunk.code.size() - 1);

		unsigned int elseAddress = (unsigned int)chunk.code.size();
		chunk.code[exitLoopPtr] = elseAddress;

		compileCodeBlock(theAst->nodes[2], chunk);

		unsigned int afterElseAddress = (unsigned int)chunk.code.size();
		chunk.code[jumpElsePtr] = afterElseAddress;
	}
}

void liaVM::compileReturnStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	if (theAst->nodes.size() == 0)
	{
		// return without parameters
		chunk.code.push_back(liaOpcode::opReturn);
		chunk.code.push_back(0);
	}
	else
	{
		chunk.code.push_back(liaOpcode::opReturn);
		chunk.code.push_back(1);
		compileExpression(theAst->nodes[0], chunk);
	}
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
		else if (stmt->nodes[0]->iName == grammarElement::ForeachStmt)
		{
			compileForeachStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::VarFuncCallStmt)
		{
			compileVarFunctionCallStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::IfStmt)
		{
			compileIfStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::ReturnStmt)
		{
			compileReturnStatement(stmt->nodes[0], chunk);
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

void liaVM::printCompilationStats()
{
	std::cout << "Compiled " << funList.size() << " functions." << std::endl;
	std::cout << "Chunks with size [";
	unsigned int idx = 0;
	for (auto& c : chunks)
	{
		std::cout << funList[idx].name << ":" << c.code.size() << ",";
		idx += 1;
	}
	std::cout << "]" << std::endl;
}

int liaVM::compile(const std::shared_ptr<peg::Ast>& theAst, std::vector<std::string> params, std::unordered_map<std::string, liaFunction> functionList)
{
	for (auto& f : functionList)
	{
		funList.push_back(f.second);
	}

	for (auto& f : functionList)
	{
		liaCodeChunk chunk;

		// inject function parameters into chunk's env footprint
		for (auto& parm : f.second.parameters)
		{
			liaVariable p;
			p.name = parm.name;
			chunk.basicEnv.push_back(p);
		}

		compileCodeBlock(f.second.functionCodeBlockAst,chunk);
		chunks.push_back(chunk);
	}

	printCompilationStats();

	return 0;
}

liaVM::~liaVM()
{
}
