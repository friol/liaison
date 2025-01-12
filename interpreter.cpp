
//
// the interpreter
// liaiason - project for the hard times
// (c) friol 2k23
//

#include "parser.h"
#include "interpreter.h"

liaInterpreter::liaInterpreter()
{
	srand((unsigned int)time(0));
}

void liaInterpreter::fatalError(std::string err)
{
	std::cout << "Error: " << err << std::endl;
	throw interpreterException();
}

int liaInterpreter::validateMainFunction(std::shared_ptr<peg::Ast> theAst)
{
	bool mainFound = false;
	bool paramsFound = false;
	for (auto node : theAst->nodes)
	{
		if (node->iName == grammarElement::TopLevelStmt)
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->iName == grammarElement::FuncDeclStmt)
				{
					for (auto funcNode : innerNode->nodes)
					{
						if (funcNode->is_token)
						{
							if (funcNode->token == "main")
							{
								mainFound = true;
							}
						}
						else
						{
							if (funcNode->iName == grammarElement::FuncParamList)
							{
								// check if function has one parameter named "params"
								for (auto mainNode : funcNode->nodes)
								{
									if (mainNode->is_token)
									{
										if (mainNode->token == "params")
										{
											paramsFound = true;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}

	// TODO: check if the function has just the 'params' parameter, or more than one parameter (wrong)

	if (mainFound && paramsFound)
	{
		return 0;
	}
	else
	{
		std::cout << "Error: Couldn't find the 'main' function, or this function hasn't a parameter named 'params'." << std::endl;
	}

	return 1;
}

int liaInterpreter::validateAst(std::shared_ptr<peg::Ast> theAst)
{
	// check0: the parsed ast should be a "LiaProgram"
	
	auto name = theAst->original_name;
	if (name != "LiaProgram")
	{
		std::cout << "Not a valid LiaProgram" << std::endl;
		return 1;
	}

	// check1: check that the provided code has a main function
	// with the correct "params" parameter
	
	auto retcode = validateMainFunction(theAst);
	if (retcode!=0)
	{
		return 1;
	}

	return 0;
}

void liaInterpreter::getFunctions(std::shared_ptr<peg::Ast> theAst)
{
	// store all the functions and their parameters in the internal structure

	for (auto node : theAst->nodes)
	{
		if (node->iName == grammarElement::TopLevelStmt)
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->iName == grammarElement::FuncDeclStmt)
				{
					liaFunction aFun;
					for (auto funcNode : innerNode->nodes)
					{
						if (funcNode->is_token)
						{
							aFun.name = funcNode->token;
							for (auto el : innerNode->nodes)
							{
								if (el->iName == grammarElement::CodeBlock)
								{
									aFun.functionCodeBlockAst = el;
								}
							}
						}
						else
						{
							if (funcNode->iName == grammarElement::FuncParamList)
							{
								for (auto mainNode : funcNode->nodes)
								{
									if (mainNode->is_token)
									{
										std::string tokenCopy;
										tokenCopy+= mainNode->token;
										liaFunctionParam funcParm;
										funcParm.name = tokenCopy;
										aFun.parameters.push_back(funcParm);
									}
								}
							}
						}

					}

					functionMap[aFun.name] = aFun;
					//functionList.push_back(aFun);
				}
			}
		}
	}
}

void liaInterpreter::dumpFunctions()
{
	//for (liaFunction f: functionList)
	for (auto& fx : functionMap)
	{
		liaFunction f = fx.second;
		std::cout << f.name << std::endl;
		for (liaFunctionParam fp : f.parameters)
		{
			std::cout << " - " << fp.name << std::endl;
		}
		//std::cout << peg::ast_to_s(f.functionCodeBlockAst);
	}
}

void liaInterpreter::replaceAll(std::string& str, const std::string& from, const std::string& to)
{
	if (from.empty()) return;
	size_t start_pos = 0;
	while ((start_pos = str.find(from, start_pos)) != std::string::npos)
	{
		str.replace(start_pos, from.length(), to);
		start_pos += to.length();
	}
}

bool liaInterpreter::liaVariableArrayComparison(liaVariable& v0, liaVariable& v1)
{
	if (v0.type == v1.type)
	{
		if (v0.type == liaVariableType::array)
		{
			for (int idx = 0;idx < v0.vlist.size();idx++)
			{
				if (!liaVariableArrayComparison(v0.vlist[idx], v1.vlist[idx]))
				{
					return false;
				}
			}

			return true;
		}
		else
		{
			return v0.value == v1.value;
		}
	}

	return false;
}

bool compareIntVars(liaVariable i1, liaVariable i2)
{
	int iv1 = std::get<int>(i1.value);
	int iv2 = std::get<int>(i2.value);
	return (iv1<iv2);
}

bool compareLongVars(liaVariable i1, liaVariable i2)
{
	long long iv1 = std::get<long long>(i1.value);
	long long iv2 = std::get<long long>(i2.value);
	return (iv1 < iv2);
}

bool compareStringVars(liaVariable i1, liaVariable i2)
{
	std::string iv1 = std::get<std::string>(i1.value);
	std::string iv2 = std::get<std::string>(i2.value);
	return (iv1 < iv2);
}

void liaInterpreter::exeCuteMethodCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, std::string varName,liaVariable& retVal)
{
	//std::cout << peg::ast_to_s(theAst);

	std::vector<liaVariable> parameters;

	size_t lineNum = theAst->line;

	// get variable
	liaVariable* pvarValue = findVar(varName, lineNum, env);

	if (theAst->nodes.size() == 3)
	{
		for (auto& expr : theAst->nodes[2]->nodes)
		{
			assert(expr->iName == grammarElement::Expression);
			liaVariable arg;
			evaluateExpression(expr, env, arg);
			parameters.push_back(arg);
		}
	}

	const int funId = theAst->nodes[1]->tokenId;

	if (funId == libMethodId::MethodSplit)
	{
		if (pvarValue->type != liaVariableType::string)
		{
			std::string err;
			err += "Split can be applied only to strings. ";
			err += "Terminating.";
			fatalError(err);
		}
					
		assert(parameters.size() == 1);

		retVal.type = liaVariableType::array;

		std::string string2split = std::get<std::string>(pvarValue->value);
		std::string delimiter = std::get<std::string>(parameters[0].value);
		//std::cout << delimiter << std::endl;

		size_t pos = 0;
		std::string token;
		while ((pos = string2split.find(delimiter)) != std::string::npos)
		{
			token = string2split.substr(0, pos);
			liaVariable varEl;
			varEl.type = liaVariableType::string;
			varEl.value = token;
			retVal.vlist.push_back(varEl);
			string2split.erase(0, pos + delimiter.length());
		}

		token = string2split.substr(0, pos);
		liaVariable varEl;
		varEl.type = liaVariableType::string;
		varEl.value = token;
		retVal.vlist.push_back(varEl);
	}
	else if (funId == libMethodId::MethodAdd)
	{
		// add element to an array
		assert(pvarValue->type == liaVariableType::array);
		assert(parameters.size() == 1);

		pvarValue->vlist.push_back(parameters[0]);
	}
	else if (funId == libMethodId::MethodFindkey)
	{
		assert(parameters.size() == 1);
		assert(pvarValue->type == liaVariableType::dictionary);

		retVal.type = liaVariableType::integer;
		retVal.value = -1;

		if (parameters[0].type == liaVariableType::string)
		{
			std::string key2find = std::get<std::string>(parameters[0].value);
			if (pvarValue->vMap.find(key2find) != pvarValue->vMap.end())
			{
				retVal.value = 1;
			}
		}
		else
		{
			std::string err;
			err += "Unhandled findKey type at " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
	}
	else if (funId == libMethodId::MethodFind)
	{
		// s.find("value"), returns idx of found element, or -1 if not found
		//assert(pvarValue->type == liaVariableType::array);
		assert(parameters.size() == 1);

		retVal.type = liaVariableType::integer;
		retVal.value = -1;

		if (parameters[0].type == liaVariableType::string)
		{
			std::string val2find=std::get<std::string>(parameters[0].value);

			if (pvarValue->type == liaVariableType::array)
			{
				for (int idx = 0;idx < pvarValue->vlist.size();idx++)
				{
					if (std::get<std::string>(pvarValue->vlist[idx].value) == val2find)
					{
						retVal.value = idx;
						//return retVal;
					}
				}
				/*liaVariable l2f;
				l2f.value = val2find;
				auto df=std::find(pvarValue->vlist.begin(), pvarValue->vlist.end(),l2f);
				//std::cout << pvarValue->vlist.end()-df << std::endl;
				retVal.value=(int)(pvarValue->vlist.end()-df-1);*/
			}
			else if (pvarValue->type == liaVariableType::string)
			{
				std::string s = std::get<std::string>(pvarValue->value);
				const char* ptr = strstr(s.c_str(), val2find.c_str());
				if (ptr == NULL)
				{
					retVal.value = -1;
				}
				else
				{
					const char* ptrbeg = s.c_str();
					retVal.value = (int)(ptr-ptrbeg);
				}

				return;
			}
		}
		else if (parameters[0].type == liaVariableType::integer)
		{
			int val2find = std::get<int>(parameters[0].value);
			if (pvarValue->vlist.size() > 0)
			{
				//assert(pvarValue->vlist[0].type == liaVariableType::integer);
				if (pvarValue->vlist[0].type != liaVariableType::integer)
				{
					std::string err;
					err += "Searching an integer in an array with elements not of type integer at " + std::to_string(lineNum) + ". ";
					err += "Terminating.";
					fatalError(err);
				}

				/*for (int idx = 0;idx < pvarValue->vlist.size();idx++)
				{
					if (std::get<int>(pvarValue->vlist[idx].value) == val2find)
					{
						retVal.value = idx;
						return;
					}
				}*/

				liaVariable l2f;
				l2f.value = val2find;
				ptrdiff_t pos = std::distance(pvarValue->vlist.begin(), std::find(pvarValue->vlist.begin(), pvarValue->vlist.end(), l2f));
				if (pos >= (signed int)pvarValue->vlist.size())
				{
					retVal.value = -1;
					return;
				}
				retVal.value = (int)pos;
				return;
			}
		}
		else if (parameters[0].type == liaVariableType::longint)
		{
			long long val2find = std::get<long long>(parameters[0].value);
			if (pvarValue->vlist.size() > 0)
			{
				assert(pvarValue->vlist[0].type == liaVariableType::longint);

				for (int idx = 0;idx < pvarValue->vlist.size();idx++)
				{
					if (std::get<long long>(pvarValue->vlist[idx].value) == val2find)
					{
						retVal.value = idx;
						return;
					}
				}
			}
		}
		else if (parameters[0].type == liaVariableType::array)
		{
			if (pvarValue->type != liaVariableType::array)
			{
				std::string err;
				err += "You can only search an array inside an array at "+std::to_string(lineNum)+". ";
				err += "Terminating.";
				fatalError(err);
			}

			for (int idx = 0;idx < pvarValue->vlist.size();idx++)
			{
				if (liaVariableArrayComparison(pvarValue->vlist[idx],parameters[0]))
				{
					retVal.value = idx;
					return;
				}
			}
		}
		else
		{
			std::string err;
			err += "Unhandled find type at " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
	}
	else if (funId == libMethodId::MethodReplace)
	{
		// s.replace(<string to replace>,<what to replace with>)
		// replace all the occurrencies of a string with another string
		assert(parameters.size() == 2);
		assert(pvarValue->type == liaVariableType::string);

		//std::cout << peg::ast_to_s(theAst);

		std::string thes = std::get<std::string>(pvarValue->value);
		std::string replaceWhat = std::get<std::string>(parameters[0].value);
		std::string replaceWith = std::get<std::string>(parameters[1].value);

		replaceAll(thes, replaceWhat, replaceWith);

		retVal.type = liaVariableType::string;
		retVal.value = thes;
		return;
	}
	else if (funId == libMethodId::MethodSlice)
	{
		assert(parameters.size() == 2);
		int beg = std::get<int>(parameters[0].value);
		int end = std::get<int>(parameters[1].value);

		if (pvarValue->type == liaVariableType::string)
		{
			std::string s = std::get<std::string>(pvarValue->value);

			if (s.size() == 0)
			{
				retVal.type = liaVariableType::string;
				retVal.value = s;
				return;
			}

			std::string s2;
			int idx = 0;
			while ((idx < end) && (idx < s.size()))
			{
				if (idx >= beg)
				{
					s2 += s[idx];
				}
				idx += 1;
			}

			retVal.type = liaVariableType::string;
			retVal.value = s2;
			return;
		}
		else if (pvarValue->type == liaVariableType::array)
		{
			retVal.type = liaVariableType::array;
			int idx = 0;
			while (idx < end)
			{
				if (idx >= beg)
				{
					retVal.vlist.push_back(pvarValue->vlist[idx]);
				}
				idx += 1;
			}

			return;
		}
	}
	else if (funId == libMethodId::MethodSort)
	{
		// sort monodimensional array
		assert(pvarValue->type == liaVariableType::array);

		if (parameters.size() != 0)
		{
			std::string err;
			err += "Sort function doesn't accept parameters. ";
			err += "Terminating.";
			fatalError(err);
		}

		if (pvarValue->vlist.size() != 0)
		{
			if (pvarValue->vlist[0].type == liaVariableType::integer)
			{
				std::sort(pvarValue->vlist.begin(), pvarValue->vlist.end(), compareIntVars);
			}
			else if (pvarValue->vlist[0].type == liaVariableType::string)
			{
				std::sort(pvarValue->vlist.begin(), pvarValue->vlist.end(), compareStringVars);
			}
			else
			{
				// TODO: unsupported type in sort
			}
		}
	}
	else if (funId == libMethodId::MethodClear)
	{
		// clear something
		if (pvarValue->type == liaVariableType::dictionary)
		{
			pvarValue->vMap.clear();
		}
	}
	else
	{
		// unknown method called
		std::string err;
		err += "Method name [" + theAst->nodes[1]->token_to_string() + "] not found. ";
		err += "Terminating.";
		fatalError(err);
	}
}

void inline liaInterpreter::evaluateExpression(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env,liaVariable& retVar)
{
	//std::cout << peg::ast_to_s(theAst);
	//size_t lineNum = theAst->nodes[0]->line;

	const int iName = theAst->nodes[0]->iName;

	if (iName == grammarElement::IntegerNumber)
	{
		retVar.type = liaVariableType::integer;
		retVar.value = theAst->nodes[0]->iNumber;
	}
	else if (iName == grammarElement::VariableName)
	{
		size_t lineNum = theAst->nodes[0]->line;
		std::string varName = theAst->nodes[0]->token_to_string();

		liaVariable* v = findVar(varName, lineNum, env);
		if (v->type == liaVariableType::integer)
		{
			int val2print = std::get<int>(v->value);
			retVar.type = liaVariableType::integer;
			retVar.value = val2print;
		}
		else if (v->type == liaVariableType::longint)
		{
			long long val2print = std::get<long long>(v->value);
			retVar.type = liaVariableType::longint;
			retVar.value = val2print;
		}
		else if (v->type == liaVariableType::string)
		{
			std::string s2print = std::get<std::string>(v->value);
			retVar.type = liaVariableType::string;
			retVar.value = s2print;
		}
		else if (v->type == liaVariableType::boolean)
		{
			bool sval = std::get<bool>(v->value);
			retVar.type = liaVariableType::boolean;
			if (sval == true) retVar.value = true;
			else retVar.value = false;
		}
		else if (v->type == liaVariableType::array)
		{
			retVar.type = liaVariableType::array;
			for (auto el : v->vlist)
			{
				retVar.vlist.push_back(el);
			}
		}
		else if (v->type == liaVariableType::dictionary)
		{
			retVar.type = liaVariableType::dictionary;
			for (std::map<std::string, liaVariable>::iterator it = v->vMap.begin(); it != v->vMap.end(); ++it)
			{
				std::string key = it->first;
				retVar.vMap[key] = v->vMap[key];
			}
		}
	}
	else if (iName == grammarElement::StringLiteral)
	{
		retVar.type = liaVariableType::string;
		retVar.value = theAst->nodes[0]->token_to_string();
	}
	else if (iName == grammarElement::ArraySubscript)
	{
		//std::cout << peg::ast_to_s(theAst->nodes[0]);
		size_t lineNum = theAst->nodes[0]->line;

		std::string arrName = theAst->nodes[0]->nodes[0]->token_to_string();
		liaVariable* pVar = findVar(arrName, lineNum, env);

		if (pVar->type == liaVariableType::array)
		{
			liaVariable vIdx;
			unsigned int limit;

			for (int aidx = 1;aidx < theAst->nodes[0]->nodes.size();aidx++)
			{
				//std::cout << "idx is " << std::get<int>(vIdxArr[aidx].value) << std::endl;
				evaluateExpression(theAst->nodes[0]->nodes[aidx], env, vIdx);
				unsigned int arrIdx = std::get<int>(vIdx.value);

				if (pVar->type == liaVariableType::array)
				{
					limit = (unsigned int)pVar->vlist.size();
				}
				else if (pVar->type == liaVariableType::string)
				{
					limit = (unsigned int)std::get<std::string>(pVar->value).size();
				}
				else
				{
					std::string err;
					err += "Unhandled type for sub-array indexing at " + std::to_string(lineNum) + ".";
					err += "Terminating.";
					fatalError(err);
				}

				if (arrIdx >= limit)
				{
					std::string err;
					err += "Array index out of range at " + std::to_string(lineNum) + ". Requested index: " +
						std::to_string(arrIdx) + " size of array: " + std::to_string(pVar->vlist.size()) + ". ";
					err += "Terminating.";
					fatalError(err);
				}

				if (pVar->type == liaVariableType::array)
				{
					pVar = &pVar->vlist[arrIdx];

					if (aidx == (theAst->nodes[0]->nodes.size() - 1))
					{
						retVar.type = pVar->type;
						retVar.value = pVar->value;
						retVar.vlist = pVar->vlist;
						retVar.vMap = pVar->vMap;
					}
				}
				else if (pVar->type == liaVariableType::string)
				{
					// we assume this is the last subscript of the array
					// otherwise, everything will crash
					retVar.type = liaVariableType::string;
					std::string s = std::get<std::string>(pVar->value);
					std::string rv = s.substr(arrIdx, 1);
					retVar.value = rv;
				}
			}
		}
		else if (pVar->type == liaVariableType::dictionary)
		{
			std::vector<liaVariable> vIdxArr;
			for (int i = 1;i < theAst->nodes[0]->nodes.size();i++)
			{
				liaVariable vIdx;
				evaluateExpression(theAst->nodes[0]->nodes[i], env, vIdx);
				vIdxArr.push_back(vIdx);
			}

			if (vIdxArr.size() != 1)
			{
				std::string err;
				err += "Dictionaries can only be indexed once. ";
				err += "Terminating.";
				fatalError(err);
			}

			std::string key = std::get<std::string>(vIdxArr[0].value);

			// check if key is in dictionary keys
			if (pVar->vMap.find(key) == pVar->vMap.end())
			{
				std::string err;
				err += "Key " + key + " not found in dictionary. ";
				err += "Terminating.";
				fatalError(err);
			}
			else
			{
				retVar.type = pVar->vMap[key].type;
				retVar.value = pVar->vMap[key].value;
				retVar.vlist = pVar->vMap[key].vlist;
				retVar.vMap = pVar->vMap[key].vMap;
			}
		}
		else if (pVar->type == liaVariableType::string)
		{
			std::vector<liaVariable> vIdxArr;
			for (int i = 1;i < theAst->nodes[0]->nodes.size();i++)
			{
				liaVariable vIdx;
				evaluateExpression(theAst->nodes[0]->nodes[i], env, vIdx);
				vIdxArr.push_back(vIdx);
			}

			if (vIdxArr.size() != 1)
			{
				std::string err;
				err += "Strings are mono-dimensional. ";
				err += "Terminating.";
				fatalError(err);
			}

			int arrIdx = std::get<int>(vIdxArr[0].value);
			std::string tmp = std::get<std::string>(pVar->value);

			if (arrIdx >= tmp.size())
			{
				std::string err;
				err += "String index out of range at " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			retVar.type = liaVariableType::string;
			char c = tmp.at(arrIdx);
			std::string sc;
			sc += c;
			retVar.value = sc;
		}
		else
		{
			std::string err;
			err += "Subscript is applicable only to arrays, strings and dictionaries at " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
	}
	else if (iName == grammarElement::LongNumber)
	{
		std::string tmp;
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::longint;
		retVar.value = std::stoll(tmp);
	}
	else if (iName == grammarElement::BooleanConst)
	{
		std::string tmp;
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::boolean;
		if (tmp=="true") retVar.value = true;
		else retVar.value = false;
	}
	else if (iName == grammarElement::BitwiseNot)
	{
		//std::cout << "NOT/RJ" << peg::ast_to_s(theAst);

		assert(theAst->nodes[0]->nodes.size() == 1);
		assert(theAst->nodes[0]->nodes[0]->iName == grammarElement::Expression);

		liaVariable expr;
		evaluateExpression(theAst->nodes[0]->nodes[0], env,expr);
		assert(expr.type == liaVariableType::integer);

		retVar.type = liaVariableType::integer;
		retVar.value = ~std::get<int>(expr.value);
	}
	else if (iName == grammarElement::MinusExpression)
	{
		assert(theAst->nodes[0]->nodes.size() == 1);
		assert(theAst->nodes[0]->nodes[0]->iName == grammarElement::Expression);

		liaVariable expr;
		evaluateExpression(theAst->nodes[0]->nodes[0], env,expr);
		assert(expr.type == liaVariableType::integer);

		retVar.type = liaVariableType::integer;
		retVar.value = -std::get<int>(expr.value);
	}
	else if (iName == grammarElement::VariableWithFunction)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName=theAst->nodes[0]->nodes[0]->token_to_string();
		exeCuteMethodCallStatement(theAst->nodes[0], env,varName, retVar);
	}
	else if (iName == grammarElement::VariableWithProperty)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName;
		std::string varProp;
		liaVariable arridx;

		//std::cout << theAst->nodes[0]->name << std::endl;

		// TODO: should handle multidimensional arrays with the usual method
		if (theAst->nodes[0]->nodes[0]->iName == grammarElement::ArraySubscript)
		{
			varName += theAst->nodes[0]->nodes[0]->nodes[0]->token;
			evaluateExpression(theAst->nodes[0]->nodes[0]->nodes[1],env,arridx);
			if (arridx.type != liaVariableType::integer)
			{
				std::string err;
				size_t lineNum = theAst->nodes[0]->line;
				err += "Array index should be an integer at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
			varProp+= theAst->nodes[0]->nodes[1]->token;
		}
		else
		{
			varName += theAst->nodes[0]->nodes[0]->token;
			varProp += theAst->nodes[0]->nodes[1]->token;
		}

		liaVariable* v = nullptr;
		//std::cout << varName << std::endl;
		if (env->varMap.find(varName) == env->varMap.end())
		{
			if (globalScope.varMap.find(varName) == globalScope.varMap.end())
			{
				// variable name not found
				std::string err;
				size_t lineNum = theAst->nodes[0]->line;
				err += "Variable [" + varName + "] not found in method call at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
			else
			{
				v = &globalScope.varMap[varName];
			}
		}
		else
		{
			v = &env->varMap[varName];
		}

		if (theAst->nodes[0]->nodes[0]->iName == grammarElement::ArraySubscript)
		{
			v = &v->vlist[std::get<int>(arridx.value)];
		}

		if (varProp == "length")
		{
			if (v->type == liaVariableType::string)
			{
				std::string s2print = std::get<std::string>(v->value);
				auto len = s2print.size();
				retVar.type = liaVariableType::integer;
				retVar.value = (int)len;
			}
			else if (v->type == liaVariableType::array)
			{
				retVar.type = liaVariableType::integer;
				retVar.value = (int)v->vlist.size();
			}
			else
			{
				// makes no sense (at least for me) for other types
				std::string err;
				err += "'length' property is applicable only to strings and arrays. ";
				err += "Terminating.";
				fatalError(err);
			}
		}
		else if (varProp == "keys")
		{
			assert(v->type == liaVariableType::dictionary);

			retVar.type = liaVariableType::array;

			for (std::map<std::string,liaVariable>::iterator it = v->vMap.begin(); it != v->vMap.end(); ++it) 
			{
				std::string key = it->first;
				liaVariable s;
				s.type = liaVariableType::string;
				s.value = key;
				retVar.vlist.push_back(s);
			}
		}
		else
		{
			// property name not found
			std::string err;
			err += "Property name [" + varProp + "] not found";
			err += "Terminating.";
			fatalError(err);
		}

	}
	else if (iName == grammarElement::ArrayInitializer)
	{
		retVar.type = liaVariableType::array;
		if (theAst->nodes[0]->nodes.size() != 0)
		{
			assert(theAst->nodes[0]->nodes[0]->iName == grammarElement::ExpressionList);

			for (auto& el : theAst->nodes[0]->nodes[0]->nodes)
			{
				liaVariable varel;
				evaluateExpression(el, env,varel);
				retVar.vlist.push_back(varel);
			}
		}
	}
	else if (iName == grammarElement::RFuncCall)
	{
		//std::cout << "funCall" << std::endl;
		//std::cout << peg::ast_to_s(theAst);
		exeCuteFuncCallStatement(theAst->nodes[0], env, retVar);
	}
	else if (iName ==grammarElement::DictInitializer)
	{
		//std::cout << peg::ast_to_s(theAst);
		retVar.type = liaVariableType::dictionary;

		if (theAst->nodes[0]->nodes.size() != 0)
		{
			assert(theAst->nodes[0]->nodes[0]->name == "DictList");
			assert(theAst->nodes[0]->nodes[0]->nodes[0]->name == "KeyValueList");
			assert((theAst->nodes[0]->nodes[0]->nodes[0]->nodes.size() % 2) == 0);

			std::string curKey;
			liaVariable curVar;
			for (auto kv : theAst->nodes[0]->nodes[0]->nodes[0]->nodes)
			{
				if (kv->is_token)
				{
					curKey = "";
					curKey += kv->token;
					std::regex quote_re("\"");
					curKey = std::regex_replace(curKey, quote_re, "");
				}
				else
				{
					evaluateExpression(kv, env, curVar);
					retVar.vMap[curKey] = curVar;
				}
			}
		}
	}
	else if ( (theAst->iName==grammarElement::Expression) && ((theAst->nodes.size() % 2) == 1) && (theAst->nodes.size()>2))
	{
		int nodepos = 0;

		liaVariable vResult;
		evaluateExpression(theAst->nodes[nodepos], env,vResult);
		nodepos += 1;

		while (nodepos < theAst->nodes.size())
		{
			std::string opz;
			opz += theAst->nodes[nodepos]->token;

			liaVariable v1;
			evaluateExpression(theAst->nodes[nodepos + 1], env,v1);

			if (vResult.type != v1.type)
			{
				std::string err;
				size_t lineNum = theAst->nodes[0]->line;
				err += "Only expressions of elements with the same type are supported at the moment at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			if ((vResult.type != liaVariableType::integer)&&(vResult.type != liaVariableType::longint)&& (vResult.type != liaVariableType::string))
			{
				std::string err;
				size_t lineNum = theAst->nodes[0]->line;
				err += "Only expressions of elements with type integer/long/string are supported at the moment " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			if (vResult.type == liaVariableType::integer)
			{
				int partialResult = std::get<int>(vResult.value);
				int newVal = std::get<int>(v1.value);

				if (opz == "+") partialResult += newVal;
				else if (opz == "-") partialResult -= newVal;
				else if (opz == "*") partialResult *= newVal;
				else if (opz == "/") partialResult /= newVal;
				else if (opz == "%") partialResult %= newVal;
				else if (opz == "^") partialResult ^= newVal;

				vResult.value = partialResult;
			}
			else if (vResult.type == liaVariableType::longint)
			{
				long long partialResult = std::get<long long>(vResult.value);
				long long newVal = std::get<long long>(v1.value);

				if (opz == "+") partialResult += newVal;
				else if (opz == "-") partialResult -= newVal;
				else if (opz == "*") partialResult *= newVal;
				else if (opz == "/") partialResult /= newVal;
				else if (opz == "%") partialResult %= newVal;
				else if (opz == "^") partialResult ^= newVal;

				vResult.value = partialResult;
			}
			else if (vResult.type == liaVariableType::string)
			{
				std::string partialResult = std::get<std::string>(vResult.value);
				std::string newVal = std::get<std::string>(v1.value);

				if (opz == "+") partialResult += newVal;
				else
				{
					// only + supported for strings
					std::string err;
					size_t lineNum = theAst->nodes[0]->line;
					err += "Only + is supported for strings at line " + std::to_string(lineNum) + ". ";
					err += "Terminating.";
					fatalError(err);
				}

				vResult.value = partialResult;
			}

			nodepos += 2;
		}

		retVar=vResult;
	}
	else if ((theAst->iName ==grammarElement::InnerExpression) || (theAst->iName == grammarElement::Expression))
	{
		evaluateExpression(theAst->nodes[0], env,retVar);
	}
	else if (theAst->iName == grammarElement::NotExpression)
	{
		evaluateExpression(theAst->nodes[0], env, retVar);
		retVar.value = !std::get<bool>(retVar.value);
	}
}

void liaInterpreter::innerPrint(liaVariable& var)
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
	else if (var.type == liaVariableType::dictionary)
	{
		bool first = true;
		std::cout << "{";

		for (const auto& myPair : var.vMap)
		{
			if (!first) std::cout << ",";
			std::cout << "\"" << myPair.first << "\"" << ":";
			
			liaVariable v2p = myPair.second;
			innerPrint(v2p); // std::get<int>(myPair.second.value);
			
			first = false;
		}
		std::cout << "} ";
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

void liaInterpreter::exeCuteLibFunctionPrint(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);
	assert(theAst->iName == grammarElement::ArgList);
	assert(theAst->nodes[0]->iName == grammarElement::Expression);

	for (auto node : theAst->nodes)
	{
		liaVariable retVar;
		evaluateExpression(node, env,retVar);
		innerPrint(retVar);
	}

	std::cout << std::endl;
}

void liaInterpreter::exeCuteLibFunctionReadFile(std::string fname,int linenum, liaVariable& retVar)
{
	//liaVariable retVar;
	retVar.type = liaVariableType::array;

	std::string line;
	std::ifstream file(fname);
	if (file.is_open()) 
	{
		while (std::getline(file, line)) 
		{
			liaVariable l;
			l.type = liaVariableType::string;
			l.value = line;
			retVar.vlist.push_back(l);
		}
		file.close();
	}
	else
	{
		std::string err;
		err += "Could not open file [" + fname + "] at line "+ std::to_string(linenum)+".Terminating.";
		fatalError(err);
	}

	//return retVar;
}

void liaInterpreter::customFunctionCall(std::string& fname, std::vector<liaVariable>* parameters, liaEnvironment* env,size_t lineNum,liaVariable& retVal)
{
	//liaVariable retVal;

	// check if function name exists
	bool funFound = false;
	std::shared_ptr<peg::Ast> pBlock;

	if (functionMap.find(fname) != functionMap.end())
	{
		liaFunction f = functionMap[fname];
		pBlock = f.functionCodeBlockAst;

		// check that number of parameters is the same of function call

		if (f.parameters.size() != parameters->size())
		{
			std::string err;
			err += "Function " + fname + " called with wrong number of parameters at "+std::to_string(lineNum)+". "+std::to_string(parameters->size())+" parameter(s) passed ";
			err += "instead of "+std::to_string(f.parameters.size()) + ".";
			err += "Terminating.";
			fatalError(err);
		}

		for (int p = 0;p < parameters->size();p++)
		{
			(*parameters)[p].name = f.parameters[p].name;
		}
	}
	else
	{
		std::string err;
		err += "Unknown function name '" + fname + "' at line " + std::to_string(lineNum) + ". ";
		err += "Terminating.";
		fatalError(err);
	}

	// execute code block
	// what stays in the function code block stays in the function code block

	liaEnvironment funEvn;

	// add to the local function environment the function parameters
	for (auto& parm : *parameters)
	{
		//std::cout << parm.name << std::endl;
	//	addvarOrUpdateEnvironment(&parm, &funEvn, 0);
		funEvn.varMap[parm.name] = parm;
	}

	exeCuteCodeBlock(pBlock, &funEvn,retVal);
}

void liaInterpreter::exeCuteFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, liaVariable& retVal)
{
	//std::cout << peg::ast_to_s(theAst);

	//liaVariable retVal;
	std::vector<liaVariable> parameters;
	size_t lineNum=theAst->line;

	const int funId = theAst->nodes[0]->tokenId;
	if (funId==0)
	{
		// should be a custom function
		// evaluate and store the parameters

		//std::cout << peg::ast_to_s(theAst);
		std::string funName = theAst->nodes[0]->token_to_string();

		if (theAst->nodes.size() > 1)
		{
			for (auto& funparm : theAst->nodes[1]->nodes)
			{
				assert(funparm->iName == grammarElement::Expression);
				liaVariable arg;
				evaluateExpression(funparm, env, arg);
				parameters.push_back(arg);
			}
		}

		customFunctionCall(funName, &parameters, env, lineNum, retVal);
	}
	else if (funId == StdFunctionId::FunctionPrint)
	{
		exeCuteLibFunctionPrint(theAst->nodes[1],env);
	}
	else if (funId == StdFunctionId::FunctionReadTextFileLineByLine)
	{
		assert(theAst->nodes[1]->name == "ArgList");
		assert(theAst->nodes[1]->nodes[0]->name == "Expression");
		int linenum = (int)theAst->nodes[1]->nodes[0]->line;
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);
		exeCuteLibFunctionReadFile(std::get<std::string>(p0.value),linenum,retVal);
	}
	else if (funId == StdFunctionId::FunctionToInteger)
	{
		assert(theAst->nodes[1]->iName == grammarElement::ArgList);
		assert(theAst->nodes[1]->nodes[0]->iName == grammarElement::Expression);
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);
					
		// parameter to convert must be a string
		if (p0.type != liaVariableType::string)
		{
			std::string err;
			err += "toInteger accepts only string values at line " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::integer;

		std::string sVal = std::get<std::string>(p0.value);

		try
		{
			retVal.value = std::stoi(sVal);
		}
		catch (...)
		{
			// unable to convert to integer
			std::string err;
			err += "Can't convert [" + sVal + "] to integer at line "+std::to_string(lineNum)+". ";
			err += "Terminating.";
			fatalError(err);
		}

	}
	else if (funId == StdFunctionId::FunctionToLong)
	{
		assert(theAst->nodes[1]->iName == grammarElement::ArgList);
		assert(theAst->nodes[1]->nodes[0]->iName == grammarElement::Expression);
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env, p0);

		// parameter to convert must be a string
		if (p0.type != liaVariableType::string)
		{
			std::string err;
			err += "toLong accepts only string values at line " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::longint;

		std::string sVal = std::get<std::string>(p0.value);

		try
		{
			retVal.value = std::stoll(sVal);
		}
		catch (...)
		{
			// unable to convert to integer
			std::string err;
			err += "Can't convert [" + sVal + "] to long at line " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}

	}
	else if (funId == StdFunctionId::FunctionToString)
	{
		assert(theAst->nodes[1]->name == "ArgList");
		assert(theAst->nodes[1]->nodes[0]->name == "Expression");
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);

		if (p0.type == liaVariableType::integer)
		{
			retVal.type = liaVariableType::string;
			int sVal = std::get<int>(p0.value);
			retVal.value = std::to_string(sVal);
		}
		else if (p0.type==liaVariableType::longint)
		{
			retVal.type = liaVariableType::string;
			long long sVal = std::get<long long>(p0.value);
			retVal.value = std::to_string(sVal);
		}
		else
		{
			// parameter to convert must be an integer or long yep
			std::string err;
			err += "Parameter to toString should be an integer or long. ";
			err += "Terminating.";
			fatalError(err);
		}
	}
	else if (funId == StdFunctionId::FunctionOrd)
	{
		// char to its ascii code
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);

		// parameter to convert must be a string
		if (p0.type != liaVariableType::string)
		{
			std::string err;
			err += "Parameter for the 'ord' function should be a string. ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::integer;
		std::string sVal = std::get<std::string>(p0.value);
		retVal.value = (int)sVal.at(0);
	}
	else if (funId == StdFunctionId::FunctionChr)
	{
		// the opposite
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);
					
		// parameter to convert must be an integer
		if (p0.type != liaVariableType::integer)
		{
			std::string err;
			err += "Parameter for the 'chr' function should be an integer. ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::string;
		char sVal = (char)std::get<int>(p0.value);
		std::string ss;
		ss = (char)sVal;
		retVal.value = ss;
	}
	else if (funId == StdFunctionId::FunctionLsqrt)
	{
		// integer sqrt
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);

		// parameter to convert must be lang lang
		if (p0.type != liaVariableType::longint)
		{
			std::string err;
			err += "Parameter for the 'lSqrt' function should be a longint. ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::longint;
		long long iVal = std::get<longint>(p0.value);
		double fsq = sqrt(iVal);
		retVal.value = (long long)fsq;
	}
	else if (funId == StdFunctionId::FunctionRnd)
	{
		// generate a random number from zero to parameter 0
		liaVariable p0;
		evaluateExpression(theAst->nodes[1]->nodes[0], env,p0);

		// parameter must be integer
		if (p0.type != liaVariableType::integer)
		{
			std::string err;
			err += "Parameter for the 'rnd' function should be an integer. ";
			err += "Terminating.";
			fatalError(err);
		}

		retVal.type = liaVariableType::integer;
		retVal.value = rand() % std::get<int>(p0.value);
	}
	else if (funId == StdFunctionId::FunctionGetMillisecondsSinceEpoch)
	{
		// another function with an infinite name
		long long now =(long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
		//std::cout << now << std::endl;

		retVal.type = liaVariableType::longint;
		retVal.value = now;
	}
}

void liaInterpreter::exeCuteVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	std::string vname;
	size_t curLine=0;
	
	vname += theAst->nodes[0]->token;
	evaluateExpression(theAst->nodes[1], env, theVar);
	theVar.name = vname;

	// now, if variable is not in env, create it. otherwise, update it
	addvarOrUpdateEnvironment(&theVar, env, curLine);
}

void liaInterpreter::exeCuteArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	assert(theAst->nodes[0]->iName==grammarElement::ArraySubscript);
	assert(theAst->nodes[1]->iName==grammarElement::Expression);

	size_t lineNum = theAst->nodes[0]->line;

	liaVariable value2assign;
	evaluateExpression(theAst->nodes[1], env,value2assign);

	std::string arrayName;
	arrayName+=theAst->nodes[0]->nodes[0]->token;
	liaVariable* pArr = findVar(arrayName, lineNum, env);

	if (pArr->type == liaVariableType::array)
	{
		liaVariable vIdx;
		int arrIdx;
		for (int idx=1;idx<theAst->nodes[0]->nodes.size();idx++)
		{
			evaluateExpression(theAst->nodes[0]->nodes[idx], env, vIdx);
			arrIdx = std::get<int>(vIdx.value);

			if (arrIdx >= pArr->vlist.size())
			{
				std::string err;
				err += "Array index out of range in array assignment at " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			pArr = &pArr->vlist[arrIdx];
		}

		pArr->value = value2assign.value;
		pArr->vlist = value2assign.vlist;
	}
	else if (pArr->type == liaVariableType::dictionary)
	{
		liaVariable arrayIndex;
		evaluateExpression(theAst->nodes[0]->nodes[1], env,arrayIndex);
		assert(arrayIndex.type == liaVariableType::string);
		pArr->vMap[std::get<std::string>(arrayIndex.value)] = value2assign;
	}
	else if (pArr->type == liaVariableType::string)
	{
		liaVariable arrayIndex;
		evaluateExpression(theAst->nodes[0]->nodes[1], env, arrayIndex);
		assert(arrayIndex.type == liaVariableType::integer);
		int iidx = std::get<int>(arrayIndex.value);
		std::string theString = std::get<std::string>(pArr->value);

		if (iidx > theString.size())
		{
			std::string err;
			err += "String index out of range at " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}

		theString[iidx] = std::get<std::string>(value2assign.value)[0];
		pArr->value=theString;
	}
	else
	{
		std::string err;
		err += "For assignment, variable [" + arrayName + "] should be an array or a dictionary at " + std::to_string(lineNum) + ". ";
		err += "Terminating.";
		fatalError(err);
	}
}

void liaInterpreter::exeCuteMultiplyStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	long long mulAmount = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->iName == grammarElement::VariableName)
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->iName == grammarElement::Expression)
			{
				liaVariable vInc;
				evaluateExpression(ch, env,vInc);
				if ((vInc.type!=liaVariableType::integer)&& (vInc.type != liaVariableType::longint))
				{
					std::string err;
					err += "Multiply term should be an integer or long at line " + std::to_string(curLine) + ". ";
					err += "Terminating.";
					fatalError(err);
				}

				if (vInc.type == liaVariableType::integer)
				{
					mulAmount = std::get<int>(vInc.value);
				}
				else
				{
					mulAmount = std::get<long long>(vInc.value);
				}
			}
		}
	}
	
	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv*=(int)mulAmount;
	}
	else if (pvar->type == liaVariableType::longint)
	{
		long long vv = std::get<long long>(pvar->value);
		pvar->value = vv *= mulAmount;
	}
	else
	{
		std::string err;
		err += "Trying to multiply numerically a variable of other type";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}

	// TODO: handle other types
}

