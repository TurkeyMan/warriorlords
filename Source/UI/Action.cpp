#include "Warlords.h"
#include "MFStringCache.h"

#include "Action.h"
#include "Entity.h"

Factory<uiAction> uiActionManager::actionFactory;
MFObjectPool uiActionManager::actionTypePool;
HashList<uiActionManager::ActionType> uiActionManager::actionRegistry;

const char * const gpDelimeters = " \t;:',()[]{}+-*/%\\|=";
MFObjectPool gActionPool(sizeof(uiActionScript_Action), 128, 128);

MFObjectPool uiRuntimeArgs::runtimeArgPool;
uiRuntimeArgs::Argument *uiRuntimeArgs::pArgPool = NULL;
uiRuntimeArgs::Argument *uiRuntimeArgs::pCurrent = NULL;
int uiRuntimeArgs::poolSize = 256;

bool uiRuntimeArgs::IsNumeric(int index)
{
	if(index == -1)
	{
		for(int a=0; a<numArgs; ++a)
		{
			if(!IsNumeric(a))
				return false;
		}
		return true;
	}

	Argument &arg = pArgs[index];

	return arg.type == AT_Int || arg.type == AT_Float ||
			(arg.type == AT_String && arg.string.IsNumeric()) ||
			(arg.type == AT_Array && arg.pArray->IsNumeric());
}

uiRuntimeArgs::Argument *uiRuntimeArgs::Get(int index)
{
	return pArgs + index;
}

MFString uiRuntimeArgs::GetString(int index)
{
	Argument &arg = pArgs[index];

	switch(arg.type)
	{
		case AT_String:
			return arg.string;
		case AT_Int:
		{
			MFString str;
			str.FromInt(arg.iValue);
			return str;
		}
		case AT_Float:
		{
			MFString str;
			str.FromFloat(arg.fValue);
			return str;
		}
	}
	return NULL;
}

bool uiRuntimeArgs::GetBool(int index)
{
	Argument &arg = pArgs[index];

	switch(arg.type)
	{
		case AT_String:
			if(!MFString_CaseCmp(arg.string.CStr(), "true"))
				return true;
			return arg.string.ToInt() != 0;
		case AT_Int:
			return arg.iValue != 0;
		case AT_Float:
			return arg.fValue != 0.f;
	}
	return false;
}

int uiRuntimeArgs::GetInt(int index)
{
	Argument &arg = pArgs[index];

	switch(arg.type)
	{
		case AT_String:
			return arg.string.ToInt();
		case AT_Int:
			return arg.iValue;
		case AT_Float:
			return (int)arg.fValue;
	}
	return 0;
}

float uiRuntimeArgs::GetFloat(int index)
{
	Argument &arg = pArgs[index];

	switch(arg.type)
	{
		case AT_String:
			return arg.string.ToFloat();
		case AT_Int:
			return (float)arg.iValue;
		case AT_Float:
			return arg.fValue;
	}
	return 0.f;
}

MFVector uiRuntimeArgs::GetVector(int index)
{
	Argument &arg = pArgs[index];
	MFVector v = MFVector::zero;

	if(arg.type == AT_Array)
	{
		switch(arg.pArray->numArgs)
		{
			default:
			case 4: v.w = arg.pArray->GetFloat(3);
			case 3: v.z = arg.pArray->GetFloat(2);
			case 2: v.y = arg.pArray->GetFloat(1);
			case 1: v.x = arg.pArray->GetFloat(0);
				break;
		}
	}
	else
	{
		float f = GetFloat(index);
		v.x = f;
		v.y = f;
		v.z = f;
		v.w = f;
	}

	return v;
}

uiRuntimeArgs *uiRuntimeArgs::GetArray(int index)
{
	Argument &arg = pArgs[index];
	if(arg.type == AT_Array)
		return arg.pArray;
	return NULL;
}

void uiRuntimeArgs::Set(int index, Argument *pArg)
{
	pArgs[index].type = pArg->type;
	pArgs[index].iValue = pArg->iValue;
	pArgs[index].string = pArg->string;
}

