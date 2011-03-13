#include "Warlords.h"
#include "SessionProp.h"
#include "Action.h"

void uiSessionProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Session", Create, "Entity");

	uiActionManager::RegisterProperty("userid", GetUsername, NULL, pType);
	uiActionManager::RegisterProperty("username", GetUsername, NULL, pType);
	uiActionManager::RegisterProperty("online", GetOnline, NULL, pType);

	uiActionManager::RegisterProperty("current", GetCurrentGame, NULL, pType);
	uiActionManager::RegisterProperty("iscurrentonline", GetCurrentGameOnline, NULL, pType);

	uiActionManager::RegisterInstantAction("login", LoginAction, pType);
	uiActionManager::RegisterInstantAction("logout", LogoutAction, pType);
	uiActionManager::RegisterInstantAction("findgames", FindGames, pType);

	uiActionManager::RegisterInstantAction("createonline", CreateOnline, pType);
	uiActionManager::RegisterInstantAction("createoffline", CreateOffline, pType);
	uiActionManager::RegisterInstantAction("joingame", JoinGame, pType);
	uiActionManager::RegisterInstantAction("resumegame", ResumeGame, pType);
	uiActionManager::RegisterInstantAction("entergame", EnterGame, pType);
}

uiSessionProp::uiSessionProp()
{
	pSession = Session::Get();

	bVisible = false;
	size.x = 0.f;
	size.y = 0.f;
}

uiSessionProp::~uiSessionProp()
{
}

void uiSessionProp::Update()
{
	uiEntity::Update();
}

void uiSessionProp::Draw(const uiDrawState &state)
{
	return;
}

void uiSessionProp::RunScript(const char *pScriptName, const char *pParams)
{
	uiActionManager *pAM = GameData::Get()->GetActionManager();
	uiActionScript *pScript = pAM->FindAction(pScriptName);
	if(pScript)
	{
		uiRuntimeArgs *pArgs = pParams ? pAM->ParseArgs(pParams, NULL) : NULL;

		pAM->RunScript(pScript, NULL, pArgs);

		if(pArgs)
			pArgs->Release();
	}
}

MFString uiSessionProp::GetUserID(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	uint32 user = pThis->pSession->GetUserID();
	return MFString::Format("%d", user);
}

MFString uiSessionProp::GetUsername(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	const char *pUsername = pThis->pSession->GetUsername();
	return pUsername ? pUsername : "";
}

MFString uiSessionProp::GetOnline(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	return MFString::Static(pThis->pSession->IsOffline() ? "false" : "true");
}

MFString uiSessionProp::GetCurrentGame(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->gameName;
}

MFString uiSessionProp::GetCurrentGameOnline(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->bOffline ? "false" : "true";
}

void uiSessionProp::LoginAction(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	if(pArguments->GetString(0).IsEmpty() || pArguments->GetString(1).IsEmpty())
	{
		MFDebug_Warn(2, "Invalid login request");

		pThis->RunScript("loginfailed");
		return;
	}

	pThis->pSession->Login(pArguments->GetString(0).CStr(), pArguments->GetString(1).CStr());
}

void uiSessionProp::LogoutAction(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	pThis->pSession->BeginOffline();
}

void uiSessionProp::FindGames(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	pThis->pSession->FindGames(pArguments->GetString(0));
}

void uiSessionProp::CreateOnline(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	MFString name = pArguments->GetString(0);
	MFString map = pArguments->GetString(1);
	int turnTime = pArguments->GetInt(2);
	pThis->callback = pArguments->GetString(3);

	// create online game
}

void uiSessionProp::CreateOffline(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	MFString map = pArguments->GetString(0);
	pThis->callback = pArguments->GetString(1);

	// create offline game
	pThis->gameName = "Offline game";
	pThis->gameID = 0;
	pThis->bOffline = true;

	if(!pThis->callback.IsEmpty())
		pThis->RunScript(pThis->callback.CStr(), "");
}

void uiSessionProp::JoinGame(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	MFString name = pArguments->GetString(0);
	pThis->callback = pArguments->GetString(1);

	// find and join game
}

void uiSessionProp::ResumeGame(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	MFString name = pArguments->GetString(0);
	pThis->callback = pArguments->GetString(1);

	// find and resume game
}

void uiSessionProp::EnterGame(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	// enter the game...

	// enter the game!
	pThis->RunScript("onentergame");
}
