#include "Warlords.h"
#include "MFStringCache.h"

#include "Action.h"
#include "Entity.h"

Factory<uiAction> uiActionManager::actionFactory;
MFObjectPool uiActionManager::actionTypePool;
MFObjectPool uiActionManager::actionPool;
HKOpenHashTable<uiActionManager::ActionType*> uiActionManager::actionRegistry;

const char * const gpDelimeters = " \t;:',()[]{}!+-*/%\\|=";

MFObjectPool uiRuntimeArgs::runtimeArgPool;
uiRuntimeArgs::Argument *uiRuntimeArgs::pArgPool = NULL;
uiRuntimeArgs::Argument *uiRuntimeArgs::pCurrent = NULL;
int uiRuntimeArgs::poolSize = 1024;

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
	return (const char *)NULL;
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

	if(pArg->type == AT_Array)
	{
		int numItems = pArg->pArray->GetNumArgs();
		pArgs[index].pArray = uiRuntimeArgs::Allocate(numItems);
		for(int a=0; a<numItems; ++a)
			pArgs[index].pArray->Set(a, pArg->pArray->Get(a));
	}
	else
	{
		pArgs[index].iValue = pArg->iValue;
		pArgs[index].string = pArg->string;
	}
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

void uiRuntimeArgs::NotValue(int index)
{
	if(index == -1)
	{
		for(int a=0; a<numArgs; ++a)
			NotValue(a);
	}
	else
	{
		if(pArgs[index].type == AT_Int)
			pArgs[index].iValue = !pArgs[index].iValue;
		else if(pArgs[index].type == AT_Float)
		{
			pArgs[index].iValue = pArgs[index].fValue == 0 ? 1 : 0;
			pArgs[index].type = AT_Int;
		}
		else if(pArgs[index].type == AT_String)
		{
			if(pArgs[index].string.EqualsInsensitive("false"))
			{
				pArgs[index].string = "true";
			}
			else if(pArgs[index].string.EqualsInsensitive("true"))
			{
				pArgs[index].string = "false";
			}
			else
			{
				pArgs[index].iValue = pArgs[index].string.IsEmpty() ? 1 : 0;
				pArgs[index].string = NULL;
				pArgs[index].type = AT_Int;
			}
		}
		else if(pArgs[index].type == AT_Array)
			pArgs[index].pArray->NotValue();
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
	actionPool.Init(sizeof(uiActionScript_Action), 128, 128);
	actionRegistry.Init(64, 256, 32);
}

void uiActionManager::DeinitManager()
{
	actionRegistry.Deinit();
	actionPool.Deinit();
	actionTypePool.Deinit();
}

void uiActionManager::Init()
{
	actions.Init(256, 512, 512);
	metrics.Init(256, 512, 512);

	activeDeferredActions.Init("Deferred Action Pool", 1024);
	runningActions.Init("Active Actions", 256);
}

void uiActionManager::Deinit()
{
	activeDeferredActions.Deinit();
	runningActions.Deinit();

	metrics.Deinit();
	actions.Deinit();
}

void uiActionManager::Update()
{
	uiExecuteContext **ppI = runningActions.Begin();
	for(; *ppI; ++ppI)
	{
		uiExecuteContext *pContext = *ppI;

		for(uiActiveAction *pActive = pContext->activeList.head(); pActive; )
		{
			uiActiveAction *pNext = pContext->activeList.next(pActive);

			if(pActive->pAction->Update())
			{
				delete pActive->pAction;

				uiActionScript_Action *pNext = pActive->pNextAction;
				uiEntity *pEntity = pActive->pEntity;

				pContext->activeList.remove(pActive);
				activeDeferredActions.Destroy(pActive);

				if(pNext)
					Continue(pContext, pNext, pEntity);
			}

			pActive = pNext;
		}

		if(pContext->activeList.head() == NULL)
		{
			if(pContext->pArgs)
				pContext->pArgs->Release();
			runningActions.Destroy(ppI);
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

	actionRegistry.Add(pType->pName, pType);
}

void uiActionManager::RegisterInstantAction(const char *pActionName, InstantActionHandler *pActionHandler, FactoryType *pEntityType)
{
	ActionType *pType = (ActionType*)actionTypePool.Alloc();
	pType->pName = pActionName;
	pType->pGetHandler = NULL;
	pType->pSetHandler = pActionHandler;
	pType->pEntityType = pEntityType;

	actionRegistry.Add(pType->pName, pType);
}

void uiActionManager::RegisterDeferredAction(const char *pActionName, Factory_CreateFunc *pCreateFunc, FactoryType *pEntityType)
{
	ActionType *pType = (ActionType*)actionTypePool.Alloc();
	pType->pName = pActionName;
	pType->pGetHandler = NULL;
	pType->pSetHandler = NULL;
	pType->pEntityType = pEntityType;

	actionFactory.RegisterType(pType->pName, pCreateFunc);

	actionRegistry.Add(pType->pName, pType);
}

uiExecuteContext *uiActionManager::RunScript(uiActionScript *pScript, uiEntity *pEntity, uiRuntimeArgs *pArgs)
{
	uiExecuteContext *pContext = new(runningActions.Create()) uiExecuteContext;
	pContext->pScript = pScript;
	pContext->pEntity = pEntity;
	pContext->pArgs = pArgs;

	MFDebug_Assert(pContext->activeList.head() == NULL, "!");

	if(Continue(pContext, pScript->pAction, pEntity))
	{
		if(pArgs)
			pArgs->Release();
		runningActions.Destroy(pContext);
		pContext = NULL;
	}

	return pContext;
}

bool uiActionManager::RunAction(uiExecuteContext *pContext, const char *pAction, uiRuntimeArgs *pArgs, uiEntity *pActionEntity, uiActionScript_Action *pNextAction, uiEntity *pNextEntity)
{
	bool bFinished = true;

	uiActionScript **ppScript = actions.Get(pAction);
	if(ppScript)
	{
		RunScript(*ppScript, pActionEntity, pArgs);
		return true;
	}

	if(!pActionEntity || !pActionEntity->ExecuteAction(pAction, pArgs))
	{
		ActionType *pType = FindActionType(pAction, pActionEntity);
		if(!pType)
			return true;

		MFDebug_Assert(!pType->pEntityType || pActionEntity->IsType(pType->pEntityType), MFStr("Can not perform action '%s' on this type of entity!", pAction));

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
					uiActiveAction *pNew = activeDeferredActions.Create();
					pNew->pAction = pDeferred;
					pNew->pEntity = pNextEntity;
					pNew->pNextAction = pNextAction;

					pContext->activeList.insert(pNew);
				}
				else
				{
					delete pDeferred;
				}
			}
		}
	}

	// release the args
	if(pArgs)
		pArgs->Release();

	return bFinished;
}