void uiRuntimeArgs::SetValue(int index, MFString str, bool bString)
{
	if(bString || !str.IsNumeric())
	{
		pArgs[index].string = str;
		pArgs[index].type = AT_String;
	}
	else if(MFString_Chr(str.CStr(), '.'))
	{
		pArgs[index].fValue = str.ToFloat();
		pArgs[index].type = AT_Float;
	}
	else
	{
		pArgs[index].iValue = str.ToInt();
		pArgs[index].type = AT_Int;
	}
}

void uiRuntimeArgs::SetArray(int index, uiRuntimeArgs *pArray)
{
	pArgs[index].pArray = pArray;
	pArgs[index].type = AT_Array;
}

void uiRuntimeArgs::NegateValue(int index)
{
	if(index == -1)
	{
		for(int a=0; a<numArgs; ++a)
			NegateValue(a);
	}
	else
	{
		if(pArgs[index].type == AT_Int)
			pArgs[index].iValue = -pArgs[index].iValue;
		else if(pArgs[index].type == AT_Float)
			pArgs[index].fValue = -pArgs[index].fValue;
		else if(pArgs[index].type == AT_Array)
			pArgs[index].pArray->NegateValue();
	}
}

void uiRuntimeArgs::SetPercentage(int index, float containerSize)
{
	if(pArgs[index].type == AT_Int)
		pArgs[index].fValue = (float)pArgs[index].iValue * (1.f/100.f) * containerSize;
	else if(pArgs[index].type == AT_Float)
		pArgs[index].fValue = pArgs[index].fValue * (1.f/100.f) * containerSize;
	pArgs[index].type = AT_Float;
}

void uiActionManager::InitManager()
{
	actionTypePool.Init(sizeof(ActionType), 256, 32);
	actionRegistry.Create(64, 256, 32);
}

void uiActionManager::DeinitManager()
{
	actionRegistry.Destroy();
	actionTypePool.Deinit();
}

void uiActionManager::Init()
{
	actions.Create(256, 512, 512);
	metrics.Create(256, 512, 512);

	activeDeferredActions.Init("Deferred Action Pool", 1024);
}

void uiActionManager::Deinit()
{
	activeDeferredActions.Deinit();

	metrics.Destroy();
	actions.Destroy();
}

void uiActionManager::Update()
{
	ActiveAction **ppI = activeDeferredActions.Begin();
	for(; *ppI; ++ppI)
	{
		ActiveAction *pActive = *ppI;
		if(pActive->pAction->Update())
		{
			delete pActive->pAction;

			uiActionScript_Action *pNext = pActive->pNext;
			uiEntity *pEntity = pActive->pEntity;
			activeDeferredActions.Destroy(ppI);

			if(pNext)
				Continue(pNext, pEntity);
		}
	}
}

void uiActionManager::RegisterProperty(const char *pPropertyName, GetPropertyHandler *pGetHandler, InstantActionHandler *pSetHandler, FactoryType *pEntityType)
{
	ActionType *pType = (ActionType*)actionTypePool.Alloc();
	pType->pName = pPropertyName;
	pType->pGetHandler = pGetHandler;
	pType->pSetHandler = pSetHandler;
	pType->pEntityType = pEntityType;

	actionRegistry.Add(pType, pType->pName);
}

void uiActionManager::RegisterInstantAction(const char *pActionName, InstantActionHandler *pActionHandler, FactoryType *pEntityType)
{
	ActionType *pType = (ActionType*)actionTypePool.Alloc();
	pType->pName = pActionName;
	pType->pGetHandler = NULL;
	pType->pSetHandler = pActionHandler;
	pType->pEntityType = pEntityType;

	actionRegistry.Add(pType, pType->pName);
}

void uiActionManager::RegisterDeferredAction(const char *pActionName, Factory_CreateFunc *pCreateFunc, FactoryType *pEntityType)
{
	ActionType *pType = (ActionType*)actionTypePool.Alloc();
	pType->pName = pActionName;
	pType->pGetHandler = NULL;
	pType->pSetHandler = NULL;
	pType->pEntityType = pEntityType;

	actionFactory.RegisterType(pType->pName, pCreateFunc);

	actionRegistry.Add(pType, pType->pName);
}


