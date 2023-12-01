
//
// the interpreter
// liaiason - project for the hard times
// (c) friol 2k23
//

#include "interpreter.h"

liaInterpreter::liaInterpreter()
{
}

void liaInterpreter::fatalError(std::string err)
{
	std::cout << "Error: " << err << std::endl;
	exit(1);
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
		//std::cout << "Main function found, and it has the 'params' parameter; so that's good." << std::endl;
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

liaVariable liaInterpreter::exeCuteMethodCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env, std::string varName)
{
	liaVariable retVal;
	std::vector<liaVariable> parameters;

	// get variable value
	liaVariable* pvarValue=NULL;

	if (env->varMap.find(varName) == env->varMap.end())
	{
		// variable not found
		std::string err = "";
		err += "Variable name [" + varName + "] not found. ";
		err += "Terminating.";
		fatalError(err);
	}

	pvarValue = &env->varMap[varName];

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

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "FuncName")
			{
				if (ch->token == "split")
				{
					assert(pvarValue->type == liaVariableType::string);
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

					env->varMap[varName].vlist.push_back(parameters[0]);
				}
				else if (ch->token == "find")
				{
					// s.find("value"), returns idx of found element, or -1 if not found
					//assert(pvarValue->type == liaVariableType::array);
					//assert(parameters.size() == 1);

					retVal.type = liaVariableType::integer;
					retVal.value = -1;

					if (parameters[0].type == liaVariableType::string)
					{
						std::string val2find = "";
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
			}
		}
	}

	return retVal;
}

