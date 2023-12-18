
//
// the interpreter
// liaiason - project for the hard times
// (c) friol 2k23
//

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
		if (node->name == "TopLevelStmt")
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->name == "FuncDeclStmt")
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
							if (funcNode->name == "FuncParamList")
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
		if (node->name == "TopLevelStmt")
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->name == "FuncDeclStmt")
				{
					liaFunction aFun;
					for (auto funcNode : innerNode->nodes)
					{
						if (funcNode->is_token)
						{
							aFun.name = funcNode->token;
							for (auto el : innerNode->nodes)
							{
								if (el->name == "CodeBlock")
								{
									aFun.functionCodeBlockAst = el;
								}
							}
						}
						else
						{
							if (funcNode->name == "FuncParamList")
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

					functionList.push_back(aFun);
				}
			}
		}
	}
}

void liaInterpreter::dumpFunctions()
{
	for (liaFunction f: functionList)
	{
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

liaVariable liaInterpreter::exeCuteMethodCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env, std::string varName)
{
	liaVariable retVal;
	std::vector<liaVariable> parameters;

	size_t lineNum = theAst->line;

	// get variable value
	liaVariable* pvarValue=nullptr;

	if (env->varMap.find(varName) == env->varMap.end())
	{
		if (globalScope.varMap.find(varName) == globalScope.varMap.end())
		{
			// variable name not found
			std::string err;
			err += "Variable [" + varName + "] not found in method call at line " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvarValue = &globalScope.varMap[varName];
		}
	}
	else
	{
		pvarValue = &env->varMap[varName];
	}

	for (auto& ch : theAst->nodes)
	{
		if (!ch->is_token)
		{
			if (ch->name == "ArgList")
			{
				for (auto& expr : ch->nodes)
				{
					assert(expr->name == "Expression");
					liaVariable arg = evaluateExpression(expr, env);
					parameters.push_back(arg);
				}
			}
		}
	}

	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "FuncName")
			{
				if (ch->token == "split")
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
				else if (ch->token == "add")
				{
					// add element to an array
					assert(pvarValue->type == liaVariableType::array);
					assert(parameters.size() == 1);

					pvarValue->vlist.push_back(parameters[0]);
				}
				else if (ch->token == "find")
				{
					// s.find("value"), returns idx of found element, or -1 if not found
					//assert(pvarValue->type == liaVariableType::array);
					assert(parameters.size() == 1);

					retVal.type = liaVariableType::integer;
					retVal.value = -1;

					if (parameters[0].type == liaVariableType::string)
					{
						std::string val2find;
						val2find += std::get<std::string>(parameters[0].value);

						if (pvarValue->type == liaVariableType::array)
						{
							for (int idx = 0;idx < pvarValue->vlist.size();idx++)
							{
								if (std::get<std::string>(pvarValue->vlist[idx].value) == val2find)
								{
									retVal.value = idx;
									return retVal;
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

							return retVal;
						}
					}
					else if (parameters[0].type == liaVariableType::integer)
					{
						int val2find = std::get<int>(parameters[0].value);
						if (pvarValue->vlist.size() > 0)
						{
							assert(pvarValue->vlist[0].type == liaVariableType::integer);

							for (int idx = 0;idx < pvarValue->vlist.size();idx++)
							{
								if (std::get<int>(pvarValue->vlist[idx].value) == val2find)
								{
									retVal.value = idx;
									return retVal;
								}
							}
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
									return retVal;
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
								return retVal;
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
				else if (ch->token == "replace")
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
					return retVal;
				}
				else if (ch->token == "slice")
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
							return retVal;
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
						return retVal;
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

						return retVal;
					}
				}
				else if (ch->token == "sort")
				{
					// sort monodimensional array
					assert(pvarValue->type == liaVariableType::array);
					assert(parameters.size() == 0);

					std::sort(pvarValue->vlist.begin(), pvarValue->vlist.end(), compareIntVars);
				}
				else if (ch->token == "clear")
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
					std::string m;
					m += ch->token;
					std::string err;
					err += "Method name [" + m + "] not found. ";
					err += "Terminating.";
					fatalError(err);
				}
			}
		}
	}

	return retVal;
}