void uiActionManager::RunScript(uiActionScript *pScript, uiEntity *pEntity)
{
	Continue(pScript->pAction, pEntity);
}

bool uiActionManager::RunAction(const char *pAction, uiRuntimeArgs *pArgs, uiEntity *pActionEntity, uiActionScript_Action *pNextAction, uiEntity *pNextEntity)
{
	bool bFinished = true;

	uiActionScript *pScript = actions.Find(pAction);
	if(pScript)
	{
		RunScript(pScript, pActionEntity);
		return true;
	}

	ActionType *pType = actionRegistry.Find(pAction);
	if(!pType)
		return true;

	MFDebug_Assert(pType->pEntityType && pActionEntity->IsType(pType->pEntityType), MFStr("Can not perform action '%s' on this type of entity!", pAction));

	if(pType->pSetHandler)
	{
		pType->pSetHandler(pActionEntity, pArgs);
	}
	else
	{
		uiAction *pDeferred = actionFactory.Create(pAction);
		if(pDeferred)
		{
			pDeferred->pEntity = pActionEntity;
			pDeferred->Init(pArgs);
			bFinished = pDeferred->Update();

			// if it didn't complete this frame, we'll add it to the active list.
			if(!bFinished)
			{
				ActiveAction *pNew = activeDeferredActions.Create();
				pNew->pAction = pDeferred;
				pNew->pEntity = pNextEntity;
				pNew->pNext = pNextAction;
			}
			else
			{
				delete pDeferred;
			}
		}
	}

	return bFinished;
}

MFString uiActionManager::GetEntityProperty(uiEntity *pEntity, const char *pProperty)
{
	ActionType *pType = actionRegistry.Find(pProperty);
	if(pType && pType->pGetHandler)
		return pType->pGetHandler(pEntity);
	return NULL;
}

void uiActionManager::SetEntityProperty(uiEntity *pEntity, const char *pProperty, const char *pArgs)
{
	ActionType *pType = actionRegistry.Find(pProperty);
	if(pType && pType->pSetHandler)
	{
		int numTokens;
		uiActionScript_Token *pTokens = (uiActionScript_Token*)Lex(pArgs, &numTokens);
		uiRuntimeArgs *pRuntimeArgs = ResolveArguments(pTokens, numTokens, pEntity->GetContainerSize());
		pType->pSetHandler(pEntity, pRuntimeArgs);
		pRuntimeArgs->Release();
		MFHeap_Free(pTokens);
	}
}

void uiActionManager::SetEntityProperty(uiEntity *pEntity, const char *pProperty, uiRuntimeArgs *pArguments)
{
	ActionType *pType = actionRegistry.Find(pProperty);
	if(pType && pType->pSetHandler)
		pType->pSetHandler(pEntity, pArguments);
}

void uiActionManager::Continue(uiActionScript_Action *pNext, uiEntity *pEntity)
{
	// check if action specifies an entity
	uiEntity *pActionEntity = pNext->pEntity ? pEntity->GetEntityManager()->Find(pNext->pEntity) : pEntity;

	bool bFinished = true;
	if(pActionEntity)
	{
		// resolve action parameters
		uiRuntimeArgs *pArgs = ResolveArguments(pNext->pArgs, pNext->numArgs, pActionEntity->GetContainerSize());

		bFinished = RunAction(pNext->pAction, pArgs, pActionEntity, pNext->bWaitComplete ? pNext->pNext : NULL, pEntity);
	}

	if(pNext->pNext && (bFinished || !pNext->bWaitComplete))
		Continue(pNext->pNext, pEntity);
}