liaVariable liaInterpreter::evaluateExpression(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env)
{
	liaVariable retVar;

	if (theAst->nodes[0]->name == "StringLiteral")
	{
		std::string stringVal = "";
		stringVal += theAst->nodes[0]->token;
		std::regex quote_re("\"");

		retVar.type = liaVariableType::string;
		retVar.value = std::regex_replace(stringVal, quote_re, "");
	}
	else if (theAst->nodes[0]->name == "IntegerNumber")
	{
		std::string tmp = "";
		tmp += theAst->nodes[0]->token;
		retVar.type = liaVariableType::integer;
		retVar.value = std::stoi(tmp);
	}
	else if (theAst->nodes[0]->name == "BooleanConst")
	{
		std::string tmp = "";
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
		std::string varName = "";
		varName += theAst->nodes[0]->token;

		if (env->varMap.find(varName) == env->varMap.end()) 
		{
			// variable name not found
			std::string err = "";
			err += "Variable name [" + varName + "] not found. ";
			err += "Terminating.";
			fatalError(err);
		}

		liaVariable* v = &env->varMap[varName];

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

		std::string varName = "";
		varName += theAst->nodes[0]->nodes[0]->token;

		if (env->varMap.find(varName) == env->varMap.end())
		{
			// variable name not found
			std::string err = "";
			err += "Variable name [" + varName + "] not found";
			err += "Terminating.";
			fatalError(err);
		}

		retVar=exeCuteMethodCallStatement(theAst->nodes[0], env,varName);
	}
	else if (theAst->nodes[0]->name == "VariableWithProperty")
	{
		std::string varName = "";
		varName += theAst->nodes[0]->nodes[0]->token;
		std::string varProp = "";
		varProp += theAst->nodes[0]->nodes[1]->token;

		//assert(varProp == "length");

		if (env->varMap.find(varName) == env->varMap.end())
		{
			// variable name not found
			std::string err = "";
			err += "Variable name [" + varName + "] not found";
			err += "Terminating.";
			fatalError(err);
		}

		liaVariable* v = &env->varMap[varName];

		if (varProp == "length")
		{
			// TODO: handle all the types
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
			std::string err = "";
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
			assert(theAst->nodes[0]->nodes[0]->name == "ArrayList");

			if (theAst->nodes[0]->nodes[0]->nodes[0]->name == "IntegerList")
			{
				for (auto t : theAst->nodes[0]->nodes[0]->nodes[0]->nodes)
				{
					assert(t->is_token);
					liaVariable varel;
					varel.name = "varWithNoName";
					varel.type = liaVariableType::integer;
					std::string tmp = "";
					tmp += t->token;
					varel.value = std::stoi(tmp);
					retVar.vlist.push_back(varel);
				}
			}
			else if (theAst->nodes[0]->nodes[0]->nodes[0]->name == "StringList")
			{
				//std::cout << peg::ast_to_s(theAst);
				for (auto t : theAst->nodes[0]->nodes[0]->nodes[0]->nodes)
				{
					assert(t->is_token);
					liaVariable varel;
					varel.name = "varWithNoNameS";
					varel.type = liaVariableType::string;
					std::string tmp = "";
					tmp += t->token;

					std::regex quote_re("\"");
					varel.value = std::regex_replace(tmp, quote_re, "");
					retVar.vlist.push_back(varel);
				}
			}
		}
	}
	else if (theAst->nodes[0]->name == "ArraySubscript")
	{
		//std::cout << peg::ast_to_s(ch);
		std::string arrName = "";
		arrName += theAst->nodes[0]->nodes[0]->token;

		assert(theAst->nodes[0]->nodes[1]->name == "Expression");

		liaVariable vIdx = evaluateExpression(theAst->nodes[0]->nodes[1], env);
		
		// can be a string too, for dictionaries
		//assert(vIdx.type == liaVariableType::integer);

		if (env->varMap[arrName].type == liaVariableType::array)
		{
			int arrIdx = std::get<int>(vIdx.value);
			assert(arrIdx < env->varMap[arrName].vlist.size());
			retVar.type = env->varMap[arrName].vlist[0].type;
			retVar.value = env->varMap[arrName].vlist[arrIdx].value;
		}
		else if (env->varMap[arrName].type == liaVariableType::dictionary)
		{
			std::string key = std::get<std::string>(vIdx.value);
			retVar.type = env->varMap[arrName].vMap[key].type;
			retVar.value = env->varMap[arrName].vMap[key].value;
		}
		else if (env->varMap[arrName].type == liaVariableType::string)
		{
			int arrIdx = std::get<int>(vIdx.value);
			std::string tmp = std::get<std::string>(env->varMap[arrName].value);
			assert(arrIdx < tmp.size());
			retVar.type = liaVariableType::string;
			char c = tmp.at(arrIdx);
			std::string sc = "";
			sc += c;
			retVar.value = sc;
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

			std::string curKey = "";
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

	return retVar;
}

void liaInterpreter::exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);
	assert(theAst->name == "ArgList");
	assert(theAst->nodes[0]->name == "Expression");

	for (auto node : theAst->nodes)
	{
		liaVariable retVar = evaluateExpression(node, env);

		if (retVar.type == liaVariableType::array)
		{
			bool first = true;
			std::cout << "[";
			for (auto el : retVar.vlist)
			{
				if (!first) std::cout << ",";
				if (el.type == liaVariableType::string)
				{
					std::visit([](const auto& x) { std::cout << "\"" << x << "\""; }, el.value);
				}
				else
				{
					std::visit([](const auto& x) { std::cout << x; }, el.value);
				}
				first = false;
			}
			std::cout << "] ";
		}
		else if (retVar.type == liaVariableType::boolean)
		{
			if (std::get<bool>(retVar.value) == true) std::cout << "true";
			else std::cout << "false";
		}
		else
		{
			if (retVar.type == liaVariableType::string)
			{
				std::string vv = "";
				vv += std::get<std::string>(retVar.value);
			}

			std::visit([](const auto& x) { std::cout << x; }, retVar.value);
			std::cout << " ";
		}
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
		std::string err = "";
		err += "Could not open file [" + fname + "] at line "+ std::to_string(linenum)+".Terminating.";
		fatalError(err);
	}

	return retVar;
}

liaVariable liaInterpreter::customFunctionCall(std::string fname, std::vector<liaVariable>* parameters, liaEnvironment* env)
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
				std::string err = "";
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
		std::string err = "";
		err += "Unknown function name "+fname+".";
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

