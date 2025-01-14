
/* 

	lia VM - 2o25 
	TODO's:
	- maps // DONE
	- solve two sequential foreaches 

*/

#include "vm.h"

liaVM::liaVM()
{
}

//
// exeCute
//

bool compareIntVars(liaVariable i1, liaVariable i2);
bool compareLongVars(liaVariable i1, liaVariable i2);
bool compareStringVars(liaVariable i1, liaVariable i2);

void liaVM::fatalError(std::string err)
{
	std::cout << "Error: " << err << std::endl;
	throw vmException();
}

void liaVM::getExpressionFromCode(liaCodeChunk& chunk, unsigned int pos, unsigned int& bytesRead, liaVariable* retvar,std::vector<liaVariable>* env)
{
	if (chunk.code[pos] == liaOpcode::opConstant)
	{
		retvar->type = constantPool[chunk.code[pos + 1]].type;
		retvar->value = constantPool[chunk.code[pos + 1]].value;
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetLocalVariable)
	{
		retvar->type = (*env)[chunk.code[pos + 1]].type;
		retvar->value = (*env)[chunk.code[pos + 1]].value;
		if (retvar->type == liaVariableType::array)
		{
			retvar->vlist = (*env)[chunk.code[pos + 1]].vlist;
		}
		if (retvar->type == liaVariableType::dictionary)
		{
			retvar->vMap = (*env)[chunk.code[pos + 1]].vMap;
		}
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetGlobalVariable)
	{
		retvar->type = globalEnv[chunk.code[pos + 1]].type;
		retvar->value = globalEnv[chunk.code[pos + 1]].value;
		if (retvar->type == liaVariableType::array)
		{
			retvar->vlist = globalEnv[chunk.code[pos + 1]].vlist;
		}
		if (retvar->type == liaVariableType::dictionary)
		{
			retvar->vMap = globalEnv[chunk.code[pos + 1]].vMap;
		}
		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opArrayInitializer)
	{
		unsigned int br;
		unsigned int numElements = chunk.code[pos + 1];
		bytesRead = 2;
		retvar->vlist.clear();
		for (unsigned int i = 0;i < numElements;i++)
		{
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element,env);
			retvar->vlist.push_back(element);
			bytesRead += br;
			pos += br;
		}

		retvar->type = liaVariableType::array;
	}
	else if (chunk.code[pos] == liaOpcode::opDictInitializer)
	{
		//unsigned int br;
		unsigned int numElements = chunk.code[pos + 1];
		bytesRead = 2;
		retvar->vMap.clear();
		/*for (unsigned int i = 0;i < numElements;i++)
		{
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element, env);
			retvar->vlist.push_back(element);
			bytesRead += br;
			pos += br;
		}*/

		retvar->type = liaVariableType::dictionary;
	}
	else if ((chunk.code[pos] == liaOpcode::opGetObjectLengthOfSubscript)|| (chunk.code[pos] == liaOpcode::opGetObjectLengthOfSubscriptGlobal))
	{
		retvar->type = liaVariableType::integer;
		bytesRead = 3;

		liaVariable* pVar;
		if (chunk.code[pos] == liaOpcode::opGetObjectLengthOfSubscript) pVar = &(*env)[chunk.code[pos + 1]];
		else pVar = &globalEnv[chunk.code[pos + 1]];

		for (unsigned int sub = 0;sub < chunk.code[pos + 2];sub++)
		{
			unsigned int br = 0;
			getExpressionFromCode(chunk, pos + bytesRead, br, &internalSub, env);

			int idx = std::get<int>(internalSub.value);
			if (pVar->type == liaVariableType::array)
			{
				if (idx >= pVar->vlist.size())
				{
					fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
				}
				pVar = &pVar->vlist[idx];
			}
			else
			{
				fatalError("Unhandled getLengthOf var type");
			}
			/*else if (pVar->type == liaVariableType::string)
			{
				if (idx >= std::get<std::string>(pVar->value).size())
				{
					fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
				}
				retvar->type = liaVariableType::string;
				char c = std::get<std::string>(pVar->value).at(idx);
				std::string sc;
				sc += c;
				retvar->value = sc;
			}*/
			bytesRead += br;
		}

		if (pVar->type == liaVariableType::array)
		{
			retvar->value = (int)pVar->vlist.size();
		}
		else if (pVar->type == liaVariableType::string)
		{
			retvar->value = (int)std::get<std::string>(pVar->value).size();
		}
		else
		{
			fatalError("Asking length of object with unsupported type:"+ pVar->type);
		}

	}
	else if ((chunk.code[pos] == liaOpcode::opArraySubscript)|| (chunk.code[pos] == liaOpcode::opArraySubscriptGlobal))
	{
		bytesRead = 3;
		liaVariable* pVar;
		if (chunk.code[pos]==liaOpcode::opArraySubscript) pVar= &(*env)[chunk.code[pos + 1]];
		else pVar= &globalEnv[chunk.code[pos + 1]];

		if ((pVar->type == liaVariableType::array)|| (pVar->type == liaVariableType::string))
		{
			const unsigned int numSubs = chunk.code[pos + 2];
			for (unsigned int sub = 0;sub < numSubs;sub++)
			{
				unsigned int br = 0;
				//liaVariable asub;
				getExpressionFromCode(chunk, pos + bytesRead, br, &internalSub, env);

				const int idx = std::get<int>(internalSub.value);
				if (pVar->type == liaVariableType::array)
				{
					if (idx >= pVar->vlist.size())
					{
						fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
					}
					pVar = &pVar->vlist[idx];
					if (sub == (numSubs - 1))
					{
						retvar->type = pVar->type;
						retvar->value = pVar->value;
						if (pVar->type == liaVariableType::array)
						{
							retvar->vlist = pVar->vlist;
						}
						if (pVar->type == liaVariableType::dictionary)
						{
							retvar->vMap = pVar->vMap;
						}
					}
				}
				else if (pVar->type == liaVariableType::string)
				{
					if (idx >= std::get<std::string>(pVar->value).size())
					{
						fatalError("Out of bounds - requested index: " + std::to_string(idx) + " - array size: " + std::to_string(pVar->vlist.size()));
					}
					retvar->type = liaVariableType::string;
					char c = std::get<std::string>(pVar->value).at(idx);
					std::string sc;
					sc += c;
					retvar->value = sc;
				}
				bytesRead += br;
			}
		}
		else if (pVar->type==liaVariableType::dictionary)
		{
			const unsigned int numSubs = chunk.code[pos + 2];
			assert(numSubs == 1);

			unsigned int br = 0;
			getExpressionFromCode(chunk, pos + bytesRead, br, &internalSub, env);

			std::string key = std::get<std::string>(internalSub.value);

			if (pVar->vMap.find(key) == pVar->vMap.end())
			{
				std::string err;
				err += "Key " + key + " not found in dictionary. ";
				err += "Terminating.";
				fatalError(err);
			}

			*retvar = pVar->vMap[key];
			bytesRead += br;
		}
		else
		{
			fatalError("Uns0pported subscript "+pVar->type);
		}
	}
	else if (chunk.code[pos] == liaOpcode::opLibFunctionCall)
	{
		//std::cout << "libfuncall" << std::endl;
		unsigned int funId = chunk.code[pos + 1];
		if (funId == StdFunctionId::FunctionGetMillisecondsSinceEpoch)
		{
			long long now = (long long)std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
			retvar->value = now;
			bytesRead = 2;
		}
		else if (funId == StdFunctionId::FunctionToInteger)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element,env);
			retvar->type = liaVariableType::integer;
			retvar->value = std::stoi(std::get<std::string>(element.value));
			bytesRead = br+2;
		}
		else if (funId == StdFunctionId::FunctionToLong)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element, env);
			retvar->type = liaVariableType::longint;
			retvar->value = std::stoll(std::get<std::string>(element.value));
			bytesRead = br + 2;
		}
		else if (funId == StdFunctionId::FunctionToString)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element, env);
			retvar->type = liaVariableType::string;
			
			if (element.type == liaVariableType::integer)
			{
				retvar->value = std::to_string(std::get<int>(element.value));
			}
			else if (element.type == liaVariableType::longint)
			{
				retvar->value = std::to_string(std::get<long long>(element.value));
			}
			else
			{
				fatalError("Unhandled toString type");
			}
			bytesRead = br + 2;
		}
		else if (funId == StdFunctionId::FunctionReadTextFileLineByLine)
		{
			unsigned int br = 0;
			liaVariable fname;
			getExpressionFromCode(chunk, pos + 2, br, &fname,env);
			retvar->type = liaVariableType::array;

			std::string line;
			std::ifstream file(std::get<std::string>(fname.value));
			if (file.is_open())
			{
				while (std::getline(file, line))
				{
					liaVariable l;
					l.type = liaVariableType::string;
					l.value = line;
					retvar->vlist.push_back(l);
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
		else if (funId == StdFunctionId::FunctionLsqrt)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element, env);
			retvar->type = liaVariableType::longint;

			long long iVal = std::get<long long>(element.value);
			double fsq = sqrt(iVal);

			retvar->value = (long long)fsq;
			bytesRead = br + 2;
		}
		else if (funId == StdFunctionId::FunctionRnd)
		{
			unsigned int br = 0;
			liaVariable element;
			getExpressionFromCode(chunk, pos + 2, br, &element, env);
			retvar->type = liaVariableType::integer;

			retvar->value = rand() % std::get<int>(element.value);
			bytesRead = br + 2;
		}
		else
		{
			fatalError("Unknown lib function called");
		}
	}
	else if ((chunk.code[pos] == liaOpcode::opVarFunctionCall)|| (chunk.code[pos] == liaOpcode::opVarFunctionCallGlobal))
	{
		//std::cout << "varfuncall" << std::endl;
		unsigned int varId = chunk.code[pos + 1];
		unsigned int funId = chunk.code[pos + 2];

		liaVariable* pVariable;
		if (chunk.code[pos] == liaOpcode::opVarFunctionCall) pVariable = &(*env)[varId];
		else pVariable = &globalEnv[varId];

		if (funId == libMethodId::MethodSplit)
		{
			retvar->type = liaVariableType::array;
			retvar->vlist.clear();

			unsigned int br = 0;
			liaVariable deLim;
			getExpressionFromCode(chunk, pos + 3, br, &deLim,env);

			std::string string2split = std::get<std::string>(pVariable->value);
			std::string delimiter = std::get<std::string>(deLim.value);

			size_t pos = 0;
			std::string token;
			while ((pos = string2split.find(delimiter)) != std::string::npos)
			{
				token = string2split.substr(0, pos);
				liaVariable varEl;
				varEl.type = liaVariableType::string;
				varEl.value = token;
				retvar->vlist.push_back(varEl);
				string2split.erase(0, pos + delimiter.length());
			}

			token = string2split.substr(0, pos);
			liaVariable varEl;
			varEl.type = liaVariableType::string;
			varEl.value = token;
			retvar->vlist.push_back(varEl);

			bytesRead = 3 + br;
		}
		else if (funId == libMethodId::MethodFind)
		{
			retvar->type = liaVariableType::integer;

			unsigned int br = 0;
			liaVariable val2find;
			getExpressionFromCode(chunk, pos + 3, br, &val2find, env);

			if (pVariable->type == liaVariableType::array)
			{
				ptrdiff_t pos = std::distance(pVariable->vlist.begin(), std::find(pVariable->vlist.begin(), pVariable->vlist.end(), val2find));
				if (pos < (signed int)pVariable->vlist.size())
				{
					retvar->value = (int)pos;
				}
				else
				{
					retvar->value = -1;
				}
			}
			else if (pVariable->type == liaVariableType::string)
			{
				std::string s = std::get<std::string>(pVariable->value);
				const char* ptr = strstr(s.c_str(), std::get<std::string>(val2find.value).c_str());
				if (ptr == NULL)
				{
					retvar->value = -1;
				}
				else
				{
					const char* ptrbeg = s.c_str();
					retvar->value = (int)(ptr - ptrbeg);
				}
			}
			else
			{
				fatalError("Unsupported find type.");
			}

			bytesRead = 3 + br;
		}
		else if (funId == libMethodId::MethodFindkey)
		{
			retvar->type = liaVariableType::integer;
			retvar->value = -1;

			unsigned int br = 0;
			liaVariable key2find;
			getExpressionFromCode(chunk, pos + 3, br, &key2find, env);

			if (key2find.type == liaVariableType::string)
			{
				if (pVariable->vMap.find(std::get<std::string>(key2find.value)) != pVariable->vMap.end())
				{
					retvar->value = 1;
				}
			}
			else
			{
				std::string err;
				err += "Unhandled findKey type. ";
				err += "Terminating.";
				fatalError(err);
			}

			bytesRead = 3 + br;
		}
		else if (funId == libMethodId::MethodSlice)
		{
			liaVariable vbeg,vend;
			unsigned int br = 0;
			unsigned totbytes = 0;
			getExpressionFromCode(chunk, pos + 3, br, &vbeg, env);
			totbytes += br;
			getExpressionFromCode(chunk, pos + 3+br, br, &vend, env);
			totbytes += br;

			int beg = std::get<int>(vbeg.value);
			int end = std::get<int>(vend.value);

			if (pVariable->type == liaVariableType::string)
			{
				std::string s = std::get<std::string>(pVariable->value);

				if (s.size() == 0)
				{
					retvar->type = liaVariableType::string;
					retvar->value = s;
					bytesRead = 3 + totbytes;
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

				retvar->type = liaVariableType::string;
				retvar->value = s2;
				bytesRead = 3 + totbytes;
				return;
			}
			else if (pVariable->type == liaVariableType::array)
			{
				retvar->type = liaVariableType::array;
				int idx = 0;
				while (idx < end)
				{
					if (idx >= beg)
					{
						retvar->vlist.push_back(pVariable->vlist[idx]);
					}
					idx += 1;
				}

				bytesRead = 3 + totbytes;
				return;
			}
		}
		else
		{
			fatalError("Unknown lib method called "+std::to_string(funId));
		}
	}
	else if (chunk.code[pos] == liaOpcode::opGetObjectLength)
	{
		//std::cout << "objlen" << std::endl;

		retvar->type = liaVariableType::integer;
		liaVariable* obj = &(*env)[chunk.code[pos + 1]];

		if (obj->type == liaVariableType::array)
		{
			retvar->value = (int)obj->vlist.size();
		}
		else if (obj->type == liaVariableType::string)
		{
			retvar->value = (int)std::get<std::string>(obj->value).size();
		}
		else
		{
			fatalError("Asking length of object with unsupported type:"+ obj->type);
		}

		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetObjectLengthGlobal)
	{
		//std::cout << "objlen" << std::endl;

		retvar->type = liaVariableType::integer;
		liaVariable* obj = &globalEnv[chunk.code[pos + 1]];

		if (obj->type == liaVariableType::array)
		{
			retvar->value = (int)obj->vlist.size();
		}
		else if (obj->type == liaVariableType::string)
		{
			retvar->value = (int)std::get<std::string>(obj->value).size();
		}
		else
		{
			fatalError("Asking length of object with unsupported type:"+obj->type);
		}

		bytesRead = 2;
	}
	else if ((chunk.code[pos] == liaOpcode::opGetObjectType)|| (chunk.code[pos] == liaOpcode::opGetObjectTypeGlobal))
	{
		retvar->type = liaVariableType::string;
		liaVariable* obj;
		if (chunk.code[pos]==liaOpcode::opGetObjectType) obj=&(*env)[chunk.code[pos + 1]];
		else obj=&globalEnv[chunk.code[pos + 1]];

		if (obj->type == liaVariableType::array)
		{
			retvar->value = "array";
		}
		else if (obj->type == liaVariableType::integer)
		{
			retvar->value = "integer";
		}
		else if (obj->type == liaVariableType::longint)
		{
			retvar->value = "long";
		}
		else if (obj->type == liaVariableType::string)
		{
			retvar->value = "string";
		}
		else
		{
			fatalError("Asking type of object with unsupported type.");
		}

		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opFunctionCall)
	{
		const unsigned int funId = chunk.code[pos + 1];
		unsigned int pbeer = 0;
		executeChunk(chunks[funId], *retvar,funId,pbeer,&chunk,pos,env);
		bytesRead = 2 + pbeer;
	}
	else if (chunk.code[pos] == liaOpcode::opNot)
	{
		unsigned int br = 0;
		liaVariable exzpr;
		getExpressionFromCode(chunk, pos + 1, br, &exzpr, env);

		*retvar = exzpr;
		if (exzpr.type == liaVariableType::boolean)
		{
			retvar->value = !std::get<bool>(retvar->value);
		}
		else
		{
			fatalError("Unhandled not type for opNot");
		}

		bytesRead = br + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opMathExpression)
	{
		liaVariable partialVal;

		unsigned int numExpressions = chunk.code[pos + 1];

		unsigned int bytesTot = 0;
		unsigned int br = 0;

		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 2, br, &lexpr, env);
		bytesTot += br;

		partialVal.value = lexpr.value;
		partialVal.type = lexpr.type;

		for (unsigned int me = 0;me < numExpressions;me++)
		{
			unsigned int mathOpz = chunk.code[pos + 2 + me + bytesTot];

			br = 0;
			liaVariable rexpr;
			getExpressionFromCode(chunk, pos + 2 + (me+1) + bytesTot, br, &rexpr, env);
			bytesTot += br;

			if (mathOpz == liaOpcode::opAdd)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					retvar->type = liaVariableType::integer;
					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) + rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					retvar->type = liaVariableType::longint;
					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) + rv;
				}
				else if (lexpr.type == liaVariableType::string)
				{
					retvar->type = liaVariableType::string;
					std::string rv = std::get<std::string>(rexpr.value);
					partialVal.value = std::get<std::string>(partialVal.value) + rv;
				}
				else
				{
					fatalError("Unsupported type in math op +");
				}
			}
			else if (mathOpz == liaOpcode::opSubtract)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					partialVal.type = liaVariableType::integer;
					retvar->type = liaVariableType::integer;

					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) - rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					partialVal.type = liaVariableType::longint;
					retvar->type = liaVariableType::longint;

					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) - rv;
				}
				else
				{
					fatalError("Unsupported type in math op -");
				}
			}
			else if (mathOpz == liaOpcode::opMultiply)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					partialVal.type = liaVariableType::integer;
					retvar->type = liaVariableType::integer;

					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) * rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					partialVal.type = liaVariableType::longint;
					retvar->type = liaVariableType::longint;

					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) * rv;
				}
				else
				{
					fatalError("Unsupported type in math op *");
				}
			}
			else if (mathOpz == liaOpcode::opDivide)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					partialVal.type = liaVariableType::integer;
					retvar->type = liaVariableType::integer;

					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) / rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					partialVal.type = liaVariableType::longint;
					retvar->type = liaVariableType::longint;

					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) / rv;
				}
				else
				{
					fatalError("Unsupported type in math op /");
				}
			}
			else if (mathOpz == liaOpcode::opModulo)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					partialVal.type = liaVariableType::integer;
					retvar->type = liaVariableType::integer;

					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) % rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					partialVal.type = liaVariableType::longint;
					retvar->type = liaVariableType::longint;

					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) % rv;
				}
				else
				{
					fatalError("Unsupported type in math op %");
				}

			}
			else if (mathOpz == liaOpcode::opXor)
			{
				if (lexpr.type == liaVariableType::integer)
				{
					partialVal.type = liaVariableType::integer;
					retvar->type = liaVariableType::integer;

					int rv = std::get<int>(rexpr.value);
					partialVal.value = std::get<int>(partialVal.value) ^ rv;
				}
				else if (lexpr.type == liaVariableType::longint)
				{
					partialVal.type = liaVariableType::longint;
					retvar->type = liaVariableType::longint;

					long long rv = std::get<long long>(rexpr.value);
					partialVal.value = std::get<long long>(partialVal.value) ^ rv;
				}
				else
				{
					fatalError("Unsupported type in math op ^");
				}

			}
		}

		retvar->value = partialVal.value;

		bytesRead = bytesTot + 2 + numExpressions;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareGreaterEqual)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;
		if (lexpr.type == liaVariableType::integer)
		{
			retvar->value = std::get<int>(lexpr.value) >= std::get<int>(rexpr.value);
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			retvar->value = std::get<long long>(lexpr.value) >= std::get<long long>(rexpr.value);
		}
		else
		{
			fatalError("Unsupported comparison type for >=");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareGreater)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;
		if (lexpr.type == liaVariableType::integer)
		{
			bool cond = std::get<int>(lexpr.value) > std::get<int>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			retvar->value = std::get<long long>(lexpr.value) > std::get<long long>(rexpr.value);
		}
		else
		{
			fatalError("Unsupported comparison type for >");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareLess)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;
		if (lexpr.type == liaVariableType::integer)
		{
			retvar->value = std::get<int>(lexpr.value) < std::get<int>(rexpr.value);
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			retvar->value = std::get<long long>(lexpr.value) < std::get<long long>(rexpr.value);
		}
		else
		{
			fatalError("Unsupported comparison type for <");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareEqual)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;

		if (lexpr.type != rexpr.type)
		{
			fatalError("opCompareEqual: Comparing variables of different types");
		}

		if (lexpr.type == liaVariableType::integer)
		{
			bool cond = std::get<int>(lexpr.value) == std::get<int>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::boolean)
		{
			bool cond = std::get<bool>(lexpr.value) == std::get<bool>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			bool cond = std::get<long long>(lexpr.value) == std::get<long long>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::string)
		{
			bool cond = std::get<std::string>(lexpr.value) == std::get<std::string>(rexpr.value);
			retvar->value = cond;
		}
		else
		{
			fatalError("Unsupported comparison type ["+std::to_string(lexpr.type)+"] for ==");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareLessEqual)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		if (lexpr.type != rexpr.type)
		{
			fatalError("Comparing with <= variables of different type");
		}

		retvar->type = liaVariableType::boolean;
		if (lexpr.type == liaVariableType::integer)
		{
			bool cond = std::get<int>(lexpr.value) <= std::get<int>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			bool cond = std::get<long long>(lexpr.value) <= std::get<long long>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::array)
		{
			// array comparison...gah
			if (lexpr.vlist.size() != rexpr.vlist.size())
			{
				retvar->value = false;
			}
			else
			{
				retvar->value = true;
				for (unsigned int idx = 0;idx < lexpr.vlist.size();idx++)
				{
					if (lexpr.vlist[idx].value > rexpr.vlist[idx].value)
					{
						retvar->value = false;
					}
				}
			}
		}
		else
		{
			fatalError("Unsupported comparison type [" + std::to_string(lexpr.type) + "] for <=");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opCompareNotEqual)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;
		if (lexpr.type == liaVariableType::integer)
		{
			bool cond = std::get<int>(lexpr.value) != std::get<int>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::longint)
		{
			bool cond = std::get<long long>(lexpr.value) != std::get<long long>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::boolean)
		{
			bool cond = std::get<bool>(lexpr.value) != std::get<bool>(rexpr.value);
			retvar->value = cond;
		}
		else if (lexpr.type == liaVariableType::string)
		{
			bool cond = std::get<std::string>(lexpr.value) != std::get<std::string>(rexpr.value);
			retvar->value = cond;
		}
		else
		{
			fatalError("Unsupported comparison type for !=");
		}

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opLogicalAnd)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		retvar->type = liaVariableType::boolean;
		retvar->value = std::get<bool>(lexpr.value) && std::get<bool>(rexpr.value);

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opLogicalOr)
	{
		unsigned int bytesTot = 0;
		unsigned int br = 0;
		liaVariable lexpr;
		getExpressionFromCode(chunk, pos + 1, br, &lexpr, env);
		bytesTot += br;

		br = 0;
		liaVariable rexpr;
		getExpressionFromCode(chunk, pos + 1 + bytesTot, br, &rexpr, env);
		bytesTot += br;

		// TODO: short circuit here
		retvar->type = liaVariableType::boolean;
		retvar->value = std::get<bool>(lexpr.value) || std::get<bool>(rexpr.value);

		bytesRead = bytesTot + 1;
	}
	else if (chunk.code[pos] == liaOpcode::opGetDictKeys)
	{
		retvar->type = liaVariableType::array;
		retvar->vlist.clear();

		liaVariable* obj = &(*env)[chunk.code[pos + 1]];

		for (std::map<std::string, liaVariable>::iterator it = obj->vMap.begin(); it != obj->vMap.end(); ++it)
		{
			std::string key = it->first;
			liaVariable s;
			s.type = liaVariableType::string;
			s.value = key;
			retvar->vlist.push_back(s);
		}

		bytesRead = 2;
	}
	else if (chunk.code[pos] == liaOpcode::opGetDictKeysGlobal)
	{
		retvar->type = liaVariableType::array;
		retvar->vlist.clear();

		liaVariable* obj = &globalEnv[chunk.code[pos + 1]];

		for (std::map<std::string, liaVariable>::iterator it = obj->vMap.begin(); it != obj->vMap.end(); ++it)
		{
			std::string key = it->first;
			liaVariable s;
			s.type = liaVariableType::string;
			s.value = key;
			retvar->vlist.push_back(s);
		}

		bytesRead = 2;

	}
	else
	{
		std::stringstream stream;
		stream << std::hex << chunk.code[pos];
		fatalError("Unknown getExpressionFromCode: 0x" + stream.str());
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

void liaVM::executeChunk(liaCodeChunk& chunk, liaVariable& retval, 
	unsigned int funId, unsigned int& parmBytesRead,
	liaCodeChunk* callerChunk,unsigned int callerpos, std::vector<liaVariable>* callerEnv)
{
	unsigned int ip = 0;
	std::vector<liaVariable> runtimeEnv;

	// init runtime env
	unsigned int idx = 0;
	unsigned int bbr = 0;
	for (liaVariable& v : chunk.basicEnv)
	{
		runtimeEnv.push_back(v);
		if (idx < funList[funId].parameters.size())
		{
			getExpressionFromCode(*callerChunk, callerpos + 2 + parmBytesRead, bbr, &runtimeEnv[idx], callerEnv);
			//theParm.name = parm.name;
			parmBytesRead += bbr;
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

			getExpressionFromCode(chunk, ip + 2, br, &runtimeEnv[varId], &runtimeEnv);

			ip += 2 + br;
		}
		else if (opcode == liaOpcode::opSetGlobalVariable)
		{
			const unsigned int varId = chunk.code[ip + 1];
			unsigned int br;
			getExpressionFromCode(chunk, ip + 2, br, &globalEnv[varId], &runtimeEnv);

			ip += 2 + br;
		}
		else if ((opcode == liaOpcode::opSetArrayElement)|| (opcode == liaOpcode::opSetArrayElementGlobal))
		{
			const unsigned int arrId = chunk.code[ip + 1];
			const unsigned int numSubscripts = chunk.code[ip + 2];

			liaVariable* pVar;
			
			if (opcode == liaOpcode::opSetArrayElement)
			{
				pVar = &runtimeEnv[arrId];
			}
			else
			{
				pVar = &globalEnv[arrId];
			}

			if (pVar->type == liaVariableType::array)
			{
				unsigned int bytesRead = 0;
				for (unsigned int sub = 0;sub < numSubscripts;sub++)
				{
					// TODO: check bounds
					unsigned int br = 0;
					getExpressionFromCode(chunk, ip + 3 + bytesRead, br, &internalSub, &runtimeEnv);
					pVar = &pVar->vlist[std::get<int>(internalSub.value)];
					bytesRead += br;
				}

				unsigned int br2 = 0;
				getExpressionFromCode(chunk, ip + 3 + bytesRead, br2, pVar, &runtimeEnv);

				ip += 3 + bytesRead + br2;
			}
			else
			{
				if (numSubscripts != 1)
				{
					fatalError("Maps accept only one subscript");
				}

				unsigned int bytesRead = 0;
				unsigned int br = 0;
				getExpressionFromCode(chunk, ip + 3 + bytesRead, br, &internalSub, &runtimeEnv);
				bytesRead += br;

				std::string key = std::get<std::string>(internalSub.value);

				/*if (pVar->vMap.find(key) == pVar->vMap.end())
				{
					std::string err;
					err += "Key " + key + " not found in dictionary. ";
					err += "Terminating.";
					fatalError(err);
				}*/

				unsigned int br2 = 0;
				getExpressionFromCode(chunk, ip + 3 + bytesRead, br2, &pVar->vMap[key], &runtimeEnv);

				ip += 3 + bytesRead + br2;
			}
		}
		else if ((opcode == liaOpcode::opPostIncrement)|| (opcode == liaOpcode::opPostIncrementGlobal))
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &incAmount, &runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				if (opcode == liaOpcode::opPostIncrement)
				{
					int val = std::get<int>(runtimeEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					runtimeEnv[varId].value = val + inc;
				}
				else
				{
					int val = std::get<int>(globalEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					globalEnv[varId].value = val + inc;
				}
			}
			else if (incType == liaVariableType::longint)
			{
				if (opcode == liaOpcode::opPostIncrement)
				{
					long long val = std::get<long long>(runtimeEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					runtimeEnv[varId].value = val + inc;
				}
				else
				{
					long long val = std::get<long long>(globalEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					globalEnv[varId].value = val + inc;
				}
			}
			else if (incType == liaVariableType::string)
			{
				if (opcode == liaOpcode::opPostIncrement)
				{
					std::string val = std::get<std::string>(runtimeEnv[varId].value);
					std::string inc = std::get<std::string>(incAmount.value);
					runtimeEnv[varId].value = val + inc;
				}
				else
				{
					std::string val = std::get<std::string>(globalEnv[varId].value);
					std::string inc = std::get<std::string>(incAmount.value);
					globalEnv[varId].value = val + inc;
				}
			}
			else if (incType == liaVariableType::array)
			{
				for (unsigned int idx = 0;idx < incAmount.vlist.size();idx++)
				{
					runtimeEnv[varId].vlist.push_back(incAmount.vlist[idx]);
				}
			}
			else
			{
				fatalError("Unhandled postincrement type");
			}

			ip += 2 + bytesRead;
		}
		else if ((opcode == liaOpcode::opPostDecrement)|| (opcode == liaOpcode::opPostDecrementGlobal))
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &incAmount, &runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				if (opcode == liaOpcode::opPostDecrement)
				{
					int val = std::get<int>(runtimeEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					runtimeEnv[varId].value = val - inc;
				}
				else
				{
					int val = std::get<int>(globalEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					globalEnv[varId].value = val - inc;
				}
			}
			else if (incType == liaVariableType::longint)
			{
				if (opcode == liaOpcode::opPostDecrement)
				{
					long long val = std::get<long long>(runtimeEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					runtimeEnv[varId].value = val - inc;
				}
				else
				{
					long long val = std::get<long long>(globalEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					globalEnv[varId].value = val - inc;
				}
			}
			else
			{
				fatalError("Unhandled postdecrement type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opPostMultiply)
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable mulAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &mulAmount, &runtimeEnv);
			liaVariableType incType = mulAmount.type;

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(runtimeEnv[varId].value);
				int inc = std::get<int>(mulAmount.value);
				runtimeEnv[varId].value = val*inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(runtimeEnv[varId].value);
				long long inc = std::get<long long>(mulAmount.value);
				runtimeEnv[varId].value = val*inc;
			}
			else
			{
				fatalError("Unhandled post-multiply type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opPostDivide)
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable mulAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &mulAmount, &runtimeEnv);
			liaVariableType incType = mulAmount.type;

			if (incType == liaVariableType::integer)
			{
				int val = std::get<int>(runtimeEnv[varId].value);
				int inc = std::get<int>(mulAmount.value);
				runtimeEnv[varId].value = val/inc;
			}
			else if (incType == liaVariableType::longint)
			{
				long long val = std::get<long long>(runtimeEnv[varId].value);
				long long inc = std::get<long long>(mulAmount.value);
				runtimeEnv[varId].value = val/inc;
			}
			else
			{
				fatalError("Unhandled post-divide type");
			}

			ip += 2 + bytesRead;
		}
		else if ((opcode == liaOpcode::opPostModulo) || (opcode == liaOpcode::opPostModuloGlobal))
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &incAmount, &runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				if (opcode == liaOpcode::opPostModulo)
				{
					int val = std::get<int>(runtimeEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					runtimeEnv[varId].value = val%inc;
				}
				else
				{
					int val = std::get<int>(globalEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					globalEnv[varId].value = val%inc;
				}
			}
			else if (incType == liaVariableType::longint)
			{
				if (opcode == liaOpcode::opPostModulo)
				{
					long long val = std::get<long long>(runtimeEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					runtimeEnv[varId].value = val%inc;
				}
				else
				{
					long long val = std::get<long long>(globalEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					globalEnv[varId].value = val%inc;
				}
			}
			else
			{
				fatalError("Unhandled postModulo type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opPostBitwiseOr)
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &incAmount, &runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				if (opcode == liaOpcode::opPostBitwiseOr)
				{
					int val = std::get<int>(runtimeEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					runtimeEnv[varId].value = val | inc;
				}
				else
				{
					int val = std::get<int>(globalEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					globalEnv[varId].value = val | inc;
				}
			}
			else if (incType == liaVariableType::longint)
			{
				if (opcode == liaOpcode::opPostBitwiseOr)
				{
					long long val = std::get<long long>(runtimeEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					runtimeEnv[varId].value = val | inc;
				}
				else
				{
					long long val = std::get<long long>(globalEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					globalEnv[varId].value = val | inc;
				}
			}
			else
			{
				fatalError("Unhandled postBitwiseOr type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opPostRshift)
		{
			const unsigned int varId = chunk.code[ip + 1];

			unsigned int bytesRead;
			liaVariable incAmount;
			getExpressionFromCode(chunk, ip + 2, bytesRead, &incAmount, &runtimeEnv);
			liaVariableType incType = incAmount.type;

			if (incType == liaVariableType::integer)
			{
				if (opcode == liaOpcode::opPostRshift)
				{
					int val = std::get<int>(runtimeEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					runtimeEnv[varId].value = val >> inc;
				}
				else
				{
					int val = std::get<int>(globalEnv[varId].value);
					int inc = std::get<int>(incAmount.value);
					globalEnv[varId].value = val >> inc;
				}
			}
			else if (incType == liaVariableType::longint)
			{
				if (opcode == liaOpcode::opPostRshift)
				{
					long long val = std::get<long long>(runtimeEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					runtimeEnv[varId].value = val >> inc;
				}
				else
				{
					long long val = std::get<long long>(globalEnv[varId].value);
					long long inc = std::get<long long>(incAmount.value);
					globalEnv[varId].value = val >> inc;
				}
			}
			else
			{
				fatalError("Unhandled postRshift type");
			}

			ip += 2 + bytesRead;
		}
		else if (opcode == liaOpcode::opJump)
		{
			ip = chunk.code[ip + 1];
		}
		else if ((opcode == liaOpcode::opVarFunctionCall)|| (opcode == liaOpcode::opVarFunctionCallGlobal))
		{
			unsigned int varId = chunk.code[ip + 1];
			unsigned int funId = chunk.code[ip + 2];

			liaVariable* pVar;
			if (opcode == liaOpcode::opVarFunctionCall) pVar = &runtimeEnv[varId];
			else pVar = &globalEnv[varId];

			unsigned int bytesRead = 0;
			if (funId == libMethodId::MethodAdd)
			{
				unsigned int br = 0;
				liaVariable itemToAdd;
				getExpressionFromCode(chunk, ip + 3, br, &itemToAdd, &runtimeEnv);
				pVar->vlist.push_back(itemToAdd);
				bytesRead += br;
			}
			else if (funId == libMethodId::MethodSort)
			{
				if (pVar->vlist.size() != 0)
				{
					if (pVar->vlist[0].type == liaVariableType::integer)
					{
						std::sort(pVar->vlist.begin(), pVar->vlist.end(), compareIntVars);
					}
					else if (pVar->vlist[0].type == liaVariableType::longint)
					{
						std::sort(pVar->vlist.begin(), pVar->vlist.end(), compareLongVars);
					}
					else if (pVar->vlist[0].type == liaVariableType::string)
					{
						std::sort(pVar->vlist.begin(), pVar->vlist.end(), compareStringVars);
					}
					else
					{
						// TODO: unsupported type in sort
						fatalError("Unsupported type in sort");
					}
				}

				bytesRead = 0;
			}
			else
			{
				fatalError("Unknown method called: ["+std::to_string(funId)+"]");
			}

			ip += 3 + bytesRead;
		}
		else if (opcode == liaOpcode::opFunctionCall)
		{
			unsigned int funId = chunk.code[ip + 1];

			liaVariable retVal;
			unsigned int pbr = 0;
			executeChunk(chunks[funId], retVal, funId, pbr, &chunk, ip, &runtimeEnv);

			ip += 2 + pbr;
		}
		else if (opcode == liaOpcode::opReturn)
		{
			if (chunk.code[ip + 1] == 0)
			{
				return;
			}
			else
			{
				unsigned int br2 = 0;
				getExpressionFromCode(chunk, ip + 2, br2, &retval, &runtimeEnv);
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
					else if (chunk.code[pos] == liaOpcode::opGetGlobalVariable)
					{
						unsigned int varId = chunk.code[pos + 1];
						innerPrint(globalEnv[varId]);
						pos += 2;
						bytesRead += 2;
					}
					else if (chunk.code[pos] == liaOpcode::opArraySubscript)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opGetObjectLength)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if ((chunk.code[pos] == liaOpcode::opGetObjectType)|| (chunk.code[pos] == liaOpcode::opGetObjectTypeGlobal))
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opLibFunctionCall)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opArraySubscriptGlobal)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opFunctionCall)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
						innerPrint(v);
						pos += br;
						bytesRead += br;
					}
					else if (chunk.code[pos] == liaOpcode::opMathExpression)
					{
						unsigned int br = 0;
						liaVariable v;
						getExpressionFromCode(chunk, pos, br, &v, &runtimeEnv);
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
		else if (opcode == liaOpcode::opLogicalCondition)
		{
			unsigned int br;
			liaVariable condition;
			getExpressionFromCode(chunk, ip + 2, br, &condition, &runtimeEnv);

			if (std::get<bool>(condition.value)==true)
			{
				ip += 2 + br;
			}
			else
			{
				ip = chunk.code[ip + 1];
			}
		}
		else if (opcode == liaOpcode::opRemoveLocalVariables)
		{
			const unsigned int varId = chunk.code[ip + 1];
			chunk.basicEnv[varId].name += "del" + chunk.getNextSeq();
			runtimeEnv[varId].name += "del" + chunk.getNextSeq();
			ip += 2;
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
			mainFunId = funId;
			//std::vector<liaVariable> dummyParms; // TODO: pass params to main
			funList[mainFunId].parameters.clear(); // for now, we clear the parameters
			liaVariable dummyVal;
			unsigned int dummyBytesRead=0;
			executeChunk(chunks[funId], dummyVal, funId, dummyBytesRead, &chunks[funId], 0, nullptr);
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
	else if (theAst->iName == grammarElement::LongNumber)
	{
		// numeric constant - add it to or find it in the constant pool
		long long constVal = std::stoll(theAst->token_to_string());

		liaVariable theConst;
		theConst.type = liaVariableType::longint;
		theConst.value = constVal;
		unsigned int constantId = findOrAddConstantToConstantPool(theConst);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(constantId);

		return liaVariableType::longint;
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
	else if (theAst->iName == grammarElement::DictInitializer)
	{
		chunk.code.push_back(liaOpcode::opDictInitializer);

		if (theAst->nodes.size() == 0)
		{
			// empty dict
			chunk.code.push_back(0);
		}
		else
		{
			// TODO: finish this
			fatalError("Uns0pported dict initializer");
			/*chunk.code.push_back((unsigned int)theAst->nodes[0]->nodes.size());
			if (theAst->nodes[0]->nodes.size() != 0)
			{
				assert(theAst->nodes[0]->iName == grammarElement::ExpressionList);

				for (auto& el : theAst->nodes[0]->nodes)
				{
					compileExpression(el, chunk);
				}
			}*/
		}

		return liaVariableType::dictionary;
	}
	else if (theAst->iName == grammarElement::ArraySubscript)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;

		if (!findGlobalVariable(varName, varId))
		{
			chunk.findVar(varName, varId);
			chunk.code.push_back(liaOpcode::opArraySubscript);
		}
		else
		{
			chunk.code.push_back(liaOpcode::opArraySubscriptGlobal);
		}

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

		if (findGlobalVariable(varName, varId))
		{
			chunk.code.push_back(liaOpcode::opGetGlobalVariable);
			chunk.code.push_back(varId);
			return globalEnv[varId].type;
		}
		else
		{
			chunk.findVar(varName, varId);
			chunk.code.push_back(liaOpcode::opGetLocalVariable);
			chunk.code.push_back(varId);
			return chunk.basicEnv[varId].type;
		}
	}
	else if (theAst->iName == grammarElement::VariableWithProperty)
	{
		//std::cout << peg::ast_to_s(theAst);

		if (theAst->nodes[0]->iName == grammarElement::ArraySubscript)
		{
			std::string prop = theAst->nodes[1]->token_to_string();
			if (prop == "length")
			{
				std::string varName = theAst->nodes[0]->nodes[0]->token_to_string();
				unsigned int varId;
				bool isGlobal = true;

				if (!findGlobalVariable(varName, varId))
				{
					isGlobal = false;
					chunk.findVar(varName, varId);
				}

				if (!isGlobal) chunk.code.push_back(liaOpcode::opGetObjectLengthOfSubscript);
				else chunk.code.push_back(liaOpcode::opGetObjectLengthOfSubscriptGlobal);

				chunk.code.push_back(varId);

				unsigned int numSubscripts = (unsigned int)(theAst->nodes.size() - 1);
				chunk.code.push_back(numSubscripts);

				for (auto& sub : theAst->nodes[0]->nodes)
				{
					if (sub->iName == grammarElement::Expression)
					{
						compileExpression(sub, chunk);
					}
				}
			}
			else
			{
				fatalError("Unsupported array subscript property " + prop);
			}

			//fatalError("Property of array subscript is unsupported at line "+std::to_string(theAst->line));
		}
		else
		{
			std::string varName = theAst->nodes[0]->token_to_string();
			unsigned int varId;
			bool isGlobal = true;

			if (!findGlobalVariable(varName, varId))
			{
				isGlobal = false;
				chunk.findVar(varName, varId);
			}

			std::string prop = theAst->nodes[1]->token_to_string();

			if (prop == "length")
			{
				if (!isGlobal) chunk.code.push_back(liaOpcode::opGetObjectLength);
				else chunk.code.push_back(liaOpcode::opGetObjectLengthGlobal);
				chunk.code.push_back(varId);
			}
			else if (prop == "typeof")
			{
				if (!isGlobal) chunk.code.push_back(liaOpcode::opGetObjectType);
				else chunk.code.push_back(liaOpcode::opGetObjectTypeGlobal);
				chunk.code.push_back(varId);
			}
			else if (prop == "keys")
			{
				if (!isGlobal) chunk.code.push_back(liaOpcode::opGetDictKeys);
				else chunk.code.push_back(liaOpcode::opGetDictKeysGlobal);
				chunk.code.push_back(varId);
				return liaVariableType::array;
			}
			else
			{
				fatalError("Unhandled variable property " + prop);
			}
		}

		return liaVariableType::integer;
	}
	else if (theAst->iName == grammarElement::VariableWithFunction)
	{
		//std::cout << peg::ast_to_s(theAst);

		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;
		bool varIsGlobal = true;

		if (!findGlobalVariable(varName, varId))
		{
			varIsGlobal = false;
			if (!chunk.findVar(varName, varId))
			{
				// TODO: non-existant var function call
				fatalError("Variable Named " + varName + " not declared at line " + std::to_string(theAst->line));
			}
		}

		unsigned int funId = theAst->nodes[1]->tokenId;
		if (funId == libMethodId::MethodSplit)
		{
			if (!varIsGlobal) chunk.code.push_back(liaOpcode::opVarFunctionCall);
			else chunk.code.push_back(liaOpcode::opVarFunctionCallGlobal);
			chunk.code.push_back(varId);
			chunk.code.push_back(funId);
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
			return liaVariableType::array;
		}
		else if (funId == libMethodId::MethodFind)
		{
			if (!varIsGlobal) chunk.code.push_back(liaOpcode::opVarFunctionCall);
			else chunk.code.push_back(liaOpcode::opVarFunctionCallGlobal);
			chunk.code.push_back(varId);
			chunk.code.push_back(funId);
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
			return liaVariableType::integer;
		}
		else if (funId == libMethodId::MethodFindkey)
		{
			if (!varIsGlobal) chunk.code.push_back(liaOpcode::opVarFunctionCall);
			else chunk.code.push_back(liaOpcode::opVarFunctionCallGlobal);
			chunk.code.push_back(varId);
			chunk.code.push_back(funId);
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
			return liaVariableType::integer;
		}
		else if (funId == libMethodId::MethodSlice)
		{
			if (!varIsGlobal) chunk.code.push_back(liaOpcode::opVarFunctionCall);
			else chunk.code.push_back(liaOpcode::opVarFunctionCallGlobal);
			chunk.code.push_back(varId);
			chunk.code.push_back(funId);
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
			compileExpression(theAst->nodes[2]->nodes[1], chunk);
			return liaVariableType::string; // TODO: can be string or array
		}
		else
		{
			fatalError("Unknown lib method ["+std::to_string(funId)+"] at line "+std::to_string(theAst->line));
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
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionToLong)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionToLong);
			compileExpression(theAst->nodes[1]->nodes[0], chunk);
			return liaVariableType::longint;
		}
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionToString)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionToString);
			compileExpression(theAst->nodes[1]->nodes[0], chunk);
			return liaVariableType::string;
		}
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionLsqrt)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionLsqrt);
			compileExpression(theAst->nodes[1]->nodes[0], chunk);
			return liaVariableType::integer;
		}
		else if (theAst->nodes[0]->tokenId == StdFunctionId::FunctionRnd)
		{
			chunk.code.push_back(liaOpcode::opLibFunctionCall);
			chunk.code.push_back(StdFunctionId::FunctionRnd);
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
	else if (theAst->iName == grammarElement::NotExpression)
	{
		//std::cout << "Notz " << peg::ast_to_s(theAst);

		chunk.code.push_back(liaOpcode::opNot);
		compileExpression(theAst->nodes[0], chunk);

		return liaVariableType::boolean;
	}
	else if (theAst->iName == grammarElement::MinusExpression)
	{
		// TODO: handle longs

		chunk.code.push_back(liaOpcode::opMathExpression);
		chunk.code.push_back(1);

		liaVariable minusOne;
		minusOne.type = liaVariableType::integer;
		minusOne.value = -1;
		constantPool.push_back(minusOne);
		const unsigned int minusOneConstId = (unsigned int)(constantPool.size() - 1);

		chunk.code.push_back(liaOpcode::opConstant);
		chunk.code.push_back(minusOneConstId);

		chunk.code.push_back(liaOpcode::opMultiply);
		compileExpression(theAst->nodes[0], chunk);

		return liaVariableType::integer;
	}
	else if ((theAst->iName == grammarElement::Expression) && ((theAst->nodes.size() % 2) == 1) && (theAst->nodes.size() > 2))
	{
		//std::cout << peg::ast_to_s(theAst);

		chunk.code.push_back(liaOpcode::opMathExpression);
		chunk.code.push_back((unsigned int)((theAst->nodes.size()-1)/2));

		int nodepos = 1;
		compileExpression(theAst->nodes[0], chunk);
		while (nodepos < theAst->nodes.size())
		{
			std::string mathOperator = theAst->nodes[nodepos]->token_to_string();

			if (mathOperator == "+")
			{
				chunk.code.push_back(liaOpcode::opAdd);
			}
			else if (mathOperator == "-")
			{
				chunk.code.push_back(liaOpcode::opSubtract);
			}
			else if (mathOperator == "*")
			{
				chunk.code.push_back(liaOpcode::opMultiply);
			}
			else if (mathOperator == "/")
			{
				chunk.code.push_back(liaOpcode::opDivide);
			}
			else if (mathOperator == "%")
			{
				chunk.code.push_back(liaOpcode::opModulo);
			}
			else if (mathOperator == "^")
			{
				chunk.code.push_back(liaOpcode::opXor);
			}
			else
			{
				fatalError("Unsupported math operator " + mathOperator);
			}

			compileExpression(theAst->nodes[nodepos + 1], chunk);
			nodepos += 2;
		}
		//compileExpression(theAst->nodes[nodepos - 1], chunk);
	}
	else if ((theAst->iName == grammarElement::InnerExpression) || (theAst->iName == grammarElement::Expression))
	{
		return compileExpression(theAst->nodes[0], chunk);
	}
	else
	{
		fatalError("Unknown expression type:"+theAst->name+" at line "+std::to_string(theAst->line));
	}

	// "unreachable"
	return liaVariableType::integer;
}

inline bool liaVM::findGlobalVariable(std::string& vname, unsigned int& varId)
{
	unsigned int idx = 0;
	while (idx < globalEnv.size())
	{
		if (globalEnv[idx].name == vname)
		{
			varId = idx;
			return true;
		}
		idx += 1;
	}

	return false;
}

void liaVM::compileVarDeclStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (findGlobalVariable(varName, varId))
	{
		chunk.code.push_back(liaOpcode::opSetGlobalVariable);
		chunk.code.push_back(varId);
		compileExpression(theAst->nodes[1], chunk);
	}
	else
	{
		if (!chunk.findVar(varName, varId))
		{
			liaVariable newVar;
			newVar.name = varName;
			chunk.basicEnv.push_back(newVar);
			varId = (unsigned int)(chunk.basicEnv.size() - 1);
		}

		assert(theAst->nodes[1]->iName == grammarElement::Expression);

		chunk.code.push_back(liaOpcode::opSetLocalVariable);
		chunk.code.push_back(varId);

		liaVariableType vartype = compileExpression(theAst->nodes[1], chunk);
		chunk.basicEnv[varId].type = vartype;
	}
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

void liaVM::compileInnerCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);
	//std::cout << std::endl;

	if ((theAst->iName == grammarElement::Condition) || (theAst->iName == grammarElement::InnerCondition))
	{
		if (theAst->nodes.size() == 1)
		{
			return compileInnerCondition(theAst->nodes[0], chunk);
		}
		else if (theAst->nodes.size() == 3)
		{
			// [InnerCondition CondOperator Condition] or [Expression Relop Expression]
			//std::cout << "Size 3 " << theAst->name << " ns: " << theAst->nodes.size() << std::endl;
			if ((theAst->nodes[0]->iName == grammarElement::InnerCondition) || (theAst->nodes[0]->iName == grammarElement::Condition))
			{
				// condition &&/|| condition

				std::shared_ptr<peg::Ast> lExpr = theAst->nodes[0];
				std::string condOperator = theAst->nodes[1]->token_to_string();
				std::shared_ptr<peg::Ast> rExpr = theAst->nodes[2];

				if (condOperator == "&&")
				{
					chunk.code.push_back(liaOpcode::opLogicalAnd);
				}
				else if (condOperator == "||")
				{
					chunk.code.push_back(liaOpcode::opLogicalOr);
				}
				else
				{
					fatalError("Unimplemented logical condition " + condOperator);
				}

				compileInnerCondition(lExpr, chunk);
				compileInnerCondition(rExpr, chunk);
			}
			else if (theAst->nodes[0]->iName == grammarElement::Expression)
			{
				std::shared_ptr<peg::Ast> lExpr = theAst->nodes[0];
				int relopid = theAst->nodes[1]->tokenId;
				std::shared_ptr<peg::Ast> rExpr = theAst->nodes[2];

				if (relopid == relopId::RelopLess)
				{
					chunk.code.push_back(liaOpcode::opCompareLess);
				}
				else if (relopid == relopId::RelopGreater)
				{
					chunk.code.push_back(liaOpcode::opCompareGreater);
				}
				else if (relopid == relopId::RelopEqual)
				{
					chunk.code.push_back(liaOpcode::opCompareEqual);
				}
				else if (relopid == relopId::RelopNotEqual)
				{
					chunk.code.push_back(liaOpcode::opCompareNotEqual);
				}
				else if (relopid == relopId::RelopGreaterEqual)
				{
					chunk.code.push_back(liaOpcode::opCompareGreaterEqual);
				}
				else if (relopid == relopId::RelopLessEqual)
				{
					chunk.code.push_back(liaOpcode::opCompareLessEqual);
				}
				else
				{
					fatalError("Unhandled relop [" + std::to_string(relopid) + "] at line " + std::to_string(theAst->line));
				}

				compileExpression(lExpr, chunk);
				compileExpression(rExpr, chunk);
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
			compileExpression(theAst->nodes[0], chunk);
		}
	}

}

unsigned int liaVM::compileCondition(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	chunk.code.push_back(liaOpcode::opLogicalCondition);
	unsigned int jumpPtr = (unsigned int)chunk.code.size();
	chunk.code.push_back(0); // space for jump address

	compileInnerCondition(theAst, chunk);

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

	if (theAst->nodes[0]->iName == grammarElement::VariableName)
	{
		std::string varName = theAst->nodes[0]->token_to_string();
		unsigned int varId;

		if (findGlobalVariable(varName, varId))
		{
			chunk.code.push_back(liaOpcode::opPostIncrementGlobal);
			chunk.code.push_back(varId);
			compileExpression(theAst->nodes[1], chunk);
		}
		else
		{
			if (!chunk.findVar(varName, varId))
			{
				// var not declared before
				fatalError("Trying to postincrement undeclared variable " + varName);
			}

			chunk.code.push_back(liaOpcode::opPostIncrement);
			chunk.code.push_back(varId);

			compileExpression(theAst->nodes[1], chunk);
		}
	}
	else if (theAst->nodes[0]->iName==grammarElement::ArraySubscript)
	{
		//fatalError("Unhandled postincrement of array subscript");
		unsigned int arrId;
		std::string arrName = theAst->nodes[0]->nodes[0]->token_to_string();

		if (findGlobalVariable(arrName, arrId))
		{
			chunk.code.push_back(liaOpcode::opSetArrayElementGlobal);
			chunk.code.push_back(arrId);

			unsigned int numSubscripts = (unsigned int)(theAst->nodes[0]->nodes.size() - 1);
			chunk.code.push_back(numSubscripts);

			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
			}

			// = arr[x][y][...][z]+expr;
			chunk.code.push_back(liaOpcode::opMathExpression);
			chunk.code.push_back(1);
			chunk.code.push_back(liaOpcode::opArraySubscript);
			chunk.code.push_back(arrId);
			chunk.code.push_back(numSubscripts);
			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
			}
			chunk.code.push_back(liaOpcode::opAdd);
			compileExpression(theAst->nodes[1], chunk);
		}
		else
		{
			if (!chunk.findVar(arrName, arrId))
			{
				// TODO: non-existant array assignment
				fatalError("Array named " + arrName + " not declared at line " + std::to_string(theAst->line));
			}

			assert(theAst->nodes[1]->iName == grammarElement::Expression);

			chunk.code.push_back(liaOpcode::opSetArrayElement);
			chunk.code.push_back(arrId);

			unsigned int numSubscripts = (unsigned int)(theAst->nodes[0]->nodes.size() - 1);
			chunk.code.push_back(numSubscripts);

			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
			}

			// = arr[x][y][...][z]+expr;
			chunk.code.push_back(liaOpcode::opMathExpression);
			chunk.code.push_back(1);
			chunk.code.push_back(liaOpcode::opArraySubscript);
			chunk.code.push_back(arrId);
			chunk.code.push_back(numSubscripts);
			for (unsigned int sub = 0;sub < numSubscripts;sub++)
			{
				compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
			}
			chunk.code.push_back(liaOpcode::opAdd);
			compileExpression(theAst->nodes[1], chunk);
		}
	}
	else
	{
		fatalError("Unhandled postincrement type");
	}
}

void liaVM::compilePostDecrementStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (findGlobalVariable(varName, varId))
	{
		chunk.code.push_back(liaOpcode::opPostDecrementGlobal);
		chunk.code.push_back(varId);
		compileExpression(theAst->nodes[1], chunk);
	}
	else
	{
		if (!chunk.findVar(varName, varId))
		{
			// var not declared before
			fatalError("Trying to postdecrement undeclared variable " + varName);
		}

		chunk.code.push_back(liaOpcode::opPostDecrement);
		chunk.code.push_back(varId);

		compileExpression(theAst->nodes[1], chunk);
	}
}

void liaVM::compileArrayAssignmentStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->nodes[0]->token_to_string();
	unsigned int varId;

	if (findGlobalVariable(varName, varId))
	{
		chunk.code.push_back(liaOpcode::opSetArrayElementGlobal);
		chunk.code.push_back(varId);

		unsigned int numSubscripts = (unsigned int)(theAst->nodes[0]->nodes.size() - 1);
		chunk.code.push_back(numSubscripts);

		for (unsigned int sub = 0;sub < numSubscripts;sub++)
		{
			compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
		}

		liaVariableType vartype = compileExpression(theAst->nodes[1], chunk);
	}
	else
	{
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
			compileExpression(theAst->nodes[0]->nodes[sub + 1], chunk);
		}

		liaVariableType vartype = compileExpression(theAst->nodes[1], chunk);
	}
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
	std::string iteratedName;

	// tmpVarName already in env?
	unsigned int dummyId;
	if (chunk.findVar(tmpVarName, dummyId))
	{
		fatalError("You can't use the same variable name ("+tmpVarName+") for foreaches in the same chunk at line "+std::to_string(theAst->line));
	}
	
	bool iteratedIsGlobal = false;
	unsigned int iteratedId;

	// foreach (node in arr)
	if (theAst->nodes[1]->nodes[0]->nodes[0]->iName == grammarElement::VariableName)
	{
		iteratedName=theAst->nodes[1]->nodes[0]->nodes[0]->token_to_string();

		if (!findGlobalVariable(iteratedName, iteratedId))
		{
			if (!chunk.findVar(iteratedName, iteratedId))
			{
				fatalError("Variable named " + iteratedName + " not declared previously, at line " + std::to_string(theAst->line));
			}
		}
		else
		{
			iteratedIsGlobal = true;
		}
	}
	// foreach (node in map.keys)
	else if (theAst->nodes[1]->nodes[0]->nodes[0]->iName == grammarElement::VariableWithProperty)
	{
		// arr=m.keys
		liaVariable keysVar;
		keysVar.name = "keys_internal" + std::to_string(chunk.getNextSeq());
		keysVar.type = liaVariableType::array;
		chunk.basicEnv.push_back(keysVar);
		iteratedId = (unsigned int)(chunk.basicEnv.size() - 1);

		unsigned int mapId;
		bool mapIsGlobal = false;
		if (!chunk.findVar(theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0]->token_to_string(), mapId))
		{
			std::string mapNamez = theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0]->token_to_string();
			if (!findGlobalVariable(mapNamez, mapId))
			{
				fatalError("Iterated .keys name not found");
			}
			else
			{
				mapIsGlobal = true;
			}
		}
		
		chunk.code.push_back(liaOpcode::opSetLocalVariable);
		chunk.code.push_back(iteratedId);
		if (!mapIsGlobal) chunk.code.push_back(liaOpcode::opGetDictKeys);
		else chunk.code.push_back(liaOpcode::opGetDictKeysGlobal);
		chunk.code.push_back(mapId);
	}
	// foreach (node in arr[x]...[z])
	else if (theAst->nodes[1]->nodes[0]->nodes[0]->iName == grammarElement::ArraySubscript)
	{
		// arr=arr[x]...[z]
		liaVariable subsVar;
		subsVar.name = "subs_internal" + std::to_string(chunk.getNextSeq());
		subsVar.type = liaVariableType::array;
		chunk.basicEnv.push_back(subsVar);
		iteratedId = (unsigned int)(chunk.basicEnv.size() - 1);

		unsigned int arrId;
		bool arrIsGlobal = false;
		if (!chunk.findVar(theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0]->token_to_string(), arrId))
		{
			std::string arrNamez = theAst->nodes[1]->nodes[0]->nodes[0]->nodes[0]->token_to_string();
			if (!findGlobalVariable(arrNamez, arrId))
			{
				fatalError("Iterated array name not found");
			}
			else
			{
				arrIsGlobal = true;
			}
		}

		chunk.code.push_back(liaOpcode::opSetLocalVariable);
		chunk.code.push_back(iteratedId);
		if (!arrIsGlobal) chunk.code.push_back(liaOpcode::opArraySubscript);
		else chunk.code.push_back(liaOpcode::opArraySubscriptGlobal);
		chunk.code.push_back(arrId);
		unsigned int numSubscripts = (unsigned int)(theAst->nodes[1]->nodes[0]->nodes[0]->nodes.size() - 1);
		chunk.code.push_back(numSubscripts);

		for (auto& sub : theAst->nodes[1]->nodes[0]->nodes[0]->nodes)
		{
			if (sub->iName == grammarElement::Expression)
			{
				compileExpression(sub, chunk);
			}
		}

		//fatalError("Unsupp0rted array sub iterated");
	}
	else
	{
		fatalError("Unsupp0rted foreach iterated");
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

	chunk.code.push_back(liaOpcode::opLogicalCondition);
	unsigned int exitLoopPtr = (unsigned int)chunk.code.size();
	chunk.code.push_back(0); // space for jump address

	chunk.code.push_back(liaOpcode::opCompareLess);
	chunk.code.push_back(liaOpcode::opGetLocalVariable);
	chunk.code.push_back(idxVarId);
	if (iteratedIsGlobal==false) chunk.code.push_back(liaOpcode::opGetObjectLength);
	else chunk.code.push_back(liaOpcode::opGetObjectLengthGlobal);
	chunk.code.push_back(iteratedId);

	// tmpVar=arr[idx]
	liaVariable tmpVar;
	tmpVar.name = tmpVarName;
	chunk.basicEnv.push_back(tmpVar);
	unsigned int tmpVarId = (unsigned int)(chunk.basicEnv.size() - 1);

	chunk.code.push_back(liaOpcode::opSetLocalVariable);
	chunk.code.push_back(tmpVarId);

	if (iteratedIsGlobal == false) chunk.code.push_back(liaOpcode::opArraySubscript);
	else chunk.code.push_back(liaOpcode::opArraySubscriptGlobal);
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

	// TODO: kill the temporary variable in any way
	//chunk.code.push_back(liaOpcode::opRemoveLocalVariables);
	//chunk.code.push_back(tmpVarId);
}

void liaVM::compileVarFunctionCallStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	//std::cout << peg::ast_to_s(theAst);

	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (findGlobalVariable(varName, varId))
	{
		unsigned int funId = theAst->nodes[1]->tokenId;

		chunk.code.push_back(liaOpcode::opVarFunctionCallGlobal);
		chunk.code.push_back(varId);
		chunk.code.push_back(funId);

		if (theAst->nodes.size() > 2)
		{
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
		}
	}
	else
	{
		if (!chunk.findVar(varName, varId))
		{
			// TODO: non-existant var function call
			fatalError("Variable named " + varName + " not declared at line " + std::to_string(theAst->line));
		}

		unsigned int funId = theAst->nodes[1]->tokenId;

		chunk.code.push_back(liaOpcode::opVarFunctionCall);
		chunk.code.push_back(varId);
		chunk.code.push_back(funId);

		if (theAst->nodes.size() > 2)
		{
			compileExpression(theAst->nodes[2]->nodes[0], chunk);
		}
	}
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

void liaVM::compileMultiplyStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to post-multiply undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostMultiply);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compileDivideStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to post-divide undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostDivide);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compileModuloStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to post-modulo undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostModulo);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compileBitwiseOrStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to post-bitwise or undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostBitwiseOr);
	chunk.code.push_back(varId);

	compileExpression(theAst->nodes[1], chunk);
}