void *uiActionManager::Lex(const char *pAction, int *pNumTokens, int preBytes)
{
	MFArray<uiActionScript_Token> tokens;
	MFStringCache *pStringCache = MFStringCache_Create(2048);

	// tokenise the expression
	while(*pAction)
	{
		uiActionScript_Token::TokenType type = uiActionScript_Token::TT_Unknown;
		const char *pNewToken;

		if(MFIsWhite(*pAction))
		{
			// skip past any white space
			pAction = MFSkipWhite(pAction);
			continue;
		}
		else if(*pAction == '\'')
		{
			// scan for the closing quote character
			const char *pEndQuote = MFString_Chr(++pAction, '\'');
			if(!pEndQuote)
				break;

			// copy string into token buffer
			int numChars = (int)(pEndQuote - pAction);
			pNewToken = MFStrN(pAction, numChars);
			type = uiActionScript_Token::TT_String;

			pAction = pEndQuote + 1;
		}
		else if(MFString_Chr(gpDelimeters, *pAction))
		{
			// we have some form of operator
			pNewToken = MFStrN(pAction, 1);

			// check if it's an arithmetic operator
			if(*pAction == '+' || *pAction == '-' || *pAction == '*' || *pAction == '/' || *pAction == '%')
				type = uiActionScript_Token::TT_Arithmetic;
			else
				type = uiActionScript_Token::TT_Syntax;

			++pAction;
		}
		else
		{
			const char *pStart = pAction;
			while(*++pAction && !MFString_Chr(gpDelimeters, *pAction)) {}

			// we have a number or an identifier
			pNewToken = MFStrN(pStart, pAction - pStart);
			type = MFString_IsNumber(pNewToken) ? uiActionScript_Token::TT_Number : uiActionScript_Token::TT_Identifier;
		}

		if(type != uiActionScript_Token::TT_Unknown)
		{
			uiActionScript_Token &token = tokens.push();
			token.type = type;
			while(!(token.pToken = MFStringCache_Add(pStringCache, pNewToken)))
			{
				int bytes = MFStringCache_GetSize(pStringCache);
				MFStringCache *pNew = MFStringCache_Create(bytes * 2);

				const char *pOldPtr = MFStringCache_GetCache(pStringCache);
				char *pNewPtr = MFStringCache_GetCache(pNew);

				MFCopyMemory(pNewPtr, pOldPtr, bytes);
				MFStringCache_SetSize(pNew, bytes);

				MFStringCache_Destroy(pStringCache);
				pStringCache = pNew;

				int64 difference = (int64)(size_t)pNewPtr - (int64)(size_t)pOldPtr;
				for(int a=0; a<tokens.size(); ++a)
					tokens[a].pToken = (const char*)((size_t)tokens[a].pToken + difference);
			}
		}
	}

	// we're done, now compact it into a nice linear buffer
	int numTokens = tokens.size();
	void *pData = NULL;

	if(numTokens)
	{
		int stringCacheSize = MFStringCache_GetSize(pStringCache);
		int bytes = preBytes + sizeof(uiActionScript_Token) * numTokens + stringCacheSize;
		pData = MFHeap_Alloc(bytes);
		uiActionScript_Token *pTokens = (uiActionScript_Token*)((char*)pData + preBytes);

		// copy tokens
		MFCopyMemory(pTokens, tokens.getpointer(), sizeof(uiActionScript_Token) * numTokens);

		// copy strings
		const char *pCacheStrings = MFStringCache_GetCache(pStringCache);
		char *pStrings = (char *)(pTokens + numTokens);
		MFCopyMemory(pStrings, pCacheStrings, stringCacheSize);

		// update string pointers
		int64 stringRelocation = (int64)(size_t)pStrings - (int64)(size_t)pCacheStrings;
		for(int a=0; a<numTokens; ++a)
			pTokens[a].pToken = (const char*)((size_t)pTokens[a].pToken + stringRelocation);
	}

	if(pNumTokens)
		*pNumTokens = numTokens;

	MFStringCache_Destroy(pStringCache);

	return pData;
}

uiActionScript *uiActionManager::FindAction(const char *pName)
{
	return actions.Find(pName);
}

uiActionMetric *uiActionManager::FindMetric(const char *pName)
{
	return metrics.Find(pName);
}

uiActionScript *uiActionManager::CreateAction(const char *pName, const char *pDescription)
{
	uiActionScript *pScript = ParseScript(pDescription);
	actions.Add(pScript, pName);
	return pScript;
}

uiActionMetric *uiActionManager::CreateMetric(const char *pName, const char *pDescription)
{
	uiActionMetric *pMetric = ParseMetric(pDescription);
	metrics.Add(pMetric, pName);
	return pMetric;
}

