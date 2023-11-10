
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

void liaInterpreter::exeCuteLibFunctionPrint(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);
	assert(theAst->name == "ArgList");
	assert(theAst->nodes[0]->name == "Expression");

	// TODO: parse expression and print it properly

	if (theAst->nodes[0]->nodes[0]->name == "StringLiteral")
	{
		std::string s2print = "";
		s2print+=theAst->nodes[0]->nodes[0]->token;
		std::regex quote_re("\"");
		std::cout << std::regex_replace(s2print, quote_re, "") << std::endl;
	}
	else if (theAst->nodes[0]->nodes[0]->name == "VariableName")
	{
		std::string varName = "";
		varName+=theAst->nodes[0]->nodes[0]->token;

		bool varFound = false;
		for (auto v : env->varList)
		{
			if (v.name == varName)
			{
				varFound = true;

				// TODO: handle all the types
				if (v.type == liaVariableType::integer)
				{
					int val2print = std::get<int>(v.value);
					std::cout << val2print << std::endl;
				}
				else if (v.type == liaVariableType::string)
				{
					std::string s2print = std::get<std::string>(v.value);
					std::cout << s2print << std::endl;
				}
			}
		}

		if (!varFound)
		{
			// variable name not found
			std::string err = "";
			err += "Variable name ["+varName+"] not found";
			err += "Terminating.";
			fatalError(err);
		}
	}
}

void liaInterpreter::exeCuteFuncCallStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

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
			}
		}
	}
}

void liaInterpreter::exeCuteVarDeclStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				//std::cout << ch->token << std::endl;
				int lineNum = ch->line;
				theVar.line = (int)lineNum;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				if (ch->nodes[0]->name == "IntegerNumber")
				{
					theVar.type = liaVariableType::integer;
					std::string tmp = "";
					tmp += ch->nodes[0]->token;
					theVar.value = std::stoi(tmp);
				}
				else if (ch->nodes[0]->name == "StringLiteral")
				{
					theVar.type = liaVariableType::string;
					std::regex quote_re("\"");
					std::string tmp = "";
					tmp+= ch->nodes[0]->token;
					tmp = std::regex_replace(tmp, quote_re, "");
					theVar.value = tmp;
				}

				// TODO: handle other types

			}
		}
	}

	// now, if variable is not in env, create it. otherwise, update it
	addvarOrUpdateEnvironment(&theVar, env);
}

void liaInterpreter::exeCuteIncrementStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{
	//std::cout << peg::ast_to_s(theAst);

	liaVariable theVar;
	int iIncrement = 0;

	for (auto ch : theAst->nodes)
	{
		if (ch->is_token)
		{
			if (ch->name == "VariableName")
			{
				theVar.name += ch->token;
				int lineNum = ch->line;
				theVar.line = (int)lineNum;
			}
		}
		else
		{
			if (ch->name == "Expression")
			{
				if (ch->nodes[0]->name == "IntegerNumber")
				{
					theVar.type = liaVariableType::integer;
					std::string tmp = "";
					tmp += ch->nodes[0]->token;
					iIncrement = std::stoi(tmp);
				}

				// TODO: handle other types

			}
		}
	}

	if (iIncrement != 0)
	{
		for (int i = 0;i < env->varList.size();i++)
		{
			liaVariable variable = env->varList[i];
			if (variable.name == theVar.name)
			{
				if (variable.type == liaVariableType::integer)
				{
					int vv = std::get<int>(env->varList[i].value);
					env->varList[i].value = vv+iIncrement;
				}
				else
				{
					std::string err = "";
					err += "Trying to increment numerically a variable of other type";
					err += " at line " + std::to_string(theVar.line) + ".";
					err += "Terminating.";
					fatalError(err);
				}
			}
		}
	}
	// TODO: handle other types
}

void liaInterpreter::addvarOrUpdateEnvironment(liaVariable* v, liaEnvironment* env)
{
	for (int i=0;i<env->varList.size();i++)
	{
		liaVariable variable = env->varList[i];
		if (variable.name == v->name)
		{
			if (variable.type == v->type)
			{
				// TODO handle other types
				if (v->type == liaVariableType::integer)
				{
					env->varList[i].value = v->value;
				}
			}
			else
			{
				std::string err="";
				err += "Trying to assign a different type to variable ";
				err += "at line " + std::to_string(v->line)+".";
				err+="Terminating.";
				fatalError(err);
			}

			return;
		}
	}

	// else, add it to the env
	env->varList.push_back(*v);
}

bool liaInterpreter::evaluateCondition(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
{


	return false;
}

void liaInterpreter::exeCuteWhileStatement(std::shared_ptr<peg::Ast> theAst, liaEnvironment* env)
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

	exeCuteCodeBlock(pBlock,env);
}

void liaInterpreter::exeCuteCodeBlock(std::shared_ptr<peg::Ast> theAst,liaEnvironment* env)
{
	for (auto stmt : theAst->nodes)
	{
		//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
		
		if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "EndLine"))
		{
			// ignoring endline
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "SingleLineCommentStmt"))
		{
			// ignoring (so meaningful) comment
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "FuncCallStmt"))
		{
			// user function or library function call
			exeCuteFuncCallStatement(stmt->nodes[0],env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "VarDeclStmt"))
		{
			// variable declaration/assignment
			//std::cout << stmt->name << " " << stmt->nodes.size() << "-" << stmt->nodes[0]->name << std::endl;
			exeCuteVarDeclStatement(stmt->nodes[0],env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "IncrementStmt"))
		{
			exeCuteIncrementStatement(stmt->nodes[0],env);
		}
		else if ((stmt->nodes.size() == 1) && (stmt->nodes[0]->name == "WhileStmt"))
		{
			// while statement, the cradle of all infinite loops
			std::cout << peg::ast_to_s(stmt->nodes[0]);
			exeCuteWhileStatement(stmt->nodes[0],env);
		}
	}
}

// where the Cuteness starts
void liaInterpreter::exeCute(std::shared_ptr<peg::Ast> theAst)
{
	// basically, we have to find the "main" codeblock and exeCute it
	// in the mean time, we can create "environments", that is list of variables, with types and values
	// there should be a global scope, I guess. Even if global variables suck

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
