#if !defined(_ACTION_H)
#define _ACTION_H

#include "MFList.h"
#include "MFIni.h"
#include "MFPtrList.h"
#include "MFObjectPool.h"
#include "../Tools/Factory.h"
#include "../Tools/HashTable.h"

class uiEntity;
struct uiActionScript_Token;
struct uiActionScript_Action;
class uiRuntimeArgs;

class uiRuntimeArgs
{
private:
	enum ArgType
	{
		AT_Allocated = -1,
		AT_Unallocated = 0,

		AT_String,
		AT_Int,
		AT_Float,
		AT_Array
	};

	struct Argument
	{
		ArgType type;
		union
		{
			int iValue;
			float fValue;
			uiRuntimeArgs *pArray;
		};
		MFString string; // this should go in the union... :/
	};

public:
	static void Init();
	static void Deinit();

	static uiRuntimeArgs *Allocate(int numArgs);
	int Extend(uiRuntimeArgs *pValue);
	void Release(int startArg = 0);

	inline int GetNumArgs() { return numArgs; }

	bool IsNumeric(int index = -1);

	Argument *Get(int index);
	MFString GetString(int index);
	bool GetBool(int index);
	int GetInt(int index);
	float GetFloat(int index);
	MFVector GetVector(int index);
	uiRuntimeArgs *GetArray(int index);

	void Set(int index, Argument *pArg);
	void SetValue(int index, MFString str, bool bString);
	void SetArray(int index, uiRuntimeArgs *pArray);
	void NegateValue(int index = -1);
	void SetPercentage(int index, float containerSize);

protected:
	Argument *pArgs;
	int numArgs;

	inline static Argument *FindStart(Argument *pStart);
	inline static Argument *AllocArgs(int numArgs);
	static MFObjectPool runtimeArgPool;
	static Argument *pArgPool;
	static Argument *pCurrent;
	static int poolSize;
};

class uiAction
{
	friend class uiActionManager;
public:
	virtual void Init(uiRuntimeArgs *pParameters) = 0;
	virtual bool Update() = 0;

protected:
	uiEntity *pEntity;
};


struct uiActionScript
{
	MFString name;
	MFString *pArgs;
	int numArgs;

	uiActionScript_Action *pAction;
	uiActionScript_Token *pTokens;
};

struct  uiActionMetric
{
	uiActionScript_Token *pTokens;
	int numTokens;
};

struct uiActionScript_Token
{
	enum TokenType
	{
		TT_Unknown = 0,

		TT_Identifier,
		TT_Number,
		TT_String,
		TT_Arithmetic,
		TT_Syntax
	};

	inline bool IsString(const char *pString) { return !MFString_CaseCmp(pToken, pString); }
	inline bool IsSyntax(const char *pString = NULL) { return type == TT_Syntax && (!pString || !MFString_CaseCmp(pToken, pString)); }
	inline bool IsOperator(const char *pString = NULL) { return type == TT_Arithmetic && (!pString || !MFString_CaseCmp(pToken, pString)); }
	inline bool IsIdentifier() { return type == TT_Identifier; }
	inline bool IsNumeric() { return type == TT_Number || (type == TT_String && MFString_IsNumber(pToken)); }
	inline bool IsValue() { return type == TT_Identifier || type == TT_Number || type == TT_String; }

	const char *pToken;
	TokenType type;
};

struct uiActionScript_Action
{
	int ParseAction(uiActionScript_Token *pToken, int numTokens);

	const char *pEntity;
	const char *pAction;
	uiActionScript_Token *pArgs;
	uiActionScript_Action *pNext;
	int numArgs;
	bool bWaitComplete;
};

struct uiExecuteContext;

struct uiActiveAction
{
	uiAction *pAction;
	uiEntity *pEntity;
	uiActionScript_Action *pNextAction;

	uiExecuteContext *pContext;
	uiActiveAction *pNext;
};

struct uiExecuteContext
{
	uiActionScript *pScript;
	uiEntity *pEntity;

	uiRuntimeArgs *pArgs;