void uiActionManager::DestroyAction(const char *pName)
{
	uiActionScript *pScript = actions.Remove(pName);
	MFHeap_Free(pScript);
}

void uiActionManager::DestroyMetric(const char *pName)
{
	uiActionMetric *pMetric = metrics.Remove(pName);
	MFHeap_Free(pMetric);
}

uiActionScript* uiActionManager::ParseScript(const char *pScript)
{
	// lex the script
	int numTokens;
	uiActionScript *pNew = (uiActionScript*)Lex(pScript, &numTokens, sizeof(uiActionScript));

	if(pNew)
	{
		// set token pointer
		pNew->pTokens = (uiActionScript_Token*)((char*)pNew + sizeof(uiActionScript));

		// parse the script for actions
		pNew->pAction = ParseActions(pNew->pTokens, numTokens);
	}

	return pNew;
}

uiActionMetric* uiActionManager::ParseMetric(const char *pMetric)
{
	// lex the script
	int numTokens;
	uiActionMetric *pNew = (uiActionMetric*)Lex(pMetric, &numTokens, sizeof(uiActionMetric));

	if(pNew)
	{
		// set token pointer
		pNew->pTokens = (uiActionScript_Token*)((char*)pNew + sizeof(uiActionMetric));
		pNew->numTokens = numTokens;
	}

	return pNew;
}

int uiActionScript_Action::ParseAction(uiActionScript_Token *pToken, int numTokens)
{
	// first we should expect an action identifier
	MFDebug_Assert(pToken[0].IsIdentifier() && pToken[1].IsSyntax("("), "Expected: Action.");

	int t = 0;
	pAction = pToken->pToken;

	// check if it specifies an entity
	if(char *pDot = MFString_Chr(pAction, '.'))
	{
		pEntity = pAction;
		pAction = pDot + 1;
		*pDot = 0;
	}
	t += 2;

	// we have an action, now find argument list
	pArgs = pToken + t;
	int argStart = t, depth = 0;
	while(t < numTokens && (depth || !pToken[t].IsSyntax(")")))
	{
		if(pToken[t].IsSyntax("("))
			++depth;
		if(pToken[t].IsSyntax(")"))
			--depth;
		++t;
	}

	numArgs = t - argStart;
	if(!numArgs)
		pArgs = NULL;

	bWaitComplete = true;

	return t + 1;
}

uiActionScript_Action *uiActionManager::ParseActions(uiActionScript_Token *pTokens, int numTokens)
{
	uiActionScript_Action *pFirstAction = NULL;
	uiActionScript_Action **ppNextAction = &pFirstAction;

	// there must be at least an action, opening, and closing parenthesis
	int t = 0;
	while(t < numTokens)
	{
		// allocate a new action
		uiActionScript_Action *pAction = (uiActionScript_Action*)gActionPool.AllocAndZero();
		t += pAction->ParseAction(&pTokens[t], numTokens - t);

		if(t < numTokens)
		{
			MFDebug_Assert(pTokens[t].IsSyntax(";") || pTokens[t].IsSyntax(":"), "Expected: Action separator. (';' or ':')");

			if(pTokens[t].IsSyntax(":"))
				pAction->bWaitComplete = false;
			++t;
		}

		// hook up the new action
		*ppNextAction = pAction;
		ppNextAction = &pAction->pNext;
	}

	return pFirstAction;
}