void liaVM::compileRshiftStatement(const std::shared_ptr<peg::Ast>& theAst, liaCodeChunk& chunk)
{
	std::string varName = theAst->nodes[0]->token_to_string();
	unsigned int varId;

	if (!chunk.findVar(varName, varId))
	{
		// var not declared before
		fatalError("Trying to post-rshift undeclared variable " + varName);
	}

	chunk.code.push_back(liaOpcode::opPostRshift);
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
		else if (stmt->nodes[0]->iName == grammarElement::DecrementStmt)
		{
			compilePostDecrementStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::MultiplyStmt)
		{
			compileMultiplyStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::DivideStmt)
		{
			compileDivideStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::ModuloStmt)
		{
			compileModuloStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::LogicalOrStmt)
		{
			compileBitwiseOrStatement(stmt->nodes[0], chunk);
		}
		else if (stmt->nodes[0]->iName == grammarElement::RshiftStmt)
		{
			compileRshiftStatement(stmt->nodes[0], chunk);
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

int liaVM::getGlobalVariables(liaEnvironment& gEnv)
{
	for (auto& v : gEnv.varMap)
	{
		globalEnv.push_back(v.second);
	}

	return 0;
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

		std::cout << "Compiling:" << f.second.name << std::endl;
		compileCodeBlock(f.second.functionCodeBlockAst,chunk);
		chunks.push_back(chunk);
	}

	printCompilationStats();

	return 0;
}

liaVM::~liaVM()
{
}