void liaInterpreter::exeCuteDivideStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	long long divAmount = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->iName == grammarElement::VariableName)
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->iName == grammarElement::Expression)
			{
				liaVariable vInc;
				evaluateExpression(ch, env,vInc);
				if ((vInc.type != liaVariableType::integer)&& (vInc.type != liaVariableType::longint))
				{
					std::string err;
					err += "Divide term should be an integer or a long int. ";
					err += "Terminating.";
					fatalError(err);
				}

				if (vInc.type == liaVariableType::integer)
				{
					divAmount = std::get<int>(vInc.value);
				}
				else if (vInc.type == liaVariableType::longint)
				{
					divAmount = std::get<long long>(vInc.value);
				}
			}
		}
	}

	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv /= (int)divAmount;
	}
	else if (pvar->type == liaVariableType::longint)
	{
		long long vv = std::get<long long>(env->varMap[theVar.name].value);
		env->varMap[theVar.name].value = vv /= divAmount;
	}
	else
	{
		std::string err;
		err += "Trying to divide numerically a variable of other type";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}

	// TODO: handle other types
}

void liaInterpreter::exeCuteModuloStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine = 0;
	long long modAmount = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				liaVariable vInc;
				evaluateExpression(ch, env,vInc);
				if ((vInc.type != liaVariableType::integer) && (vInc.type != liaVariableType::longint))
				{
					std::string err;
					err += "Modulo term should be an integer or a long int. ";
					err += "Terminating.";
					fatalError(err);
				}

				if (vInc.type == liaVariableType::integer)
				{
					modAmount = std::get<int>(vInc.value);
				}
				else if (vInc.type == liaVariableType::longint)
				{
					modAmount = std::get<long long>(vInc.value);
				}
			}
		}
	}

	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv %= (int)modAmount;
	}
	else if (pvar->type == liaVariableType::longint)
	{
		long long vv = std::get<long long>(pvar->value);
		pvar->value = vv %= modAmount;
	}
	else
	{
		std::string err;
		err += "Trying to perform modulo on a variable of other type";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}

	// TODO: handle other types
}