	MFList<uiActiveAction> activeList;
};

class uiActionManager
{
public:
	typedef MFString (GetPropertyHandler)(uiEntity *pEntity);
	typedef void (InstantActionHandler)(uiEntity *pEntity, uiRuntimeArgs *pArguments);

protected:
	struct ActionType
	{
		MFString GetName() { return MFString::Static(pName); }

		const char *pName;
		InstantActionHandler *pSetHandler;
		GetPropertyHandler *pGetHandler;
		FactoryType *pEntityType;
	};

public:
	typedef MFString (GetPropertyHandler)(uiEntity *pEntity);
	typedef void (InstantActionHandler)(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void RegisterProperty(const char *pPropertyName, GetPropertyHandler *pGetHandler, InstantActionHandler *pSetHandler, FactoryType *pEntityType);
	static void RegisterInstantAction(const char *pActionName, InstantActionHandler *pActionHandler, FactoryType *pEntityType);
	static void RegisterDeferredAction(const char *pActionName, Factory_CreateFunc *pCreateFunc, FactoryType *pEntityType);

	static void InitManager();
	static void DeinitManager();

	// instance methods
	void Init();
	void Deinit();

	void Update();

	uiActionScript *CreateAction(const char *pName, const char *pDescription);
	uiActionScript *ParseScript(const char *pName, const char *pScript);
	uiActionScript *FindAction(const char *pName);
	void DestroyAction(const char *pName);

	uiActionMetric *CreateMetric(const char *pName, const char *pDescription);
	uiActionMetric *ParseMetric(const char *pMetric);
	uiActionMetric *FindMetric(const char *pName);
	void DestroyMetric(const char *pName);

	uiExecuteContext *RunScript(uiActionScript *pScript, uiEntity *pEntity, uiRuntimeArgs *pArgs);

	bool RunAction(uiExecuteContext *pContext, const char *pAction, uiRuntimeArgs *pArgs, uiEntity *pActionEntity, uiActionScript_Action *pNextAction, uiEntity *pNextEntity);

	MFString GetEntityProperty(uiEntity *pEntity, const char *pProperty);
	void SetEntityProperty(uiEntity *pEntity, const char *pProperty, uiRuntimeArgs *pArguments);
	void SetEntityProperty(uiEntity *pEntity, const char *pProperty, const char *pArgs);

	void DestroyEntity(uiEntity *pEntity);

	uiRuntimeArgs *ParseArgs(const char *pArgs, uiEntity *pEntity);

	ActionType *FindActionType(const char *pName, uiEntity *pEntity);

protected:
	// internal structures

	// private methods
	bool Continue(uiExecuteContext *pContext, uiActionScript_Action *pNextData, uiEntity *pEntity);

	void *Lex(const char *pAction, int *pNumTokens, int preBytes = 0);
	uiActionScript_Action *ParseActions(uiActionScript_Token *pTokens, int numTokens);

	uiRuntimeArgs *ResolveArguments(uiExecuteContext *pContext, uiActionScript_Token *pTokens, int numTokens, MFVector &containerSize, int *pNumUsed = NULL);

	uiRuntimeArgs *ResolveIdentifier(uiExecuteContext *pContext, const char *pIdentifier, MFVector &containerSize);
	uiRuntimeArgs *GetNextValue(uiExecuteContext *pContext, uiActionScript_Token *&pT, int &remaining, MFVector &containerSize, int arrayIndex);
	uiRuntimeArgs *CalculateProducts(uiExecuteContext *pContext, uiActionScript_Token *&pT, int &remaining, MFVector &containerSize, int arrayIndex);

	// private members
	MFPtrListDL<uiExecuteContext> runningActions;
	MFPtrListDL<uiActiveAction> activeDeferredActions;

	HashList<uiActionScript> actions;
	HashList<uiActionMetric> metrics;

	// static members
	static Factory<uiAction> actionFactory;

	static MFObjectPool actionTypePool;
	static HashList<ActionType> actionRegistry;
};

#endif