liaVariable liaInterpreter::exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable retVal;
	std::vector<liaVariable> parameters;

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

	for (auto ch : theAst->nodes)
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
				else if (ch->token == "toInteger")
				{
					assert(theAst->nodes[1]->name == "ArgList");
					assert(theAst->nodes[1]->nodes[0]->name == "Expression");
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);
					
					// parameter to convert must be a string
					assert(p0.type == liaVariableType::string);

					retVal.type = liaVariableType::integer;
					std::string sVal = std::get<std::string>(p0.value);

					try
					{
						retVal.value = std::stoi(sVal);
					}
					catch (...)
					{
						// unable to convert to integer
						std::string err = "";
						err += "Can't convert [" + sVal + "] to integer. ";
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
					assert(p0.type == liaVariableType::integer);

					retVal.type = liaVariableType::string;
					int sVal = std::get<int>(p0.value);
					retVal.value = std::to_string(sVal);
				}
				else if (ch->token == "ord")
				{
					// char to its ascii code
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);
					// parameter to convert must be a string
					assert(p0.type == liaVariableType::string);

					retVal.type = liaVariableType::integer;
					std::string sVal = std::get<std::string>(p0.value);
					retVal.value = (int)sVal.at(0);
				}
				else if (ch->token == "chr")
				{
					// the opposite
					liaVariable p0 = evaluateExpression(theAst->nodes[1]->nodes[0], env);
					// parameter to convert must be an integer
					assert(p0.type == liaVariableType::integer);

					retVal.type = liaVariableType::string;
					char sVal = (char)std::get<int>(p0.value);
					std::string ss = "";
					ss = (char)sVal;
					retVal.value = ss;
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
					std::string funname = "";
					funname += ch->token;
					retVal = customFunctionCall(funname,&parameters,env);
				}
			}
		}
	}

	return retVal;
}

void liaInterpreter::exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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

void liaInterpreter::exeCuteArrayAssignmentStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	assert(theAst->nodes[0]->name=="ArraySubscript");
	assert(theAst->nodes[1]->name=="Expression");

	liaVariable value2assign = evaluateExpression(theAst->nodes[1], env);
	std::string arrayName = "";
	arrayName+=theAst->nodes[0]->nodes[0]->token;

	if (env->varMap.find(arrayName) == env->varMap.end())
	{
		// variable not found
		std::string err = "";
		err += "Array name [" + arrayName + "] not found. ";
		err += "Terminating.";
		fatalError(err);
	}

	liaVariable arrayIndex= evaluateExpression(theAst->nodes[0]->nodes[1], env);

	if (env->varMap[arrayName].type == liaVariableType::array)
	{
		assert(arrayIndex.type == liaVariableType::integer);
		assert(std::get<int>(arrayIndex.value) < env->varMap[arrayName].vlist.size());

		env->varMap[arrayName].vlist[std::get<int>(arrayIndex.value)] = value2assign;
	}
	else if (env->varMap[arrayName].type == liaVariableType::dictionary)
	{
		assert(arrayIndex.type == liaVariableType::string);

		env->varMap[arrayName].vMap[std::get<std::string>(arrayIndex.value)] = value2assign;
	}
}

void liaInterpreter::exeCuteMultiplyStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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
				assert(vInc.type == liaVariableType::integer);
				mulAmount = std::get<int>(vInc.value);
			}
		}
	}

	//for (int i = 0;i < env->varList.size();i++)
	{
		//liaVariable variable = env->varList[i];
		//if (variable.name == theVar.name)
		{
			liaVariable variable = env->varMap[theVar.name];

			if (variable.type == liaVariableType::integer)
			{
				int vv = std::get<int>(env->varMap[theVar.name].value);
				env->varMap[theVar.name].value = vv*=mulAmount;
			}
			else
			{
				std::string err = "";
				err += "Trying to multiply numerically a variable of other type";
				err += " at line " + std::to_string(curLine) + ".";
				err += "Terminating.";
				fatalError(err);
			}
		}
	}
	// TODO: handle other types

}

void liaInterpreter::exeCuteLogicalAndStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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

	if (env->varMap[theVar.name].type == liaVariableType::integer)
	{
		int vv = std::get<int>(env->varMap[theVar.name].value);
		env->varMap[theVar.name].value = vv & rExpr;
	}
	else
	{
		std::string err = "";
		err += "Logical and is valid only between integers";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}
}

