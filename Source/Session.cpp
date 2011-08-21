#include "Warlords.h"
#include "Session.h"

Session *Session::pCurrent = NULL;

void Session::InitSession()
{
	pCurrent = new Session();
}

void Session::DeinitSession()
{
	delete pCurrent;
	pCurrent = NULL;
}

Session::Session()
{
	bLoggedIn = false;

	pCurrentGames = NULL;
	pPendingGames = NULL;
	pPastGames = NULL;

	updating = 0;

	search.SetCompleteDelegate(MakeDelegate(this, &Session::OnGamesFound));
	create.SetCompleteDelegate(MakeDelegate(this, &Session::OnCreate));
	join.SetCompleteDelegate(MakeDelegate(this, &Session::OnJoined));

	setRace.SetCompleteDelegate(MakeDelegate(this, &Session::OnRaceSet));
	setColour.SetCompleteDelegate(MakeDelegate(this, &Session::OnColourSet));
	setHero.SetCompleteDelegate(MakeDelegate(this, &Session::OnHeroSet));

	begin.SetCompleteDelegate(MakeDelegate(this, &Session::OnBegin));

	setRaceValue = setColourValue = setHeroValue = -1;
}

Session::~Session()
{
}

void Session::Update()
{
	if(!pCurrent)
		return;

//	pCurrent->UpdateState();
}

void Session::UpdateGames()
{
	if(updating & 3)
		return;

	updating |= 3;

	// clear the old cache...
	numCurrentGames = numPendingGames = 0;

	if(pCurrentGames)
	{
		MFHeap_Free(pCurrentGames);
		pCurrentGames = NULL;
	}
	if(pPendingGames)
	{
		MFHeap_Free(pPendingGames);
		pPendingGames = NULL;
	}

	getCurrent.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetCurrent));
	getPending.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetPending));

	// get games
	WLServ_GetActiveGames(getCurrent, user.id);
	WLServ_GetPendingGames(getPending, user.id);
}

void Session::UpdatePastGames()
{
	if(updating & 4)
		return;

	updating |= 4;

	// clear the old cache...
	numPastGames = 0;

	if(pPastGames)
	{
		MFHeap_Free(pPastGames);
		pPastGames = NULL;
	}

	getPast.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetPast));

	// get games
	WLServ_GetPastGames(getPast, user.id);
}

void Session::FindGames(MFString callback)
{
	findEvent = callback;
	WLServ_FindGames(search, GetUserID());
}

void Session::CreateGame(const GameCreateDetails &details, JoinDelegate callback)
{
	createHandler = callback;

	WLServ_CreateGame(create, GetUserID(), &details);
}

void Session::JoinGame(MFString game, JoinDelegate callback)
{
	MFZeroMemory(&joinGame, sizeof(joinGame));

	joinHandler = callback;
	bJoining = true;

	WLServ_GetGameByName(join, game.CStr());
}

void Session::MakeCurrent(uint32 game)
{
	for(int a=0; a<numPendingGames; ++a)
	{
		if(pPendingGames[a].id == game)
		{
			bLocalGame = false;
			bIngame = false;
			activeGame = a;
			return;
		}
	}

	for(int a=0; a<numCurrentGames; ++a)
	{
		if(pCurrentGames[a].id == game)
		{
			bLocalGame = false;
			bIngame = true;
			activeGame = a;
			return;
		}
	}
}

void Session::SetRace(int race)
{
	if(setRaceValue != -1)
		return;
	setRaceValue = race;

	GameDetails *pGame = GetActiveLobby();
	WLServ_SetRace(setRace, pGame->id, GetUserID(), setRaceValue);
}

void Session::SetColour(int colour)
{
	if(setColourValue != -1)
		return;
	setColourValue = colour;

	GameDetails *pGame = GetActiveLobby();
	WLServ_SetColour(setColour, pGame->id, GetUserID(), setColourValue);
}

void Session::SetHero(int hero)
{
	if(setHeroValue != -1)
		return;
	setHeroValue = hero;

	GameDetails *pGame = GetActiveLobby();
	WLServ_SetHero(setHero, pGame->id, GetUserID(), setHeroValue);
}

void Session::BeginGame(uint32 game, uint32 *pPlayers, int numPlayers)
{
	GameDetails *pGame = GetActiveLobby();

	WLServ_BeginGame(begin, pGame->id, pPlayers, pGame->numPlayers);
}

void Session::OnGamesFound(HTTPRequest::Status status)
{
	const int MaxGames = 20;
	GameLobby games[MaxGames];
	int numGames = MaxGames;

	ServerError err = WLServResult_GetLobbies(search, games, &numGames);
	if(err != SE_NO_ERROR)
	{
		// nothing?
	}

	if(findEvent)
	{
		MFString t = "{ \"";

		for(int a=0; a<numGames; ++a)
		{
			if(a > 0)
				t += "\", \"";
			t += games[a].name;
		}

		t += "\" }";
		
		uiActionManager *pAM = GameData::Get()->GetActionManager();
		uiActionScript *pScript = pAM->FindAction(findEvent.CStr());
		if(pScript)
		{
			uiRuntimeArgs *pArgs = pAM->ParseArgs(t.CStr(), NULL);
			pAM->RunScript(pScript, NULL, pArgs);
		}
	}
}

void Session::OnCreate(HTTPRequest::Status status)
{
	ServerError err = SE_NO_ERROR;

	GameDetails createGame;
	err = WLServResult_GetGameDetails(create, &createGame);

	if(createHandler)
		createHandler(err, this, err == SE_NO_ERROR ? &createGame : NULL);
}