void liaInterpreter::exeCuteLogicalAndStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	int rExpr = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				liaVariable andVal;
				evaluateExpression(ch, env,andVal);
				assert(andVal.type == liaVariableType::integer);
				rExpr = std::get<int>(andVal.value);
			}
		}
	}

	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv & rExpr;
	}
	else
	{
		std::string err;
		err += "Logical and is valid only between integers";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}
}

void liaInterpreter::exeCuteLogicalOrStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	long long rExpr = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				liaVariable andVal;
				evaluateExpression(ch, env,andVal);
				//assert(andVal.type == liaVariableType::integer);
				if (andVal.type == liaVariableType::integer)
				{
					rExpr = (long long)std::get<int>(andVal.value);
				}
				else
				{
					rExpr = (long long)std::get<long long>(andVal.value);
				}
			}
		}
	}

	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv | rExpr;
	}
	else if (pvar->type == liaVariableType::longint)
	{
		long long vv = std::get<long long>(pvar->value);
		pvar->value = vv | rExpr;
	}
	else
	{
		std::string err;
		err += "Logical or is valid only between integers or longs";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}
}

void liaInterpreter::exeCuteRshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	long long rshiftAmount = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				liaVariable vInc;
				evaluateExpression(ch, env,vInc);
				assert(vInc.type == liaVariableType::integer);

				if (vInc.type == liaVariableType::integer)
				{
					rshiftAmount = (long long)std::get<int>(vInc.value);
				}
				else
				{
					rshiftAmount = std::get<long long>(vInc.value);
				}
			}
		}
	}

	if (rshiftAmount != 0)
	{
		liaVariable* pvar = findVar(theVar.name, curLine, env);
		if (pvar->type == liaVariableType::integer)
		{
			int vv = std::get<int>(pvar->value);
			pvar->value = vv>>rshiftAmount;
		}
		else if (pvar->type == liaVariableType::longint)
		{
			long long vv = std::get<long long>(pvar->value);
			pvar->value = vv >> rshiftAmount;
		}
		else
		{
			std::string err;
			err += "Trying to right shift a variable of type different from integer";
			err += " at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
	}
}