liaVariable liaInterpreter::evaluateExpression(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable retVar;
	size_t lineNum = theAst->nodes[0]->line;

	if (theAst->nodes[0]->name == "StringLiteral")
	{
		std::string stringVal;
		stringVal += theAst->nodes[0]->token;
		std::regex quote_re("\"");

		retVar.type = liaVariableType::string;
		retVar.value = std::regex_replace(stringVal, quote_re, "");
	}
	else if (theAst->nodes[0]->name == "IntegerNumber")
	{
		std::string tmp;
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::integer;
		retVar.value = std::stoi(tmp);
	}
	else if (theAst->nodes[0]->name == "LongNumber")
	{
		std::string tmp;
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::longint;
		retVar.value = std::stoll(tmp);
	}
	else if (theAst->nodes[0]->name == "BooleanConst")
	{
		std::string tmp;
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::boolean;
		if (tmp=="true") retVar.value = true;
		else retVar.value = false;
	}
	else if (theAst->nodes[0]->name == "BitwiseNot")
	{
		//std::cout << "NOT/RJ" << peg::ast_to_s(theAst);

		assert(theAst->nodes[0]->nodes.size() == 1);
		assert(theAst->nodes[0]->nodes[0]->name == "Expression");

		liaVariable expr = evaluateExpression(theAst->nodes[0]->nodes[0], env);
		assert(expr.type == liaVariableType::integer);

		retVar.type = liaVariableType::integer;
		retVar.value = ~std::get<int>(expr.value);
	}
	else if (theAst->nodes[0]->name == "VariableName")
	{
		std::string varName;
		varName += theAst->nodes[0]->token;

		liaVariable* v=nullptr;
		if (env->varMap.find(varName) == env->varMap.end())
		{
			if (globalScope.varMap.find(varName) == globalScope.varMap.end())
			{
				// variable name not found
				std::string err;
				err += "Variable [" + varName + "] not found at line " + std::to_string(lineNum) + ". ";
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

		if (v->type == liaVariableType::integer)
		{
			int val2print = std::get<int>(v->value);
			retVar.type = liaVariableType::integer;
			retVar.value=val2print;
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
	else if (theAst->nodes[0]->name == "VariableWithFunction")
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName;
		varName += theAst->nodes[0]->nodes[0]->token;

		liaVariable* pvarValue = nullptr;

		if (env->varMap.find(varName) == env->varMap.end())
		{
			if (globalScope.varMap.find(varName) == globalScope.varMap.end())
			{
				// variable name not found
				std::string err;
				err += "Variable [" + varName + "] not found in method call at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
		}

		retVar=exeCuteMethodCallStatement(theAst->nodes[0], env,varName);
	}
	else if (theAst->nodes[0]->name == "VariableWithProperty")
	{
		std::string varName;
		varName += theAst->nodes[0]->nodes[0]->token;
		std::string varProp;
		varProp += theAst->nodes[0]->nodes[1]->token;

		liaVariable* v = nullptr;
		if (env->varMap.find(varName) == env->varMap.end())
		{
			if (globalScope.varMap.find(varName) == globalScope.varMap.end())
			{
				// variable name not found
				std::string err;
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
	else if (theAst->nodes[0]->name == "ArrayInitializer")
	{
		retVar.type = liaVariableType::array;
		if (theAst->nodes[0]->nodes.size() != 0)
		{
			assert(theAst->nodes[0]->nodes[0]->name == "ExpressionList");

			for (auto& el : theAst->nodes[0]->nodes[0]->nodes)
			{
				liaVariable varel = evaluateExpression(el,env);
				retVar.vlist.push_back(varel);
			}
		}
	}
	else if (theAst->nodes[0]->name == "ArraySubscript")
	{
		//std::cout << peg::ast_to_s(theAst->nodes[0]);
		size_t lineNum = theAst->nodes[0]->line;

		std::string arrName;
		arrName += theAst->nodes[0]->nodes[0]->token;

		std::vector<liaVariable> vIdxArr;
		for (auto& node : theAst->nodes[0]->nodes)
		{
			if (node->name == "Expression")
			{
				liaVariable vIdx = evaluateExpression(node, env);
				vIdxArr.push_back(vIdx);
			}
		}

		liaVariable* pVar = nullptr;
		if (env->varMap.find(arrName) == env->varMap.end())
		{
			if (globalScope.varMap.find(arrName) == globalScope.varMap.end())
			{
				// variable name not found
				std::string err;
				err += "Variable [" + arrName + "] not found in method call at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
			else
			{
				pVar = &globalScope.varMap[arrName];
			}
		}
		else
		{
			pVar = &env->varMap[arrName];
		}

		if (pVar->type == liaVariableType::array)
		{
			liaVariable* pArr = pVar;

			for (auto& idx : vIdxArr)
			{
				int arrIdx = std::get<int>(idx.value);

				if (arrIdx >= pArr->vlist.size())
				{
					std::string err;
					err += "Array index out of range at " + std::to_string(lineNum) + ". ";
					err += "Terminating.";
					fatalError(err);
				}

				pArr = &pArr->vlist[arrIdx];
			}

			retVar.type = pArr->type;
			retVar.value = pArr->value;
			retVar.vlist = pArr->vlist;
		}
		else if (pVar->type == liaVariableType::dictionary)
		{
			if (vIdxArr.size() != 1)
			{
				std::string err;
				err += "Dictionaries can only be indexed once. ";
				err += "Terminating.";
				fatalError(err);
			}

			std::string key = std::get<std::string>(vIdxArr[0].value);
			retVar.type = pVar->vMap[key].type;
			retVar.value = pVar->vMap[key].value;
		}
		else if (pVar->type == liaVariableType::string)
		{
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
	else if (theAst->nodes[0]->name == "RFuncCall")
	{
		//std::cout << "funCall" << std::endl;
		//std::cout << peg::ast_to_s(theAst);
		retVar=exeCuteFuncCallStatement(theAst->nodes[0], env);
	}
	else if (theAst->nodes[0]->name == "DictInitializer")
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
					curVar = evaluateExpression(kv, env);
					retVar.vMap[curKey] = curVar;
				}
			}
		}
	}
	else if ( (theAst->name=="Expression") && ((theAst->nodes.size() % 2) == 1) && (theAst->nodes.size()>2))
	{
		int nodepos = 0;

		liaVariable vResult = evaluateExpression(theAst->nodes[nodepos], env);
		nodepos += 1;

		while (nodepos < theAst->nodes.size())
		{
			std::string opz;
			opz += theAst->nodes[nodepos]->token;

			liaVariable v1= evaluateExpression(theAst->nodes[nodepos+1], env);

			if (vResult.type != v1.type)
			{
				std::string err;
				err += "Only expressions of elements with the same type are supported at the moment at line " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			if ((vResult.type != liaVariableType::integer)&& (vResult.type != liaVariableType::longint))
			{
				std::string err;
				err += "Only expressions of elements with type integer/long are supported at the moment " + std::to_string(lineNum) + ". ";
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

				vResult.value = partialResult;
			}

			nodepos += 2;
		}

		return vResult;
	}
	else if ((theAst->name == "InnerExpression") || (theAst->name == "Expression"))
	{
		return evaluateExpression(theAst->nodes[0], env);
	}

	return retVar;
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

			/*if (el.type == liaVariableType::string)
			{
				std::visit([](const auto& x) { std::cout << "\"" << x << "\""; }, el.value);
			}
			else
			{
				std::visit([](const auto& x) { std::cout << x; }, el.value);
			}*/
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
		if (var.type == liaVariableType::string)
		{
			std::string vv;
			vv += std::get<std::string>(var.value);
		}

		std::visit([](const auto& x) { std::cout << x; }, var.value);
		//std::cout << " ";
	}


}

void liaInterpreter::exeCuteLibFunctionPrint(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);
	assert(theAst->name == "ArgList");
	assert(theAst->nodes[0]->name == "Expression");

	for (auto node : theAst->nodes)
	{
		liaVariable retVar = evaluateExpression(node, env);
		innerPrint(retVar);
	}

	std::cout << std::endl;
}

liaVariable liaInterpreter::exeCuteLibFunctionReadFile(std::string fname,int linenum)
{
	liaVariable retVar;
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

	return retVar;
}

liaVariable liaInterpreter::customFunctionCall(std::string& fname, std::vector<liaVariable>* parameters, liaEnvironment* env,size_t lineNum)
{
	liaVariable retVal;

	// check if function name exists
	bool funFound = false;
	std::shared_ptr<peg::Ast> pBlock;

	for (auto f : functionList)
	{
		if (fname == f.name)
		{
			funFound = true;
			pBlock = f.functionCodeBlockAst;

			// check that number of parameters is the same of function call

			if (f.parameters.size() != parameters->size())
			{
				std::string err;
				err += "Function " + fname + " called with wrong number of parameters. "+std::to_string(parameters->size())+" parameter(s) passed ";
				err += "instead of "+std::to_string(f.parameters.size()) + ".";
				err += "Terminating.";
				fatalError(err);
			}

			for (int p = 0;p < parameters->size();p++)
			{
				(*parameters)[p].name = f.parameters[p].name;
			}
		}
	}

	if (funFound != true)
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
	for (auto parm : *parameters)
	{
		//std::cout << parm.name << std::endl;
		addvarOrUpdateEnvironment(&parm, &funEvn, 0);
	}

	retVal=exeCuteCodeBlock(pBlock, &funEvn);

	return retVal;
}

liaVariable liaInterpreter::exeCuteFuncCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable retVal;
	std::vector<liaVariable> parameters;
	size_t lineNum=theAst->line;

	for (auto& ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "FuncName")
			{
				if (ch->token == "print")
				{
					exeCuteLibFunctionPrint(theAst->nodes[1],env);
				}
				else if (ch->token == "readTextFileLineByLine")
				{
					assert(theAst->nodes[1]->name == "ArgList");
					assert(theAst->nodes[1]->nodes[0]->name == "Expression");
					int linenum = (int)theAst->nodes[1]->nodes[0]->line;
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0],env);
					retVal=exeCuteLibFunctionReadFile(std::get<std::string>(p0.value),linenum);
				}
				else if ((ch->token == "toInteger")|| (ch->token == "toLong"))
				{
					assert(theAst->nodes[1]->name == "ArgList");
					assert(theAst->nodes[1]->nodes[0]->name == "Expression");
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);
					
					// parameter to convert must be a string
					if (p0.type != liaVariableType::string)
					{
						std::string err;
						err += "toInteger/toLong accepts only string values. ";
						err += "Terminating.";
						fatalError(err);
					}

					if (ch->token == "toInteger")
					{
						retVal.type = liaVariableType::integer;
					}
					else if (ch->token == "toLong")
					{
						retVal.type = liaVariableType::longint;
					}

					std::string sVal = std::get<std::string>(p0.value);

					try
					{
						if (ch->token == "toInteger")
						{
							retVal.value = std::stoi(sVal);
						}
						else if (ch->token == "toLong")
						{
							retVal.value = std::stoll(sVal);
						}
					}
					catch (...)
					{
						// unable to convert to integer
						std::string err;
						err += "Can't convert [" + sVal + "] to integer/long at line "+std::to_string(lineNum)+". ";
						err += "Terminating.";
						fatalError(err);
					}

					//std::visit([](const auto& x) { std::cout << x; }, p0.value);
					//retVal = exeCuteLibFunctionReadFile(std::get<std::string>(p0.value));
				}
				else if (ch->token == "toString")
				{
					assert(theAst->nodes[1]->name == "ArgList");
					assert(theAst->nodes[1]->nodes[0]->name == "Expression");
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);

					// parameter to convert must be an integer
					if (p0.type != liaVariableType::integer)
					{
						std::string err;
						err += "Parameter to toString should be an integer. ";
						err += "Terminating.";
						fatalError(err);
					}

					retVal.type = liaVariableType::string;
					int sVal = std::get<int>(p0.value);
					retVal.value = std::to_string(sVal);
				}
				else if (ch->token == "ord")
				{
					// char to its ascii code
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);

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
				else if (ch->token == "chr")
				{
					// the opposite
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);
					
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
				else if (ch->token == "lSqrt")
				{
					// integer sqrt
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);

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
				else if (ch->token == "rnd")
				{
					// generate a random number from zero to parameter 0
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);

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
				else if (ch->token == "getMillisecondsSinceEpoch")
				{
					// another function with an infinite name
					long long now =(long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
					//std::cout << now << std::endl;

					retVal.type = liaVariableType::longint;
					retVal.value = now;
				}
				else
				{
					// should be a custom function
					// evaluate and store the parameters
					for (auto ch : theAst->nodes)
					{
						if (!ch->is_token)
						{
							if (ch->name == "ArgList")
							{
								for (auto expr : ch->nodes)
								{
									assert(expr->name == "Expression");
									liaVariable arg = evaluateExpression(expr, env);
									parameters.push_back(arg);
								}
							}
						}
					}

					std::string funname;
					funname += ch->token;
					retVal = customFunctionCall(funname,&parameters,env,lineNum);
				}
			}
		}
	}

	return retVal;
}