uiRuntimeArgs *uiActionManager::GetNextValue(uiActionScript_Token *&pT, int &remaining, MFVector &containerSize, int arrayIndex)
{
	uiRuntimeArgs *pValue = NULL;
	bool bNeg = false;
	bool bString = false;

	// check for unary sign operators
	if(pT->IsOperator("+"))
		++pT, --remaining;
	if(pT->IsOperator("-"))
		bNeg = true, ++pT, --remaining;

	MFDebug_Assert(remaining > 0 && (pT->IsValue() || pT->IsSyntax("{")), "Expected: Value");

	if(pT->IsSyntax("{"))
	{
		int numTokens = 0;
		pValue = uiRuntimeArgs::Allocate(1);
		pValue->SetArray(0, ResolveArguments(++pT, --remaining, containerSize, &numTokens));
		pT += numTokens;
		remaining -= numTokens;
	}
	else
	{
		if(pT->IsIdentifier())
		{
			const char *pIdentifier = pT->pToken;

			const char *pDot = MFString_Chr(pIdentifier, '.');
			if(pDot && !MFString_CaseCmpN(pT->pToken, "metric", (uint32)(pDot - pT->pToken)))
				pIdentifier = pDot + 1;

			// see if the identifier is a metric
			uiActionMetric *pMetric = GameData::Get()->GetActionManager()->FindMetric(pIdentifier);
			if(pMetric)
			{
				pValue = ResolveArguments(pMetric->pTokens, pMetric->numTokens, containerSize);
				MFDebug_Assert(pValue->GetNumArgs() == 1, "Metrics may only specify a single value.");
			}
			else
			{
				// resolve the identifier
				pValue = ResolveIdentifier(pIdentifier);
				bString = !pValue->IsNumeric(0);
			}
		}
		else
		{
			// simple value
			bString = pT->type == uiActionScript_Token::TT_String;
			MFString val(pT->pToken);
			pValue = uiRuntimeArgs::Allocate(1);
			pValue->SetValue(0, val, bString);
		}

		++pT;
		--remaining;

		// here we can support indexing
		while(remaining && pT->IsSyntax("["))
		{
			int numTokens;
			uiRuntimeArgs *pIndex = ResolveArguments(++pT, --remaining, containerSize, &numTokens);
			pT += numTokens;
			remaining -= numTokens;

			uiRuntimeArgs *pArray = pValue->GetArray(0);
			MFDebug_Assert(pArray && pIndex->IsNumeric() && pIndex->GetNumArgs() == 1, "Invalid array index.");

			int index = pIndex->GetInt(0);
			MFDebug_Assert(index >= 0 && index < pArray->GetNumArgs(), "Index out of bounds");

			uiRuntimeArgs *pItem = uiRuntimeArgs::Allocate(1);
			pItem->Set(0, pArray->Get(index));
			pValue->Release();
			pValue = pItem;
		}

		// syntax for screen percentages
		if(remaining && pT->IsOperator("%"))
		{
			MFDebug_Assert(!bString, "Expected: Numeric value");
			MFDebug_Assert(arrayIndex < 4, "Only 4d vectors supported");

			pValue->SetPercentage(0, containerSize[arrayIndex]);
			++pT;
			--remaining;
		}
	}

	if(bNeg)
	{
		MFDebug_Assert(!bString, "Error: Can't negate a string value");

		pValue->NegateValue(0);
	}

	return pValue;
}

inline MFString DoProduct(float a, float b, bool bMul)
{
	MFString result;
	result.FromFloat(bMul ? a*b : a/b);
	return result;
}

inline MFString DoSum(MFString a, MFString b, bool bAdd, bool bConcatinate)
{
	MFString result;
	if(bConcatinate)
	{
		MFDebug_Assert(bAdd, "Expected: Numeric operands.");
		result = a + b;
	}
	else
	{
		float af = a.ToFloat();
		float bf = b.ToFloat();
		result.FromFloat(bAdd ? af+bf : af-bf);
	}
	return result;
}