void liaInterpreter::exeCuteLshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	int lshiftAmount = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				curLine = ch->line;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				liaVariable vInc;
				evaluateExpression(ch, env,vInc);
				assert(vInc.type == liaVariableType::integer);
				lshiftAmount = std::get<int>(vInc.value);
			}
		}
	}

	if (lshiftAmount != 0)
	{
		liaVariable* pvar = findVar(theVar.name, curLine, env);
		if (pvar->type == liaVariableType::integer)
		{
			int vv = std::get<int>(pvar->value);
			pvar->value = vv << lshiftAmount;
		}
		else
		{
			std::string err;
			err += "Trying to left shift a variable of type different from integer";
			err += " at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
	}
}

liaVariable* liaInterpreter::findVar(std::string& varName, size_t& curLine, liaEnvironment* env)
{
	if (env->varMap.find(varName) == env->varMap.end())
	{
		if (globalScope.varMap.find(varName) == globalScope.varMap.end())
		{
			std::string err;
			err += "Variable \"" + varName + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			return &globalScope.varMap[varName];
		}
	}
	else
	{
		return &env->varMap[varName];
	}

	return nullptr;
}

void liaInterpreter::exeCuteIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,int inc)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=theAst->nodes[0]->line;
	liaVariable theInc;
	liaVariable dictKey;
	bool isSubscript = false;

	if (theAst->nodes[0]->iName != grammarElement::ArraySubscript)
	{
		theVar.name += theAst->nodes[0]->token;
		evaluateExpression(theAst->nodes[1], env, theInc);
	}
	else
	{
		theVar.name += theAst->nodes[0]->nodes[0]->token;
		evaluateExpression(theAst->nodes[0]->nodes[1], env, dictKey);
		evaluateExpression(theAst->nodes[1], env, theInc);
		isSubscript = true;
	}

	liaVariable* pvar = findVar(theVar.name, curLine, env);
	if ((pvar->type != liaVariableType::array) && (pvar->type != liaVariableType::dictionary) && (pvar->type != theInc.type) )
	{
		std::string err;
		err += "Variable \"" + theVar.name + "\" and increment should be of the same type at line " + std::to_string(curLine) + ". ";
		err += "Terminating.";
		fatalError(err);
	}

	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		int iInc = std::get<int>(theInc.value);
		pvar->value = vv+(iInc*inc);
	}
	else if (pvar->type == liaVariableType::longint)
	{
		assert(pvar->type == theInc.type);

		long long vv = std::get<long long>(pvar->value);
		long long iInc = std::get<long long>(theInc.value);
		pvar->value = vv + (iInc * inc);
	}
	else if (pvar->type == liaVariableType::string)
	{
		std::string vv = std::get<std::string>(pvar->value);
		std::string appendix = std::get<std::string>(theInc.value);
		pvar->value = vv + appendix;
	}
	else if (pvar->type == liaVariableType::array)
	{
		if (isSubscript)
		{
			// arr[x]+=n;
			int iInc = std::get<int>(theInc.value);
			int vecVal = std::get<int>(pvar->vlist[std::get<int>(dictKey.value)].value);
			pvar->vlist[std::get<int>(dictKey.value)].value = vecVal + (iInc * inc);
		}
		else
		{
			// arr1+=arr2; (concatenation of arrays)
			for (auto& el : theInc.vlist)
			{
				pvar->vlist.push_back(el);
			}
		}
	}
	else if (pvar->type == liaVariableType::dictionary)
	{
		if (!isSubscript)
		{
			std::string err;
			err += "You can't increment a dictionary at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}

		int iInc = std::get<int>(theInc.value);
		int vv = std::get<int>(pvar->vMap[std::get<std::string>(dictKey.value)].value);
		vv += iInc * inc;
		pvar->vMap[std::get<std::string>(dictKey.value)].value = vv;
	}
	else
	{
		std::string err;
		err += "Unhandled increment type";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}

	// TODO: handle other types
}