void liaInterpreter::exeCuteVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
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
				liaVariable retVar = evaluateExpression(ch, env);
				theVar.type = retVar.type;
				theVar.value = retVar.value;
				theVar.vlist = retVar.vlist;
				theVar.vMap = retVar.vMap;
			}
		}
	}

	// now, if variable is not in env, create it. otherwise, update it
	addvarOrUpdateEnvironment(&theVar, env, curLine);
}

void liaInterpreter::exeCuteArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	assert(theAst->nodes[0]->name=="ArraySubscript");
	assert(theAst->nodes[1]->name=="Expression");

	size_t lineNum = theAst->nodes[0]->line;

	liaVariable value2assign = evaluateExpression(theAst->nodes[1], env);
	std::string arrayName;
	arrayName+=theAst->nodes[0]->nodes[0]->token;

	liaVariable* pArr = nullptr;
	if (globalScope.varMap.find(arrayName) == globalScope.varMap.end())
	{
		if (env->varMap.find(arrayName) == env->varMap.end())
		{
			std::string err;
			err += "Array \"" + arrayName + "\" not found at line " + std::to_string(lineNum) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pArr = &env->varMap[arrayName];
		}
	}
	else
	{
		pArr = &globalScope.varMap[arrayName];
	}

	liaVariable arrayIndex = evaluateExpression(theAst->nodes[0]->nodes[1], env);

	if (pArr->type == liaVariableType::array)
	{
		std::vector<liaVariable> vIdxArr;
		for (auto& node : theAst->nodes[0]->nodes)
		{
			if (node->name == "Expression")
			{
				liaVariable vIdx = evaluateExpression(node, env);
				vIdxArr.push_back(vIdx);
			}
		}

		for (auto& idx : vIdxArr)
		{
			int arrIdx = std::get<int>(idx.value);

			if (arrIdx >= pArr->vlist.size())
			{
				std::string err;
				err += "Array index out of range at " + std::to_string(lineNum) + ". ";
				err += "Terminating.";
				fatalError(err);
			}

			pArr = &pArr->vlist[arrIdx];
		}

		assert(arrayIndex.type == liaVariableType::integer);
		//assert(std::get<int>(arrayIndex.value) < pArr->vlist.size());

		pArr->value = value2assign.value;
		pArr->vlist = value2assign.vlist;
	}
	else if (pArr->type == liaVariableType::dictionary)
	{
		assert(arrayIndex.type == liaVariableType::string);
		pArr->vMap[std::get<std::string>(arrayIndex.value)] = value2assign;
	}
	else if (pArr->type == liaVariableType::string)
	{
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
	int mulAmount = 0;

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
				liaVariable vInc = evaluateExpression(ch, env);
				if (vInc.type!=liaVariableType::integer)
				{
					std::string err;
					err += "Multiply term should be an integer. ";
					err += "Terminating.";
					fatalError(err);
				}
				mulAmount = std::get<int>(vInc.value);
			}
		}
	}

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv*=mulAmount;
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
				liaVariable vInc = evaluateExpression(ch, env);
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

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

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
				liaVariable vInc = evaluateExpression(ch, env);
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

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

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
				liaVariable andVal = evaluateExpression(ch, env);
				assert(andVal.type == liaVariableType::integer);
				rExpr = std::get<int>(andVal.value);
			}
		}
	}

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

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
				liaVariable andVal = evaluateExpression(ch, env);
				assert(andVal.type == liaVariableType::integer);
				rExpr = std::get<int>(andVal.value);
			}
		}
	}

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

	if (pvar->type == liaVariableType::integer)
	{
		int vv = std::get<int>(pvar->value);
		pvar->value = vv | rExpr;
	}
	else
	{
		std::string err;
		err += "Logical or is valid only between integers";
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
	int rshiftAmount = 0;

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
				liaVariable vInc = evaluateExpression(ch, env);
				assert(vInc.type == liaVariableType::integer);
				rshiftAmount = std::get<int>(vInc.value);
			}
		}
	}

	if (rshiftAmount != 0)
	{
		liaVariable* pvar = nullptr;
		if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
		{
			if (env->varMap.find(theVar.name) == env->varMap.end())
			{
				std::string err;
				err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
			else
			{
				pvar = &env->varMap[theVar.name];
			}
		}
		else
		{
			pvar = &globalScope.varMap[theVar.name];
		}

		if (pvar->type == liaVariableType::integer)
		{
			int vv = std::get<int>(pvar->value);
			pvar->value = vv>>rshiftAmount;
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
				liaVariable vInc = evaluateExpression(ch, env);
				assert(vInc.type == liaVariableType::integer);
				lshiftAmount = std::get<int>(vInc.value);
			}
		}
	}

	if (lshiftAmount != 0)
	{
		liaVariable* pvar = nullptr;
		if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
		{
			if (env->varMap.find(theVar.name) == env->varMap.end())
			{
				std::string err;
				err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
				err += "Terminating.";
				fatalError(err);
			}
			else
			{
				pvar = &env->varMap[theVar.name];
			}
		}
		else
		{
			pvar = &globalScope.varMap[theVar.name];
		}

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

void liaInterpreter::exeCuteIncrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env,int inc)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine=0;
	liaVariable theInc;
	liaVariable dictKey;
	bool isSubscript = false;

	for (auto& ch : theAst->nodes)
	{
		if (ch->name == "ArraySubscript")
		{
			theVar.name += ch->nodes[0]->token;
			dictKey = evaluateExpression(ch->nodes[1], env);
			//std::cout << theVar.name << " " << std::get<std::string>(dictKey.value);
			isSubscript = true;
		}

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
				theInc = evaluateExpression(ch,env);
				curLine = ch->line;
			}
		}
	}

	liaVariable* pvar = nullptr;
	if (globalScope.varMap.find(theVar.name) == globalScope.varMap.end())
	{
		if (env->varMap.find(theVar.name) == env->varMap.end())
		{
			std::string err;
			err += "Variable \"" + theVar.name + "\" not found at line " + std::to_string(curLine) + ". ";
			err += "Terminating.";
			fatalError(err);
		}
		else
		{
			pvar = &env->varMap[theVar.name];
		}
	}
	else
	{
		pvar = &globalScope.varMap[theVar.name];
	}

	if ((pvar->type == liaVariableType::dictionary) && (!isSubscript))
	{
		std::string err;
		err += "You can't increment a dictionary at line " + std::to_string(curLine) + ". ";
		err += "Terminating.";
		fatalError(err);
	}

	/*if ((pvar->type == liaVariableType::array) && (!isSubscript))
	{
		std::string err;
		err += "You can't increment an array at line " + std::to_string(curLine) + ". ";
		err += "Terminating.";
		fatalError(err);
	}*/

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
	liaVariable* pVar=nullptr;
	
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
	else
	{
		std::string err;
		err += "Unhandled type for scope variable update at " + std::to_string(curLine) + ". ";
		err += "Terminating.";
		fatalError(err);
	}
}