uiRuntimeArgs *uiActionManager::CalculateProducts(uiActionScript_Token *&pT, int &remaining, MFVector &containerSize, int arrayIndex)
{
	uiRuntimeArgs *pValue = GetNextValue(pT, remaining, containerSize, arrayIndex);

	while(remaining)
	{
		bool bMul = pT->IsOperator("*");
		if(!bMul && !pT->IsOperator("/"))
			break;

		// we should expect another operand
		MFDebug_Assert(remaining > 1, "Missing: operand");

		uiRuntimeArgs *pOperand = GetNextValue(++pT, --remaining, containerSize, arrayIndex);

		// perform multiplication
		MFDebug_Assert(pValue->IsNumeric() && pOperand->IsNumeric(), "Expected: Numeric value");

		uiRuntimeArgs *pArray0 = pValue->GetArray(0);
		if(pArray0)
		{
			uiRuntimeArgs *pArray1 = pOperand->GetArray(0);
			if(pArray1)
			{
				int numArgs = pArray0->GetNumArgs();
				MFDebug_Assert(numArgs == pOperand->GetArray(0)->GetNumArgs(), "Can not multiply vectors with mismatching number of args!");

				for(int a=0; a<numArgs; ++a)
					pArray0->SetValue(a, DoProduct(pArray0->GetFloat(a), pArray1->GetFloat(a), bMul), false);
				pOperand->Release();
			}
			else
			{
				int numArgs = pArray0->GetNumArgs();
				for(int a=0; a<numArgs; ++a)
					pArray0->SetValue(a, DoProduct(pArray0->GetFloat(a), pOperand->GetFloat(0), bMul), false);
				pOperand->Release();
			}
		}
		else
		{
			uiRuntimeArgs *pArray1 = pOperand->GetArray(0);
			if(pArray1)
			{
				int numArgs = pArray1->GetNumArgs();
				for(int a=0; a<numArgs; ++a)
					pArray1->SetValue(a, DoProduct(pValue->GetFloat(0), pArray1->GetFloat(a), bMul), false);
				pValue->Release();
				pValue = pOperand;
			}
			else
			{
				pValue->SetValue(0, DoProduct(pValue->GetFloat(0), pOperand->GetFloat(0), bMul), false);
				pOperand->Release();
			}
		}
	}

	return pValue;
}

uiRuntimeArgs *uiActionManager::ResolveArguments(uiActionScript_Token *pTokens, int numTokens, MFVector &containerSize, int *pNumUsed)
{
	uiActionScript_Token *pT = pTokens;
	uiRuntimeArgs *pNew = NULL;

	// parse the arguments
	bool bEndStatement = false;
	int index = 0;
	while(numTokens && !bEndStatement)
	{
		uiRuntimeArgs *pValue = CalculateProducts(pT, numTokens, containerSize, index);
		bool bIsString = !pValue->IsNumeric();

		while(numTokens)
		{
			bool bAdd = pT->IsOperator("+");
			if(!bAdd && !pT->IsOperator("-"))
				break;

			MFDebug_Assert(numTokens > 1, "Missing: Operand");

			uiRuntimeArgs *pOperand = CalculateProducts(++pT, --numTokens, containerSize, index);
			bool bConcatinate = bIsString || !pOperand->IsNumeric();

			uiRuntimeArgs *pArray0 = pValue->GetArray(0);
			if(pArray0)
			{
				int numArgs = pArray0->GetNumArgs();
				uiRuntimeArgs *pArray1 = pOperand->GetArray(0);
				MFDebug_Assert(pArray1 && pArray1->GetNumArgs() == numArgs, "Operand has different number of dimensions.");

				for(int a=0; a<numArgs; ++a)
					pArray0->SetValue(a, DoSum(pArray0->GetString(a), pArray1->GetString(a), bAdd, bConcatinate), false);
				pOperand->Release();
			}
			else
			{
				MFDebug_Assert(!pOperand->GetArray(0), "Operand has different number of dimensions.");

				pValue->SetValue(0, DoSum(pValue->GetString(0), pOperand->GetString(0), bAdd, bConcatinate), false);
				pOperand->Release();
			}
		}

		if(!pNew)
			pNew = pValue;
		else
			pNew->Extend(pValue);
		++index;

		if(numTokens)
		{
			if(pT->IsSyntax("}") || pT->IsSyntax("]") || pT->IsSyntax(")"))
				bEndStatement = true;
			else
			{
				MFDebug_Assert(pT->IsSyntax(","), "Expected: Argument separator");
			}

			++pT;
			--numTokens;
		}
	}

	if(pNumUsed)
		*pNumUsed = (int)(pT - pTokens);

	return pNew;
}