void liaInterpreter::addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env,size_t curLine)
{
	liaVariable* pVar;
	
	// check first in the global environment
	if (globalScope.varMap.find(v->name) != globalScope.varMap.end())
	{
		pVar = &globalScope.varMap[v->name];
	}
	else if (env->varMap.find(v->name) != env->varMap.end())
	{
		pVar = &env->varMap[v->name];
	}
	else
	{
		// else, add it to the env
		env->varMap[v->name] = *v;
		return;
	}

	// updating the var value
	// TODO handle other types
	if (v->type == liaVariableType::integer)
	{
		pVar->value = v->value;
	}
	else if (v->type == liaVariableType::longint)
	{
		pVar->value = v->value;
	}
	else if (v->type == liaVariableType::string)
	{
		pVar->value = v->value;
	}
	else if (v->type == liaVariableType::boolean)
	{
		pVar->value = v->value;
	}
	else if (v->type == liaVariableType::array)
	{
		pVar->vlist.clear();
		for (auto el : v->vlist)
		{
			pVar->vlist.push_back(el);
		}
	}
	else if (v->type == liaVariableType::dictionary)
	{
		pVar->vMap = v->vMap;
	}
	else
	{
		std::string err;
		err += "Unhandled type for scope variable update at " + std::to_string(curLine) + ". ";
		err += "Terminating.";
		fatalError(err);
	}
}