template <typename T> bool liaInterpreter::primitiveComparison(T leftop, T rightop, std::string relOp)
{
	if (relOp == "==")
	{
		if (leftop == rightop)
		{
			return true;
		}
	}
	else if (relOp == "<")
	{
		if (leftop < rightop)
		{
			return true;
		}
	}
	else if (relOp == ">")
	{
		if (leftop > rightop)
		{
			return true;
		}
	}
	else if (relOp == "<=")
	{
		if (leftop <= rightop)
		{
			return true;
		}
	}
	else if (relOp == ">=")
	{
		if (leftop >= rightop)
		{
			return true;
		}
	}
	else if (relOp == "!=")
	{
		if (leftop != rightop)
		{
			return true;
		}
	}

	return false;
}

template bool liaInterpreter::primitiveComparison<int>(int leftop,int rightop,std::string relOp);

bool liaInterpreter::arrayComparison(std::vector<liaVariable> leftop, std::vector<liaVariable> rightop, std::string relOp)
{
	if (leftop.size() != rightop.size()) return false;

	int idx = 0;
	while (idx<leftop.size())
	{
		if (!primitiveComparison(leftop[idx].value, rightop[idx].value, relOp))
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
	if ((theAst->name == "Condition")|| (theAst->name == "InnerCondition"))
	{
		if (theAst->nodes.size() == 1)
		{
			return evaluateCondition(theAst->nodes[0], env);
		}
		else if (theAst->nodes.size() == 3)
		{
			// [InnerCondition CondOperator Condition] or [Expression Relop Expression]
			if ((theAst->nodes[0]->name == "InnerCondition")|| (theAst->nodes[0]->name == "Condition"))
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
			else if (theAst->nodes[0]->name == "Expression")
			{
				std::string relOp;
				relOp += theAst->nodes[1]->token;

				liaVariable lExpr = evaluateExpression(theAst->nodes[0], env);
				liaVariable rExpr = evaluateExpression(theAst->nodes[2], env);

				size_t line = theAst->nodes[0]->line;

				if (lExpr.type != rExpr.type)
				{
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

	return false;
}

liaVariable liaInterpreter::exeCuteWhileStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
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
	
	liaVariable retVal;
	while (evaluateCondition(pCond, env) == true)
	{
		retVal=exeCuteCodeBlock(pBlock, env);
		if (retVal.name == "lia-ret-val")
		{
			return retVal;
		}
	}

	return retVal;
}

liaVariable liaInterpreter::exeCuteForeachStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
{
	liaVariable retVal;
	//std::cout << peg::ast_to_s(theAst);
	size_t lineNum = theAst->line;

	//assert(theAst->nodes.size() == 3);

	std::shared_ptr<peg::Ast> pBlock;

	for (auto& ch : theAst->nodes)
	{
		if (ch->name == "CodeBlock")
		{
			pBlock = ch;
		}
	}

	std::string tmpVarname;
	tmpVarname = theAst->nodes[0]->token;

	liaVariable iterated = evaluateExpression(theAst->nodes[1], env);

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

				retVal = exeCuteCodeBlock(pBlock, env);
				if (retVal.name == "lia-ret-val")
				{
					env->varMap.erase(tmpVarname);
					return retVal;
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

				retVal = exeCuteCodeBlock(pBlock, env);
				if (retVal.name == "lia-ret-val")
				{
					env->varMap.erase(tmpVarname);
					return retVal;
				}
			}

			// remove temporary variable from scope
			env->varMap.erase(tmpVarname);
		}
	}
	else
	{
		std::string err;
		err += "Unknown foreach iterated at line "+std::to_string(lineNum)+". ";
		err += "Terminating.";
		fatalError(err);
	}

	return retVal;
}

liaVariable liaInterpreter::exeCuteIfStatement(const std::shared_ptr<peg::Ast>& theAst, liaEnvironment* env)
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
		if (evaluateCondition(pCond, env) == true)
		{
			retVar=exeCuteCodeBlock(pBlock, env);
			if (retVar.name == "lia-ret-val")
			{
				return retVar;
			}
		}
	}
	else if (theAst->nodes.size() == 3)
	{
		// if/else
		pBlock2 = theAst->nodes[2];

		if (evaluateCondition(pCond, env) == true)
		{
			retVar=exeCuteCodeBlock(pBlock, env);
			if (retVar.name == "lia-ret-val")
			{
				return retVar;
			}
		}
		else
		{
			retVar=exeCuteCodeBlock(pBlock2, env);
			if (retVar.name == "lia-ret-val")
			{
				return retVar;
			}
		}
	}

	return retVar;
}