MFString uiActionManager::GetEntityProperty(uiEntity *pEntity, const char *pProperty)
{
	ActionType *pType = FindActionType(pProperty, pEntity);
	if(pType && pType->pGetHandler)
		return pType->pGetHandler(pEntity);
	return (const char *)NULL;
}

void uiActionManager::SetEntityProperty(uiEntity *pEntity, const char *pProperty, const char *pArgs)
{
	ActionType *pType = FindActionType(pProperty, pEntity);
	if(pType && pType->pSetHandler)
	{
		uiRuntimeArgs *pRuntimeArgs = ParseArgs(pArgs, pEntity);
		pType->pSetHandler(pEntity, pRuntimeArgs);
		pRuntimeArgs->Release();
	}
}

void uiActionManager::DestroyEntity(uiEntity *pEntity)
{
	uiExecuteContext **ppI = runningActions.Begin();
	for(; *ppI; ++ppI)
	{
		uiExecuteContext *pContext = *ppI;

		if(pContext->pEntity == pEntity)
		{
			for(uiActiveAction *pActive = pContext->activeList.head(); pActive; )
			{
				uiActiveAction *pNext = pContext->activeList.next(pActive);

				delete pActive->pAction;

				pContext->activeList.remove(pActive);
				activeDeferredActions.Destroy(pActive);

				pActive = pNext;
			}

			if(pContext->pArgs)
				pContext->pArgs->Release();
			runningActions.Destroy(ppI);
		}
	}
}