template <typename T> bool liaInterpreter::primitiveComparison(T& leftop, T& rightop, relopId& relop)
{
	if (relop == relopId::RelopEqual)
	{
		if (leftop == rightop)
		{
			return true;
		}
	}
	else if (relop == relopId::RelopLess)
	{
		if (leftop < rightop)
		{
			return true;
		}
	}
	else if (relop == relopId::RelopGreater)
	{
		if (leftop > rightop)
		{
			return true;
		}
	}
	else if (relop == relopId::RelopLessEqual)
	{
		if (leftop <= rightop)
		{
			return true;
		}
	}
	else if (relop == relopId::RelopGreaterEqual)
	{
		if (leftop >= rightop)
		{
			return true;
		}
	}
	else if (relop == relopId::RelopNotEqual)
	{
		if (leftop != rightop)
		{
			return true;
		}
	}

	return false;
}

template bool liaInterpreter::primitiveComparison<int>(int& leftop,int& rightop, relopId& relop);

bool liaInterpreter::arrayComparison(std::vector<liaVariable> leftop, std::vector<liaVariable> rightop, relopId& relop)
{
	if (leftop.size() != rightop.size()) return false;

	int idx = 0;
	while (idx<leftop.size())
	{
		if (!primitiveComparison(leftop[idx].value, rightop[idx].value, relop))
		{
			return false;
		}
		idx += 1;
	}

	return true;
}