liaVariable liaInterpreter::exeCuteCodeBlock(const std::shared_ptr<peg::Ast>& theAst,liaEnvironment* env)
{
	liaVariable retVal;

	for (auto& stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
		
		if (stmt->nodes[0]->name == "FuncCallStmt")
		{
			// user function or library function call
			exeCuteFuncCallStatement(stmt->nodes[0],env);
		}
		else if (stmt->nodes[0]->name == "WhileStmt")
		{
			// while statement, the cradle of all infinite loops
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal = exeCuteWhileStatement(stmt->nodes[0], env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if (stmt->nodes[0]->name == "ForeachStmt")
		{
			retVal = exeCuteForeachStatement(stmt->nodes[0], env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if (stmt->nodes[0]->name == "IfStmt")
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal = exeCuteIfStatement(stmt->nodes[0], env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if (stmt->nodes[0]->name == "VarFuncCallStmt")
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			std::string variableName;
			variableName += stmt->nodes[0]->nodes[0]->token;
			exeCuteMethodCallStatement(stmt->nodes[0], env, variableName);
		}
		else if (stmt->nodes[0]->name == "VarDeclStmt")
		{
			// variable declaration/assignment
			//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
			exeCuteVarDeclStatement(stmt->nodes[0],env);
		}
		else if (stmt->nodes[0]->name == "ArrayAssignmentStmt")
		{
			// assignment to array element
			exeCuteArrayAssignmentStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes[0]->name == "IncrementStmt") || (stmt->nodes[0]->name == "DecrementStmt"))
		{
			int inc = 1;
			if (stmt->nodes[0]->name == "DecrementStmt") inc = -1;
			exeCuteIncrementStatement(stmt->nodes[0],env,inc);
		}
		else if (stmt->nodes[0]->name == "RshiftStmt")
		{
			exeCuteRshiftStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "LshiftStmt")
		{
			exeCuteLshiftStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "ModuloStmt")
		{
			exeCuteModuloStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "LogicalAndStmt")
		{
			exeCuteLogicalAndStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "LogicalOrStmt")
		{
			exeCuteLogicalOrStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "MultiplyStmt")
		{
			exeCuteMultiplyStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "DivideStmt")
		{
			exeCuteDivideStatement(stmt->nodes[0], env);
		}
		else if (stmt->nodes[0]->name == "ReturnStmt")
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal = evaluateExpression(stmt->nodes[0]->nodes[0], env);
			retVal.name = "lia-ret-val"; // gah
			return retVal;
		}
	}

	return retVal;
}

int liaInterpreter::storeGlobalVariables(std::shared_ptr<peg::Ast> theAst)
{
	for (auto node : theAst->nodes)
	{
		if (node->name == "TopLevelStmt")
		{
			for (auto innerNode : node->nodes)
			{
				if (innerNode->name == "GlobalVarDecl")
				{
					liaVariable glbVar;
					//std::cout << peg::ast_to_s(innerNode);

					liaVariable expr = evaluateExpression(innerNode->nodes[1], &globalScope);

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

	liaEnvironment curEnv;

	for (liaFunction f : functionList)
	{
		if (f.name == "main")
		{
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
			assert(f.functionCodeBlockAst->name == "CodeBlock");

			// execute each statement in code block
			exeCuteCodeBlock(f.functionCodeBlockAst,&curEnv);
		}
	}
}

liaInterpreter::~liaInterpreter()
{
}