void liaInterpreter::exeCuteLogicalOrStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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

	if (env->varMap[theVar.name].type == liaVariableType::integer)
	{
		int vv = std::get<int>(env->varMap[theVar.name].value);
		env->varMap[theVar.name].value = vv | rExpr;
	}
	else
	{
		std::string err = "";
		err += "Logical or is valid only between integers";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}
}

void liaInterpreter::exeCuteRshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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
		if (env->varMap[theVar.name].type == liaVariableType::integer)
		{
			int vv = std::get<int>(env->varMap[theVar.name].value);
			env->varMap[theVar.name].value = vv>>rshiftAmount;
		}
		else
		{
			std::string err = "";
			err += "Trying to increment numerically a variable of other type";
			err += " at line " + std::to_string(curLine) + ".";
			err += "Terminating.";
			fatalError(err);
		}
	}
	// TODO: handle other types
}

void liaInterpreter::exeCuteLshiftStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
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
		if (env->varMap[theVar.name].type == liaVariableType::integer)
		{
			int vv = std::get<int>(env->varMap[theVar.name].value);
			env->varMap[theVar.name].value = vv << lshiftAmount;
		}
		else
		{
			std::string err = "";
			err += "Trying to increment numerically a variable of other type";
			err += " at line " + std::to_string(curLine) + ".";
			err += "Terminating.";
			fatalError(err);
		}
	}
	// TODO: handle other types
}

void liaInterpreter::exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env,int inc)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	size_t curLine;
	liaVariable theInc;

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
				theInc = evaluateExpression(ch,env);
			}
		}
	}

	if (env->varMap.find(theVar.name) == env->varMap.end())
	{
		std::string err = "";
		err += "Variable "+theVar.name+" not found.";
		err += "Terminating.";
		fatalError(err);
	}

	liaVariable* pvar = NULL;
	pvar = &env->varMap[theVar.name];

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
	else
	{
		std::string err = "";
		err += "Trying to increment numerically a variable of other type";
		err += " at line " + std::to_string(curLine) + ".";
		err += "Terminating.";
		fatalError(err);
	}

	// TODO: handle other types
}

void liaInterpreter::addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env,size_t curLine)
{
	if (env->varMap.find(v->name) != env->varMap.end())
	{
		// TODO handle other types
		if (v->type == liaVariableType::integer)
		{
			env->varMap[v->name].value = v->value;
		}
		else if (v->type == liaVariableType::string)
		{
			env->varMap[v->name].value = v->value;
		}
		else if (v->type == liaVariableType::boolean)
		{
			env->varMap[v->name].value = v->value;
		}
		else if (v->type == liaVariableType::array)
		{
			env->varMap[v->name].vlist.clear();
			for (auto el : v->vlist)
			{
				env->varMap[v->name].vlist.push_back(el);
			}
		}
		// TODO: check this
		/*else
		{
			std::string err="";
			err += "Trying to assign a different type to variable ";
			err += "at line " + std::to_string(curLine)+".";
			err+="Terminating.";
			fatalError(err);
		}*/
	}
	else
	{
		// else, add it to the env
		env->varMap[v->name] = *v;
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

bool liaInterpreter::evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	// a condition is Expression Relop Expression
	// std::cout << peg::ast_to_s(theAst);

	std::string relOp = "";
	relOp+=theAst->nodes[1]->token;

	liaVariable lExpr = evaluateExpression(theAst->nodes[0], env);
	liaVariable rExpr = evaluateExpression(theAst->nodes[2], env);

	if (lExpr.type != rExpr.type)
	{
		std::string err = "";
		err += "Comparing variables of different types is forbidden. ";
		err += "Terminating.";
		fatalError(err);
		return false;
	}

	return primitiveComparison(lExpr.value, rExpr.value, relOp);
}

liaVariable liaInterpreter::exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	std::shared_ptr<peg::Ast> pCond;
	std::shared_ptr<peg::Ast> pBlock;

	for (auto ch : theAst->nodes)
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

liaVariable liaInterpreter::exeCuteForeachStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	liaVariable retVal;
	//std::cout << peg::ast_to_s(theAst);

	assert(theAst->nodes.size() == 3);

	std::shared_ptr<peg::Ast> pBlock;

	for (auto ch : theAst->nodes)
	{
		if (ch->name == "CodeBlock")
		{
			pBlock = ch;
		}
	}

	std::string tmpVarname = "";
	tmpVarname = theAst->nodes[0]->token;

	std::string arrVarname = ""; // array or string
	arrVarname = theAst->nodes[1]->token;

	if (env->varMap.find(arrVarname) == env->varMap.end())
	{
		std::string err = "";
		err += "Variable named "+arrVarname+" not found.";
		err += "Terminating.";
		fatalError(err);
	}

	liaVariable* pArr = &env->varMap[arrVarname];

	if (pArr->type == liaVariableType::array)
	{
		if (pArr->vlist.size() > 0)
		{
			liaVariable tmpVar;
			tmpVar.type = pArr->vlist[0].type;

			for (int idx = 0;idx < pArr->vlist.size();idx++)
			{
				tmpVar.value = pArr->vlist[idx].value;

				env->varMap[tmpVarname] = tmpVar;

				retVal = exeCuteCodeBlock(pBlock, env);
				if (retVal.name == "lia-ret-val")
				{
					env->varMap.erase(tmpVarname);
					return retVal;
				}

				// remove temporary variable from scope
				env->varMap.erase(tmpVarname);
			}
		}
	}
	else if (pArr->type == liaVariableType::string)
	{
		std::string s = "";
		s = std::get<std::string>(pArr->value);
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

				// remove temporary variable from scope
				env->varMap.erase(tmpVarname);
			}
		}
	}

	return retVal;
}