uiRuntimeArgs *uiActionManager::ResolveIdentifier(const char *pIdentifier)
{
	uiRuntimeArgs *pValue = uiRuntimeArgs::Allocate(1);
	MFString value(pIdentifier);
	bool bString = false;

	char *pDot = MFString_Chr(pIdentifier, '.');
	if(pDot)
	{
		const char *pSource = MFStrN(pIdentifier, pDot - pIdentifier);
		++pDot;

		if(!MFString_CaseCmp(pSource, "strings"))
		{
			// find and return string in string table
			int stringID = GameData::Get()->FindString(pDot);
			if(stringID >= 0)
			{
				value = GameData::Get()->GetString(stringID);
				bString = true;
			}
		}
		else if(!MFString_CaseCmp(pSource, "options"))
		{
			// look up value from the game options
			//...
		}
		else
		{
			// it could be an entity identifier
			uiEntity *pEntity = GameData::Get()->GetEntityManager()->Find(pSource);
			if(pEntity)
			{
				// which means we must be accessing an entity property
				value = GetEntityProperty(pEntity, pDot);
			}
			else
			{
				// nope, don't know what it is...
				//... error?
			}
		}
	}
	else
	{
		// it could be a property for 'this' entity...
		// TODO: we don't have a pointer to the current entity
		//...
	}

	pValue->SetValue(0, value, bString);

	return pValue;
}

void uiRuntimeArgs::Init()
{
	runtimeArgPool.Init(sizeof(uiRuntimeArgs), 64, 16);

	pArgPool = (Argument*)MFHeap_AllocAndZero(sizeof(Argument) * poolSize);
	pCurrent = pArgPool;
}

void uiRuntimeArgs::Deinit()
{
	MFHeap_Free(pArgPool);

	runtimeArgPool.Deinit();
}

uiRuntimeArgs::Argument *uiRuntimeArgs::FindStart(Argument *pStart)
{
	while(pCurrent->type)
	{
		++pCurrent;
		if(pCurrent - pArgPool >= poolSize)
			pCurrent = pArgPool;

		if(pCurrent == pStart)
		{
			MFDebug_Assert(false, "Not enough args!");
			return NULL;
		}
	}

	return pCurrent++;
}

uiRuntimeArgs::Argument *uiRuntimeArgs::AllocArgs(int numArgs)
{
	Argument *pStart = pCurrent;
	Argument *pFirst = FindStart(pStart);

	while(pCurrent - pFirst < numArgs)
	{
		if(pCurrent->type)
		{
			pFirst = FindStart(pStart);
			if(!pFirst)
				return NULL;
		}
		else
		{
			++pCurrent;
			if(pCurrent - pArgPool >= poolSize)
				pCurrent = pArgPool;

			if(pCurrent == pStart)
			{
				MFDebug_Assert(false, "Not enough args!");
				return NULL;
			}
		}
	}

	for(int a=0; a<numArgs; ++a)
		pFirst[a].type = AT_Allocated;

	return pFirst;
}

uiRuntimeArgs *uiRuntimeArgs::Allocate(int numArgs)
{
	uiRuntimeArgs *pArgs = (uiRuntimeArgs*)runtimeArgPool.Alloc();
	pArgs->numArgs = numArgs;
	pArgs->pArgs = AllocArgs(numArgs);
	return pArgs;
}

int uiRuntimeArgs::Extend(uiRuntimeArgs *pValue)
{
	if(pValue->pArgs == pArgs + numArgs)
	{
		pValue->Release(1);
		return ++numArgs;
	}

	if(pArgs[numArgs].type != AT_Unallocated)
	{
		Argument *pNew = AllocArgs(numArgs + 1);
		for(int a=0; a<numArgs; ++a)
		{
			pNew[a].type = pArgs[a].type;
			pNew[a].iValue = pArgs[a].iValue;
			pNew[a].string = pArgs[a].string;
			pArgs[a].type = AT_Unallocated;
			pArgs[a].string = NULL; // release the reference on the string
		}
		pArgs = pNew;
	}

	// append the new arg
	pArgs[numArgs].type = pValue->pArgs[0].type;
	pArgs[numArgs].iValue = pValue->pArgs[0].iValue;
	pArgs[numArgs].string = pValue->pArgs[0].string;
	pValue->Release();

	return ++numArgs;
}

void uiRuntimeArgs::Release(int startArg)
{
	for(int a=startArg; a<numArgs; ++a)
	{
		if(pArgs[a].type == AT_Array)
			pArgs[a].pArray->Release();
		else
			pArgs[a].string = NULL;
		pArgs[a].type = AT_Unallocated;
	}

	runtimeArgPool.Free(this);
}