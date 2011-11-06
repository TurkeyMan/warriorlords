#include "Warlords.h"
#include "SessionProp.h"
#include "Action.h"

extern Game *pGame;
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
	uiActionManager::RegisterProperty("activegames", GetActiveGames, NULL, pType);

	uiActionManager::RegisterInstantAction("login", LoginAction, pType);
	uiActionManager::RegisterInstantAction("logout", LogoutAction, pType);
	uiActionManager::RegisterInstantAction("findgames", FindGames, pType);

	uiActionManager::RegisterInstantAction("createonline", CreateOnline, pType);
	uiActionManager::RegisterInstantAction("createoffline", CreateOffline, pType);
	uiActionManager::RegisterInstantAction("joingame", JoinGame, pType);
	uiActionManager::RegisterInstantAction("getactivegames", GetCurrentGames, pType);
	uiActionManager::RegisterInstantAction("resumegame", ResumeGame, pType);

	uiActionManager::RegisterInstantAction("editmap", EditMap, pType);
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
	return MFString::Format("\"%s\"", pThis->activeLobby.name);
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

MFString uiSessionProp::GetActiveGames(uiEntity *pEntity)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	MFString games = "{";
	bool bAtLeastOne = false;

	int numGames = pThis->pSession->GetNumCurrentGames();
	for(int a=0; a<numGames; ++a)
	{
		GameState *pState = pThis->pSession->GetCurrentGame(a);
		if(bAtLeastOne)
			games += ",";
		games += pState->name;
		bAtLeastOne = true;
	}

	numGames = pThis->pSession->GetNumPendingGames();
	for(int a=0; a<numGames; ++a)
	{
		GameDetails *pState = pThis->pSession->GetPendingGame(a);
		if(bAtLeastOne)
			games += ",";
		games += pState->name;
		bAtLeastOne = true;
	}

	games += "}";

	return games;
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
	pThis->pSession->FindGames(NULL, pArguments->GetString(0));
}

void uiSessionProp::CreateOnline(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	// create online game
	MFString name = pArguments->GetString(0);
	MFString map = pArguments->GetString(1);
	int turnTime = pArguments->GetInt(2);
	pThis->callback = pArguments->GetString(3);

	// create the game
	MapDetails mapDetails;
	Map::GetMapDetails(map.CStr(), &mapDetails);

	GameCreateDetails details;
	details.pName = name.CStr();
	details.pMap = map.CStr();
	details.turnTime = turnTime;
	details.numPlayers = mapDetails.numPlayers;

	pThis->pSession->CreateGame(details, MakeDelegate(pThis, &uiSessionProp::OnCreateGame));
}

void uiSessionProp::OnCreateGame(ServerError error, Session *pSession, GameDetails *pGame)
{
	if(error == SE_NO_ERROR)
	{
		currentGame = pGame->id;
		activeLobby = *pGame;
	}

	if(!callback.IsEmpty())
	{
		MFString t = error == SE_NO_ERROR ? "false" : "true";

		uiActionManager *pAM = GameData::Get()->GetActionManager();
		uiActionScript *pScript = pAM->FindAction(callback.CStr());
		if(pScript)
		{
			uiRuntimeArgs *pArgs = pAM->ParseArgs(t.CStr(), NULL);
			pAM->RunScript(pScript, NULL, pArgs);
		}
	}
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

void uiSessionProp::GetCurrentGames(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	pThis->callback = pArguments->GetString(0);

	pThis->pSession->SetUpdateDelegate(MakeDelegate(pThis, &uiSessionProp::OnUpdateGames));
	pThis->pSession->UpdateGames();
}

void uiSessionProp::JoinGame(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;

	pThis->callback = pArguments->GetString(1);
	pThis->pSession->JoinGame(pArguments->GetString(0), MakeDelegate(pThis, &uiSessionProp::OnJoinGame));
}

void uiSessionProp::OnJoinGame(ServerError error, Session *pSession, GameDetails *pGame)
{
	if(error == SE_NO_ERROR)
	{
		currentGame = pGame->id;
		activeLobby = *pGame;
	}

	if(!callback.IsEmpty())
	{
		MFString t = error == SE_NO_ERROR ? "false" : "true";

		uiActionManager *pAM = GameData::Get()->GetActionManager();
		uiActionScript *pScript = pAM->FindAction(callback.CStr());
		if(pScript)
		{
			uiRuntimeArgs *pArgs = pAM->ParseArgs(t.CStr(), NULL);
			pAM->RunScript(pScript, NULL, pArgs);
		}
	}
}

void uiSessionProp::OnUpdateGames(ServerError error, Session *pSession)
{
	if(error == SE_NO_ERROR)
	{
		//...?
	}

	if(!callback.IsEmpty())
	{
		MFString t = error == SE_NO_ERROR ? "false" : "true";

		uiActionManager *pAM = GameData::Get()->GetActionManager();
		uiActionScript *pScript = pAM->FindAction(callback.CStr());
		if(pScript)
		{
			uiRuntimeArgs *pArgs = pAM->ParseArgs(t.CStr(), NULL);
			pAM->RunScript(pScript, NULL, pArgs);
		}
	}
}

void uiSessionProp::ResumeGame(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	uiActionManager *pAM = GameData::Get()->GetActionManager();

	MFString name = pArguments->GetString(0);
	MFString resumeGameCallback = pArguments->GetString(1);
	MFString resumeLobbyCallback = pArguments->GetString(2);

	int numGames = pThis->pSession->GetNumPendingGames();
	for(int a=0; a<numGames; ++a)
	{
		GameDetails *pGame = pThis->pSession->GetPendingGame(a);
		if(name.EqualsInsensitive(pGame->name))
		{
			pThis->currentGame = pGame->id;
			pThis->activeLobby = *pGame;

			uiActionScript *pScript = pAM->FindAction(resumeLobbyCallback.CStr());
			if(pScript)
			{
				uiRuntimeArgs *pArgs = pAM->ParseArgs("false", NULL);
				pAM->RunScript(pScript, NULL, pArgs);
			}
			return;
		}
	}

	const char *pError = "true";
	numGames = pThis->pSession->GetNumCurrentGames();
	for(int a=0; a<numGames; ++a)
	{
		GameState *pState = pThis->pSession->GetCurrentGame(a);
		if(name.EqualsInsensitive(pState->name))
		{
			pGame = new Game(pState);
			Game::SetCurrent(pGame);

			pError = "false";
			break;
		}
	}

	uiActionScript *pScript = pAM->FindAction(resumeGameCallback.CStr());
	if(pScript)
	{
		uiRuntimeArgs *pArgs = pAM->ParseArgs(pError, NULL);
		pAM->RunScript(pScript, NULL, pArgs);
	}
}


#include "Editor.h"

extern Editor *pEditor;

void uiSessionProp::EditMap(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSessionProp *pThis = (uiSessionProp*)pEntity;
	uiEntityManager *pEM = GameData::Get()->GetEntityManager();

	MFString map = pArguments->GetString(0);

	// setup game parameters
	GameParams params;
	MFZeroMemory(&params, sizeof(params));
	params.bOnline = false;
	params.bEditMap = true;
	params.gameID = 0;
	params.pMap = map.CStr();

	// hide the lobby
	uiEntity *pLobby = pEM->Find("lobbyscreen");
	pLobby->SetVisible(false);

	// hide the menu backdrop
	uiEntity *pBG = pEM->Find("background");
	pBG->SetVisible(false);

	// start game
	pGame = new Game(&params);
	Game::SetCurrent(pGame);

	pEditor = new Editor(pGame);
	Screen::SetNext(pEditor);
}