liaVariable liaInterpreter::exeCuteIfStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
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

liaVariable liaInterpreter::exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env)
{
	liaVariable retVal;

	for (auto stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
		
		if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "FuncCallStmt"))
		{
			// user function or library function call
			exeCuteFuncCallStatement(stmt->nodes[0],env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "VarFuncCallStmt"))
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			std::string variableName = "";
			variableName += stmt->nodes[0]->nodes[0]->token;
			exeCuteMethodCallStatement(stmt->nodes[0], env, variableName);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "VarDeclStmt"))
		{
			// variable declaration/assignment
			//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
			exeCuteVarDeclStatement(stmt->nodes[0],env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "ArrayAssignmentStmt"))
		{
			// assignment to array element
			exeCuteArrayAssignmentStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && ( (stmt->nodes[0]->name == "IncrementStmt") || (stmt->nodes[0]->name == "DecrementStmt")) )
		{
			int inc = 1;
			if (stmt->nodes[0]->name == "DecrementStmt") inc = -1;
			exeCuteIncrementStatement(stmt->nodes[0],env,inc);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "RshiftStmt"))
		{
			exeCuteRshiftStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "LshiftStmt"))
		{
			exeCuteLshiftStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "LogicalAndStmt"))
		{
			exeCuteLogicalAndStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "LogicalOrStmt"))
		{
			exeCuteLogicalOrStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "MultiplyStmt"))
		{
			exeCuteMultiplyStatement(stmt->nodes[0], env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "WhileStmt"))
		{
			// while statement, the cradle of all infinite loops
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal=exeCuteWhileStatement(stmt->nodes[0],env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "ForeachStmt"))
		{
			retVal = exeCuteForeachStatement(stmt->nodes[0], env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "IfStmt"))
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal=exeCuteIfStatement(stmt->nodes[0], env);
			if (retVal.name == "lia-ret-val")
			{
				return retVal;
			}
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "ReturnStmt"))
		{
			//std::cout << peg::ast_to_s(stmt->nodes[0]);
			retVal = evaluateExpression(stmt->nodes[0]->nodes[0], env);
			retVal.name = "lia-ret-val"; // gah
			return retVal;
		}
	}

	return retVal;
}

// where the Cuteness starts
void liaInterpreter::exeCute(std::shared_ptr<peg::Ast> theAst)
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