uiRuntimeArgs *uiActionManager::ParseArgs(const char *pArgs, uiEntity *pEntity)
{
	if(pArgs[0] == 0)
	{
		uiRuntimeArgs *pEmpty = uiRuntimeArgs::Allocate(1);
		pEmpty->SetValue(0, "", true);
		return pEmpty;
	}

	// create a dummy context
	uiExecuteContext context;
	MFZeroMemory(&context, sizeof(context));
	context.pEntity = pEntity;

	// resolve the args
	int numTokens;
	uiActionScript_Token *pTokens = (uiActionScript_Token*)Lex(pArgs, &numTokens);
	MFVector containerSize = pEntity ? pEntity->GetContainerSize() : MFVector::zero;
	uiRuntimeArgs *pRuntimeArgs = ResolveArguments(&context, pTokens, numTokens, containerSize);
	MFHeap_Free(pTokens);

	return pRuntimeArgs;
}

uiActionManager::ActionType *uiActionManager::FindActionType(const char *pName, uiEntity *pEntity)
{
	FactoryType *pType = pEntity ? pEntity->GetType() : NULL;

	ActionType **ppActionType = actionRegistry.Get(pName);
	if(!ppActionType)
		return NULL;

	while(1)
	{
		ActionType **ppI = ppActionType;
		do
		{
			if((*ppI)->pEntityType == pType)
				return *ppI;
		}
		while(ppI = actionRegistry.NextMatch(ppI));

		if(pType)
			pType = pType->pParent;
		else break;
	}

	return NULL;
}

void uiActionManager::SetEntityProperty(uiEntity *pEntity, const char *pProperty, uiRuntimeArgs *pArguments)
{
	ActionType *pType = FindActionType(pProperty, pEntity);
	if(pType && pType->pSetHandler)
		pType->pSetHandler(pEntity, pArguments);
}