void Session::OnJoined(HTTPRequest::Status status)
{
	ServerError err = SE_NO_ERROR;

	if(bJoining)
	{
		err = WLServResult_GetGameDetails(join, &joinGame);
		if(err == SE_NO_ERROR)
		{
			WLServ_JoinGame(join, GetUserID(), joinGame.id);
			bJoining = false;
			return;
		}
	}
	else
	{
		err = WLServResult_GetError(join);
	}

	if(joinHandler)
		joinHandler(err, this, err == SE_NO_ERROR ? &joinGame : NULL);
}

void Session::OnRaceSet(HTTPRequest::Status status)
{
	if(status == HTTPRequest::CS_Succeeded)
		GetLobbyPlayer()->race = setRaceValue;
	setRaceValue = -1;
}

void Session::OnColourSet(HTTPRequest::Status status)
{
	if(status == HTTPRequest::CS_Succeeded)
		GetLobbyPlayer()->colour = setColourValue;
	setColourValue = -1;
}

void Session::OnHeroSet(HTTPRequest::Status status)
{
	if(status == HTTPRequest::CS_Succeeded)
		GetLobbyPlayer()->hero = setHeroValue;
	setHeroValue = -1;
}

void Session::OnBegin(HTTPRequest::Status status)
{
	GameDetails *pGame = GetActiveLobby();
	ServerError err = WLServResult_GetGame(begin, &pGame->id);

	beginHandler(err, this);
}

void Session::OnGetCurrent(HTTPRequest::Status status)
{
	// get current games
	numCurrentGames = sizeof(currentGames)/sizeof(currentGames[0]);
	ServerError err = WLServResult_GetGameList(getCurrent, "activeGames", currentGames, &numCurrentGames);
	if(err != SE_NO_ERROR)
	{
		updating ^= 1;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(err, this);
		return;
	}

	getCurrent.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetCurrentGame));

	numCurrent = 0;
	if(numCurrentGames)
	{
		pCurrentGames = (GameState*)MFHeap_AllocAndZero(sizeof(GameState) * numCurrentGames);
		WLServ_GameState(getCurrent, currentGames[0]);
	}
	else
	{
		updating ^= 1;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(SE_NO_ERROR, this);
	}
}

void Session::OnGetCurrentGame(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetGameState(getCurrent, &pCurrentGames[numCurrent]);
	if(err == SE_NO_ERROR)
	{
		if(++numCurrent < numCurrentGames)
		{
			WLServ_GameState(getCurrent, currentGames[numCurrent]);
			return;
		}
	}

	updating ^= 1;
	if((updating & 3) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::OnGetPending(HTTPRequest::Status status)
{
	// get pending games
	numPendingGames = sizeof(pendingGames)/sizeof(pendingGames[0]);
	ServerError err = WLServResult_GetGameList(getPending, "gamesWaiting", pendingGames, &numPendingGames);
	if(err != SE_NO_ERROR)
	{
		updating ^= 2;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(err, this);
		return;
	}

	getPending.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetPendingGame));

	numPending = 0;
	if(numPendingGames)
	{
		pPendingGames = (GameDetails*)MFHeap_AllocAndZero(sizeof(GameDetails) * numPendingGames);
		WLServ_GetGameByID(getPending, pendingGames[0]);
	}
	else
	{
		updating ^= 2;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(SE_NO_ERROR, this);
	}
}

void Session::OnGetPendingGame(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetGameDetails(getPending, &pPendingGames[numPending]);
	if(err == SE_NO_ERROR)
	{
		if(++numPending < numPendingGames)
		{
			WLServ_GetGameByID(getPending, pendingGames[numPending]);
			return;
		}
	}

	updating ^= 2;
	if((updating & 3) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::OnGetPast(HTTPRequest::Status status)
{
	// get past games
	numPastGames = sizeof(pastGames)/sizeof(pastGames[0]);
	ServerError err = WLServResult_GetGameList(getPast, "pastGames", pastGames, &numPastGames);

	updating ^= 4;
	if((updating & 4) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::Login(const char *pUsername, const char *pPassword)
{
	login.SetCompleteDelegate(MakeDelegate(this, &Session::OnLogin));
	WLServ_Login(login, pUsername, pPassword);
}

void Session::Logout()
{
	bLoggedIn = false;

	user.id = 0;
	user.userName[0] = 0;
}

void Session::OnLogin(HTTPRequest::Status status)
{
	uiActionManager *pAM = GameData::Get()->GetActionManager();

	ServerError err = WLServResult_GetUser(login, &user);
	if(err != SE_NO_ERROR)
	{
		if(loginHandler)
			loginHandler(err, this);

		uiActionScript *pScript = pAM->FindAction("loginfailed");
		if(pScript)
			pAM->RunScript(pScript, NULL, NULL);
		return;
	}

	// we're logged in!
	bLoggedIn = true;

	// reset the game cache
	numCurrentGames = numPendingGames = numPastGames = 0;

	if(loginHandler)
		loginHandler(SE_NO_ERROR, this);

	uiActionScript *pScript = pAM->FindAction("loginsucceeded");
	if(pScript)
		pAM->RunScript(pScript, NULL, NULL);
}

GameState *Session::GetActiveGame()
{
	return bLocalGame ? &offlineGames[activeGame] : &pCurrentGames[activeGame];
}

GameDetails *Session::GetActiveLobby()
{
	return bLocalGame ? NULL : &pPendingGames[activeGame];
}

GameDetails::Player *Session::GetLobbyPlayer()
{
	GameDetails *pGame = GetActiveLobby();
	if(!pGame)
		return NULL;

	for(int a=0; a<pGame->maxPlayers; ++a)
	{
		if(pGame->players[a].id == user.id)
			return &pGame->players[a];
	}

	return NULL;
}

bool Session::IsCreator()
{
	GameDetails *pGame = GetActiveLobby();
	if(!pGame)
		return true;

	return pGame->players[0].id == user.id;
}