bool liaInterpreter::evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	//std::cout << "Parsing " << theAst->name << " ns: " << theAst->nodes.size() << std::endl;
	if ((theAst->iName == grammarElement::Condition)|| (theAst->iName == grammarElement::InnerCondition))
	{
		if (theAst->nodes.size() == 1)
		{
			//std::cout << "A lonely condition " << theAst->name << " ns: " << theAst->nodes.size() << std::endl;
			return evaluateCondition(theAst->nodes[0], env);
		}
		else if (theAst->nodes.size() == 3)
		{
			// [InnerCondition CondOperator Condition] or [Expression Relop Expression]
			//std::cout << "Size 3 " << theAst->name << " ns: " << theAst->nodes.size() << std::endl;
			if ((theAst->nodes[0]->iName == grammarElement::InnerCondition)|| (theAst->nodes[0]->iName == grammarElement::Condition))
			{
				std::string condOp;
				condOp += theAst->nodes[1]->token;
				//std::cout << condOp << std::endl;

				bool lCond = evaluateCondition(theAst->nodes[0], env);
				bool rCond = evaluateCondition(theAst->nodes[2], env);

				if (condOp == "&&")	return  (lCond&&rCond);
				else if (condOp=="||")	return (lCond||rCond);
				else
				{
					std::string err;
					err += "Unknown relational operator "+condOp+". ";
					err += "Terminating.";
					fatalError(err);
				}
			}
			else if (theAst->nodes[0]->iName == grammarElement::Expression)
			{
				relopId relOp=(relopId)theAst->nodes[1]->tokenId;

				liaVariable lExpr;
				evaluateExpression(theAst->nodes[0], env,lExpr);
				liaVariable rExpr;
				evaluateExpression(theAst->nodes[2], env,rExpr);

				if (lExpr.type != rExpr.type)
				{
					size_t line = theAst->nodes[0]->line;
					std::string err;
					err += "Comparing variables of different types ("+std::to_string(lExpr.type)+"-"+ std::to_string(rExpr.type) +") at line "+std::to_string(line) + " is forbidden. ";
					err += "Terminating.";
					fatalError(err);
				}

				if ((lExpr.type == liaVariableType::array) && (rExpr.type == liaVariableType::array))
				{
					return arrayComparison(lExpr.vlist, rExpr.vlist, relOp);
				}

				return primitiveComparison(lExpr.value, rExpr.value, relOp);
			}
		}
		else
		{
			std::string err;
			err += "Unhandled expression node scenario. ";
			err += "Terminating.";
			fatalError(err);
		}
	}
	else
	{
		if ((theAst->nodes.size() == 1) && (theAst->iName == grammarElement::Expression))
		{
			//std::cout << "Something else " << theAst->name << " ns: " << theAst->nodes.size() << std::endl;
			liaVariable exprz;
			evaluateExpression(theAst->nodes[0], env,exprz);
			if (exprz.type == liaVariableType::boolean)
			{
				return (std::get<bool>(exprz.value));
			}
			else if (exprz.type == liaVariableType::integer)
			{
				int varval = std::get<int>(exprz.value);
				if (varval == 0) return false;
				else return true;
			}
			else
			{
				std::string err;
				err += "Unsupported expression type evaluation. ";
				err += "Terminating.";
				fatalError(err);
			}
		}
	}

	return false;
}