bool uiActionManager::Continue(uiExecuteContext *pContext, uiActionScript_Action *pNext, uiEntity *pEntity)
{
	// check if action specifies an entity
	uiEntity *pActionEntity = pNext->pEntity ? pEntity->GetEntityManager()->Find(pNext->pEntity) : pEntity;

	// resolve action parameters
	uiRuntimeArgs *pArgs = NULL;
	if(pNext->pArgs)
	{
		MFVector containerSize = pActionEntity ? pActionEntity->GetContainerSize() : MFVector::zero;
		pArgs = ResolveArguments(pContext, pNext->pArgs, pNext->numArgs, containerSize);
	}

	bool bFinished = RunAction(pContext, pNext->pAction, pArgs, pActionEntity, pNext->bWaitComplete ? pNext->pNext : NULL, pEntity);

	if(pNext->pNext && (bFinished || !pNext->bWaitComplete))
		bFinished = Continue(pContext, pNext->pNext, pEntity) && bFinished;

	return bFinished;
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
		else if(*pAction == '"')
		{
			// scan for the closing quote character
			const char *pEndQuote = MFString_Chr(++pAction, '"');
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
			if(*pAction == '+' || *pAction == '-' || *pAction == '*' || *pAction == '/' || *pAction == '%' || *pAction == '!')
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
				size_t bytes = MFStringCache_GetSize(pStringCache);
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
		size_t stringCacheSize = MFStringCache_GetSize(pStringCache);
		size_t bytes = preBytes + sizeof(uiActionScript_Token) * numTokens + stringCacheSize;
		pData = MFHeap_Alloc(bytes);
		MFZeroMemory(pData, preBytes);

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
	uiActionScript **ppT = actions.Get(pName);
	return ppT ? *ppT : NULL;
}

uiActionMetric *uiActionManager::FindMetric(const char *pName)
{
	uiActionMetric **ppT = metrics.Get(pName);
	return ppT ? *ppT : NULL;
}

uiActionScript *uiActionManager::CreateAction(const char *pName, const char *pDescription)
{
	uiActionScript *pScript = ParseScript(pName, pDescription);
	actions.Add(pScript->name, pScript);
	return pScript;
}

uiActionMetric *uiActionManager::CreateMetric(const char *pName, const char *pDescription)
{
	uiActionMetric *pMetric = ParseMetric(pDescription);
	metrics.Add(pName, pMetric);
	return pMetric;
}

void uiActionManager::DestroyAction(const char *pName)
{
	uiActionScript *pScript = actions[pName];
	actions.Destroy(pName);
	MFHeap_Free(pScript);
}

void uiActionManager::DestroyMetric(const char *pName)
{
	uiActionMetric *pMetric = metrics[pName];
	metrics.Destroy(pName);
	MFHeap_Free(pMetric);
}

uiActionScript* uiActionManager::ParseScript(const char *pName, const char *pScript)
{
	// count args
	int numArgs = 0;

	const char *pArgs = MFString_Chr(pName, '(');
	if(pArgs && pArgs[1] != ')')
	{
		numArgs = 1;

		while((pArgs = MFString_Chr(pArgs + 1, ',')) != NULL)
			++numArgs;
	}

	// lex the script
	int numTokens;
	uiActionScript *pNew = (uiActionScript*)Lex(pScript, &numTokens, sizeof(uiActionScript) + sizeof(MFString)*numArgs);

	if(pNew)
	{
		// parse the args
		char *pOffset = (char*)pNew + sizeof(uiActionScript);
		pNew->pArgs = (MFString*)(numArgs ? pOffset : NULL);

		pNew->name = pName;
		int offset = pNew->name.FindChar('(');
		if(offset > 0)
		{
			MFString arg = pNew->name.SubStr(offset + 1);
			pNew->name.Truncate(offset);

			offset = arg.FindChar(')');
			if(offset > -1)
				arg.Truncate(offset);

			if(arg.NumBytes())
			{
				while((offset = arg.FindChar(',')) > -1)
				{
					pNew->pArgs[pNew->numArgs++] = arg.SubStr(0, offset).Trim();
					arg = arg.SubStr(offset + 1);
				}

				pNew->pArgs[pNew->numArgs++] = arg.Trim();
			}
		}

		// set token pointer
		pNew->pTokens = (uiActionScript_Token*)(pOffset + sizeof(MFString)*numArgs);

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
	MFDebug_Assert(pToken[0].IsIdentifier(), "Expected: Action.");

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
	int argStart = t;

	if(pToken[t-1].IsSyntax("="))
	{
		while(t < numTokens && !pToken[t].IsSyntax(";") && !pToken[t].IsSyntax(":"))
			++t;
	}
	else
	{
		MFDebug_Assert(pToken[t-1].IsSyntax("("), "Expected: '(' or '='.");

		int depth = 0;
		while(t < numTokens && (depth || !pToken[t].IsSyntax(")")))
		{
			if(pToken[t].IsSyntax("("))
				++depth;
			if(pToken[t].IsSyntax(")"))
				--depth;
			++t;
		}
	}

	numArgs = t - argStart;
	if(!numArgs)
		pArgs = NULL;

	if(pToken[t].IsSyntax(")"))
		++t;

	bWaitComplete = true;

	return t;
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
		uiActionScript_Action *pAction = (uiActionScript_Action*)actionPool.AllocAndZero();
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

uiRuntimeArgs *uiActionManager::GetNextValue(uiExecuteContext *pContext, uiActionScript_Token *&pT, int &remaining, const MFVector &containerSize, int arrayIndex)
{
	uiRuntimeArgs *pValue = NULL;
	bool bNeg = false;
	bool bNot = false;
	bool bString = false;

	// check for unary sign operators
	if(pT->IsOperator("!"))
		bNot = true, ++pT, --remaining;
	if(pT->IsOperator("+"))
		++pT, --remaining;
	if(pT->IsOperator("-"))
		bNeg = true, ++pT, --remaining;

	MFDebug_Assert(remaining > 0 && (pT->IsValue() || pT->IsSyntax("{")), "Expected: Value");

	if(pT->IsSyntax("{"))
	{
		int numTokens = 0;
		pValue = uiRuntimeArgs::Allocate(1);
		pValue->SetArray(0, ResolveArguments(pContext, ++pT, --remaining, containerSize, &numTokens));
		pT += numTokens;
		remaining -= numTokens;
	}
	else
	{
		if(pT->IsIdentifier())
		{
			const char *pIdentifier = pT->pToken;

			// resolve the identifier
			pValue = ResolveIdentifier(pContext, pIdentifier, containerSize);
			bString = !pValue->IsNumeric(0);
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
			uiRuntimeArgs *pIndex = ResolveArguments(pContext, ++pT, --remaining, containerSize, &numTokens);
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

	if(bNot)
	{
		pValue->NotValue(0);
	}

	return pValue;
}

inline MFString DoProduct(float a, float b, bool bMul)
{
	MFString result;
	result.FromFloat(bMul ? a*b : a/b);
	return result;
}

inline MFString DoSum(MFString a, MFString b, int oper, bool bConcatinate)
{
	MFString result;
	if(bConcatinate)
	{
		switch(oper)
		{
			case 0:
				result = a + b;
				break;
			case 6:
				result = a.EqualsInsensitive(b) ? "true" : "false";
				break;
			case 7:
				result = a.EqualsInsensitive(b) ? "false" : "true";
				break;
			default:
				MFDebug_Assert(false, "Expected: Numeric operands.");
				break;
		}
	}
	else
	{
		float af = a.ToFloat();
		float bf = b.ToFloat();
		switch(oper)
		{
			case 0:
				result.FromFloat(af+bf);
				break;
			case 1:
				result.FromFloat(af-bf);
				break;
			case 2:
				result = af < bf ? "true" : "false";
				break;
			case 3:
				result = af > bf ? "true" : "false";
				break;
			case 4:
				result = af <= bf ? "true" : "false";
				break;
			case 5:
				result = af >= bf ? "true" : "false";
				break;
			case 6:
				result = af == bf ? "true" : "false";
				break;
			case 7:
				result = af != bf ? "true" : "false";
				break;
			default:
				break;
		}
	}
	return result;
}

uiRuntimeArgs *uiActionManager::CalculateProducts(uiExecuteContext *pContext, uiActionScript_Token *&pT, int &remaining, const MFVector &containerSize, int arrayIndex)
{
	uiRuntimeArgs *pValue = GetNextValue(pContext, pT, remaining, containerSize, arrayIndex);

	while(remaining)
	{
		bool bMul = pT->IsOperator("*");
		if(!bMul && !pT->IsOperator("/"))
			break;

		// we should expect another operand
		MFDebug_Assert(remaining > 1, "Missing: operand");

		uiRuntimeArgs *pOperand = GetNextValue(pContext, ++pT, --remaining, containerSize, arrayIndex);

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

uiRuntimeArgs *uiActionManager::ResolveArguments(uiExecuteContext *pContext, uiActionScript_Token *pTokens, int numTokens, const MFVector &containerSize, int *pNumUsed)
{
	uiActionScript_Token *pT = pTokens;
	uiRuntimeArgs *pNew = NULL;

	// parse the arguments
	bool bEndStatement = false;
	int index = 0;
	while(numTokens && !bEndStatement)
	{
		uiRuntimeArgs *pValue = CalculateProducts(pContext, pT, numTokens, containerSize, index);
		bool bIsString = !pValue->IsNumeric();

		while(numTokens)
		{
			int oper = -1;
			if(pT->IsOperator("+"))
				oper = 0;
			else if(pT->IsOperator("-"))
				oper = 1;
			else if(pT->IsOperator("<"))
				oper = 2;
			else if(pT->IsOperator(">"))
				oper = 3;
			else if(pT->IsOperator("<="))
				oper = 4;
			else if(pT->IsOperator(">="))
				oper = 5;
			else if(pT->IsOperator("=="))
				oper = 6;
			else if(pT->IsOperator("!="))
				oper = 7;
			if(oper == -1)
				break;

			MFDebug_Assert(numTokens > 1, "Missing: Operand");

			uiRuntimeArgs *pOperand = CalculateProducts(pContext, ++pT, --numTokens, containerSize, index);
			bIsString = bIsString || !pOperand->IsNumeric();

			uiRuntimeArgs *pArray0 = pValue->GetArray(0);
			if(pArray0)
			{
				int numArgs = pArray0->GetNumArgs();
				uiRuntimeArgs *pArray1 = pOperand->GetArray(0);
				MFDebug_Assert(pArray1 && pArray1->GetNumArgs() == numArgs, "Operand has different number of dimensions.");

				for(int a=0; a<numArgs; ++a)
					pArray0->SetValue(a, DoSum(pArray0->GetString(a), pArray1->GetString(a), oper, bIsString), bIsString);
				pOperand->Release();
			}
			else
			{
				MFDebug_Assert(!pOperand->GetArray(0), "Operand has different number of dimensions.");

				pValue->SetValue(0, DoSum(pValue->GetString(0), pOperand->GetString(0), oper, bIsString), bIsString);
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

uiRuntimeArgs *uiActionManager::ResolveIdentifier(uiExecuteContext *pContext, const char *_pIdentifier, const MFVector &containerSize)
{
	enum SearchType
	{
		None = 0,
		Entity,
		Metric,
		Option,
		String
	};

	const char *pIdentifier = _pIdentifier;
	uiEntity *pEntity = pContext ? pContext->pEntity : NULL;
	bool bMemberFollows;
	int type = None;
	int depth = 0;

	do
	{
		MFString identifier(pIdentifier, true);
		bMemberFollows = false;

		// check for member syntax
		const char *pDot = MFString_Chr(pIdentifier, '.');
		if(pDot)
		{
			identifier.Truncate((int)(pDot - pIdentifier));
			identifier.Trim();

			pIdentifier = MFSkipWhite(pDot + 1);

			bMemberFollows = true;
		}

		if(!bMemberFollows)
		{
			// check if it is an action argument
			if(depth == 0 && pContext && pContext->pScript)
			{
				int a=0, numArgs = pContext->pScript->numArgs;
				for(; a<numArgs; ++a)
				{
					if(pContext->pScript->pArgs[a].EqualsInsensitive(identifier))
					{
						uiRuntimeArgs *pValue = uiRuntimeArgs::Allocate(1);
						pValue->Set(0, pContext->pArgs->Get(a));
						return pValue;
					}
				}
			}

			// check if it is an entity property
			if(type == None && pEntity)
			{
				MFString value = GetEntityProperty(pEntity, identifier.CStr());
				if(value)
					return ParseArgs(value.CStr(), pEntity);
			}

			// check if it is a metric
			if(depth == 0 || type == Metric)
			{
				uiActionMetric *pMetric = GameData::Get()->GetActionManager()->FindMetric(identifier.CStr());
				if(pMetric)
				{
					uiRuntimeArgs *pValue = ResolveArguments(pContext, pMetric->pTokens, pMetric->numTokens, containerSize);
					MFDebug_Assert(pValue->GetNumArgs() == 1, "Metrics may only specify a single value.");
					return pValue;
				}
			}
		}

		// check for other stuff
		if(type != None)
		{
			if(type == Entity)
			{
				uiEntity *pE = GameData::Get()->GetEntityManager()->Find(identifier.CStr());
				if(!pE)
					break;

				pEntity = pE;
				type = None;
			}
			else if(type == Option)
			{
				MFDebug_Assert(!bMemberFollows, "Illegal syntax: options may not have sub-members");

				// look up value from the game options
				//...

				break;
			}
			else if(type == String)
			{
				MFDebug_Assert(!bMemberFollows, "Illegal syntax: strings may not have sub-members");

				// find and return string in string table
				int stringID = GameData::Get()->FindString(pDot);
				if(stringID >= 0)
				{
					MFString string = GameData::Get()->GetString(stringID);

					uiRuntimeArgs *pValue = uiRuntimeArgs::Allocate(1);
					pValue->SetValue(0, string, true);
					return pValue;
				}
				break;
			}
		}
		else if(pEntity)
		{
			// see if it refers to a child entity
			uiEntity *pChild = pEntity->FindChild(identifier.CStr());
			if(pChild)
			{
				pEntity = pChild;
				continue;
			}

			if(identifier.EqualsInsensitive("parent"))
			{
				uiEntity *pParent = pEntity->Parent();
				if(!pParent)
					break;

				pEntity = pParent;
				continue;
			}
		}

		// check for some special keywords
		if(depth == 0)
		{
			if(identifier.EqualsInsensitive("entities"))
			{
				type = Entity;
			}
			else if(identifier.EqualsInsensitive("metrics"))
			{
				type = Metric;
			}
			else if(identifier.EqualsInsensitive("options"))
			{
				type = Option;
			}
			else if(identifier.EqualsInsensitive("strings"))
			{
				type = String;
			}
		}

		++depth;
	}
	while(bMemberFollows);

	MFString value(pIdentifier);

	uiRuntimeArgs *pValue = uiRuntimeArgs::Allocate(1);
	pValue->SetValue(0, _pIdentifier, true);
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
