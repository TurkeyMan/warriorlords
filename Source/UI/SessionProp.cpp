#include "Warlords.h"
#include "SessionProp.h"
#include "Action.h"

void uiSessionProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Session", Create, "Entity");

	uiActionManager::RegisterProperty("userid", GetUsername, NULL, pType);
	uiActionManager::RegisterProperty("username", GetUsername, NULL, pType);
	uiActionManager::RegisterProperty("online", GetOnline, NULL, pType);

	uiActionManager::RegisterProperty("gamename", GetCurrentGame, NULL, pType);
	uiActionManager::RegisterProperty("isonline", GetCurrentGameOnline, NULL, pType);
	uiActionManager::RegisterProperty("map", GetCurrentGameMap, NULL, pType);
	uiActionManager::RegisterProperty("numplayers", GetCurrentGameNumPlayers, NULL, pType);
	uiActionManager::RegisterProperty("players", GetCurrentGamePlayers, NULL, pType);
	uiActionManager::RegisterProperty("races", GetCurrentGameRaces, NULL, pType);

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

	currentGame = 0;
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

	return MFString::Static(pThis->pSession->IsLoggedIn() ? "true" : "false");
}

MFString uiSessionProp::GetCurrentGame(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->activeLobby.name;
}

MFString uiSessionProp::GetCurrentGameOnline(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->activeLobby.id == 0 ? "false" : "true";
}

MFString uiSessionProp::GetCurrentGameMap(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->activeLobby.map;
}

MFString uiSessionProp::GetCurrentGameNumPlayers(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	return pThis->activeLobby.numPlayers;
}

MFString uiSessionProp::GetCurrentGamePlayers(uiEntity *pEntity)
{
	return "";
}

MFString uiSessionProp::GetCurrentGameRaces(uiEntity *pEntity)
{
	return "";
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
	pThis->pSession->Logout();
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
	GameDetails &game = pThis->activeLobby;
	game.id = 0;
	MFString_Copy(game.name, "Offline game");
	MFString_Copy(game.map, map.CStr());
	Map::GetMapDetails(game.map, &game.mapDetails);
	game.bMapDetailsLoaded = true;
	game.maxPlayers = game.mapDetails.numPlayers;
	game.numPlayers = game.maxPlayers;
	game.turnTime = 0;

	for(int a=0; a<game.numPlayers; ++a)
	{
		game.players[a].id = 0;
		MFString_Copy(game.players[a].name, MFStr("Player %d", a+1));
		game.players[a].colour = 1 + a;
		game.players[a].race = 1;
		game.players[a].hero = 0;
	}

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