bool liaInterpreter::exeCuteWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,liaVariable& retVal)
{
	//std::cout << peg::ast_to_s(theAst);

	while (evaluateCondition(theAst->nodes[0], env) == true)
	{
		if (exeCuteCodeBlock(theAst->nodes[1], env, retVal))
		{
			return true;
		}
	}

	return false;
}

bool liaInterpreter::exeCuteForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,liaVariable& retVal)
{
	//std::cout << peg::ast_to_s(theAst);
	assert(theAst->nodes.size() == 3);

	std::string tmpVarname;
	tmpVarname = theAst->nodes[0]->token;

	liaVariable iterated;
	evaluateExpression(theAst->nodes[1], env,iterated);

	//liaVariable* pArr = &env->varMap[arrVarname];
	
	if (iterated.type == liaVariableType::array)
	{
		size_t vlsize = iterated.vlist.size();
		if (vlsize > 0)
		{
			liaVariable tmpVar;

			for (int idx = 0;idx < vlsize;idx++)
			{
				tmpVar.type = iterated.vlist[idx].type;
				tmpVar.value = iterated.vlist[idx].value;
				tmpVar.vlist = iterated.vlist[idx].vlist;

				env->varMap[tmpVarname] = tmpVar;

				if (exeCuteCodeBlock(theAst->nodes[2], env, retVal))
				{
					env->varMap.erase(tmpVarname);
					return true;
				}
			}

			// remove temporary variable from scope
			env->varMap.erase(tmpVarname);
		}
	}
	else if (iterated.type == liaVariableType::string)
	{
		std::string s;
		s = std::get<std::string>(iterated.value);
		if (s.size() > 0)
		{
			liaVariable tmpVar;
			tmpVar.type = liaVariableType::string;

			for (int idx = 0;idx < s.size();idx++)
			{
				char curchar=s.at(idx);
				tmpVar.value = std::string(1,curchar);
				env->varMap[tmpVarname] = tmpVar;

				if (exeCuteCodeBlock(theAst->nodes[2], env, retVal))
				{
					env->varMap.erase(tmpVarname);
					return true;
				}
			}

			// remove temporary variable from scope
			env->varMap.erase(tmpVarname);
		}
	}
	else
	{
		size_t lineNum = theAst->line;
		std::string err;
		err += "Unknown foreach iterated type at line "+std::to_string(lineNum)+". ";
		err += "Terminating.";
		fatalError(err);
	}

	return false;
}

bool liaInterpreter::exeCuteIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,liaVariable& retVar)
{
	if (theAst->nodes.size() == 2)
	{
		// simple if
		if (evaluateCondition(theAst->nodes[0], env) == true)
		{
			if (exeCuteCodeBlock(theAst->nodes[1], env, retVar))
			{
				return true;
			}
		}
	}
	else if (theAst->nodes.size() == 3)
	{
		// if/else
		if (evaluateCondition(theAst->nodes[0], env) == true)
		{
			if (exeCuteCodeBlock(theAst->nodes[1], env, retVar))
			{
				return true;
			}
		}
		else
		{
			if (exeCuteCodeBlock(theAst->nodes[2], env, retVar))
			{
				return true;
			}
		}
	}

	return false;
}

bool liaInterpreter::exeCuteCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env,liaVariable& retVal)
{
	for (auto& stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
		//std::cout << stmt->name << std::endl;
		const int nodeId = stmt->nodes[0]->iName;
		
		if (nodeId == grammarElement::FuncCallStmt)
		{
			// user function or library function call
			liaVariable dummyVar;
			exeCuteFuncCallStatement(stmt->nodes[0],env,dummyVar);
		}
		else if (nodeId == grammarElement::WhileStmt)
		{
			// while statement, the cradle of all infinite loops
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			if (exeCuteWhileStatement(stmt->nodes[0], env, retVal))
			{
				return true;
			}
		}
		else if (nodeId == grammarElement::ForeachStmt)
		{
			if (exeCuteForeachStatement(stmt->nodes[0], env,retVal))
			{
				return true;
			}
		}
		else if (nodeId == grammarElement::IfStmt)
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			if (exeCuteIfStatement(stmt->nodes[0], env,retVal))
			{
				return true;
			}
		}
		else if (nodeId == grammarElement::ArrayAssignmentStmt)
		{
			// assignment to array element
			exeCuteArrayAssignmentStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::VarFuncCallStmt)
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			std::string variableName;
			variableName += stmt->nodes[0]->nodes[0]->token;
			liaVariable dummyVar;
			exeCuteMethodCallStatement(stmt->nodes[0], env, variableName,dummyVar);
		}
		else if (nodeId == grammarElement::VarDeclStmt)
		{
			// variable declaration/assignment
			//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
			exeCuteVarDeclStatement(stmt->nodes[0],env);
		}
		else if (nodeId == grammarElement::IncrementStmt)
		{
			exeCuteIncrementStatement(stmt->nodes[0],env,1);
		}
		else if (nodeId == grammarElement::DecrementStmt)
		{
			exeCuteIncrementStatement(stmt->nodes[0], env, -1);
		}
		else if (nodeId == grammarElement::RshiftStmt)
		{
			exeCuteRshiftStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::LshiftStmt)
		{
			exeCuteLshiftStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::ModuloStmt)
		{
			exeCuteModuloStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::LogicalAndStmt)
		{
			exeCuteLogicalAndStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::LogicalOrStmt)
		{
			exeCuteLogicalOrStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::MultiplyStmt)
		{
			exeCuteMultiplyStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::DivideStmt)
		{
			exeCuteDivideStatement(stmt->nodes[0], env);
		}
		else if (nodeId == grammarElement::ReturnStmt)
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);

			if (stmt->nodes[0]->nodes.size() != 0)
			{
				// return <expression>
				evaluateExpression(stmt->nodes[0]->nodes[0], env, retVal);
			}

			return true;
		}
	}

	return false;
}

int liaInterpreter::storeGlobalVariables(std::shared_ptr<peg::Ast> theAst)
{
	for (auto node : theAst->nodes)
	{
		if (node->iName == grammarElement::TopLevelStmt)
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->iName == grammarElement::GlobalVarDecl)
				{
					liaVariable glbVar;
					//std::cout << peg::ast_to_s(innerNode);

					liaVariable expr;
					evaluateExpression(innerNode->nodes[1], &globalScope,expr);

					glbVar.name += innerNode->nodes[0]->token;

					if (glbVar.name.find("glb") != 0)
					{
						std::cout << "Error: global variables should start with 'glb' at line " << std::to_string(innerNode->line) << "." << std::endl;
						return 1;
					}

					glbVar.type = expr.type;
					glbVar.value = expr.value;
					glbVar.vlist = expr.vlist;
					glbVar.vMap = expr.vMap;

					globalScope.varMap[glbVar.name] = glbVar;
				}
			}
		}
	}

	return 0;
}

// where the Cuteness starts
void liaInterpreter::exeCute(const std::shared_ptr<peg::Ast>& theAst,std::vector<std::string> params)
{
	// basically, we have to find the "main" codeblock and exeCute it
	// in the mean time, we can create "environments" (scopes), that is list of variables, with types and values
	// there should be a global scope, I guess. Even if global variables suck
	// UPDATE: at the moment we don't handle the global scope
	// UPDATE2: yes, we do

	liaEnvironment curEnv;

	liaFunction f = functionMap["main"];
	// inject "params" into the environment
	liaVariable parms;
			
	parms.name = "params";
	parms.type = liaVariableType::array;
	for (std::string s: params)
	{
		liaVariable p;
		p.type = liaVariableType::string;
		p.value = s;
		parms.vlist.push_back(p);
	}
	addvarOrUpdateEnvironment(&parms, &curEnv, 0);

	//std::cout << "Executing main" << std::endl;
	//std::cout << peg::ast_to_s(f.functionCodeBlockAst);
	assert(f.functionCodeBlockAst->iName == grammarElement::CodeBlock);

	// execute each statement in code block
	liaVariable dummRetVal;
	exeCuteCodeBlock(f.functionCodeBlockAst,&curEnv, dummRetVal);
}

liaInterpreter::~liaInterpreter()
{
}
