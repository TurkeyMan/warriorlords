#include "Warlords.h"
#include "Session.h"

#include <stdio.h>

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
	bIngame = false;
	bLoggedIn = false;

	updating = 0;
	joiningStep = 0;

	search.SetCompleteDelegate(MakeDelegate(this, &Session::OnGamesFound));
	create.SetCompleteDelegate(MakeDelegate(this, &Session::OnCreate));
	join.SetCompleteDelegate(MakeDelegate(this, &Session::OnJoined));
	leave.SetCompleteDelegate(MakeDelegate(this, &Session::OnLeave));

	setRace.SetCompleteDelegate(MakeDelegate(this, &Session::OnRaceSet));
	setColour.SetCompleteDelegate(MakeDelegate(this, &Session::OnColourSet));
	setHero.SetCompleteDelegate(MakeDelegate(this, &Session::OnHeroSet));

	setRaceValue = setColourValue = setHeroValue = -1;

	// attempt to login...
	const char *pLogin = MFFileSystem_Load("home:login_info.ini", NULL, true);
	if(pLogin)
	{
		char *pBreak = MFString_Chr(pLogin, '\n');
		const char *pID = pBreak;
		while(pID && MFIsNewline(*pID))
			++pID;
		*pBreak = 0;

		uint32 id = MFString_AsciiToInteger(pID, false, 16);
		Resume(id);
	}

	// realtime thread
	bKeepConnection = false;
	connectionStage = Disconnected;
	connection = 0;

	bAddressKnown = false;
}

Session::~Session()
{
}

void Session::CloseConnection()
{
	if(connection)
	{
		MFSockets_CloseSocket(connection);
		connection = 0;
	}
	connectionStage = Disconnected;
}

void Session::Update()
{
//	pCurrent->UpdateState();

	if(bKeepConnection)
	{
		if(!bAddressKnown)
		{
#if 1
			// look up address
			MFAddressInfo *pInfo = NULL;
			MFSockets_GetAddressInfo("keywestdata.com", NULL, NULL, &pInfo);

			// do this somewhere else...
			while(pInfo)
			{
				if(pInfo->family == MFAF_Inet)
				{
					serverAddress = *(MFSocketAddressInet*)pInfo->pAddress;
					serverAddress.port = 43210;
					bAddressKnown = true;
					break;
				}

				pInfo = pInfo->pNext;
			}
#else
			serverAddress.cbSize = sizeof(MFSocketAddressInet);
			serverAddress.family = MFAF_Inet;
			serverAddress.address = MFSockets_MakeInetAddress(127, 0, 0, 1);
			serverAddress.port = 43210;
			bAddressKnown = true;
#endif
		}

		if(bAddressKnown)
		{
			char buffer[1024];

			switch(connectionStage)
			{
				case Disconnected:
				{
					// create socket
					connection = MFSockets_CreateSocket(MFAF_Inet, MFSockType_Stream, MFProtocol_TCP);

					uint32 bTrue = 1;
					MFSockets_SetSocketOptions(connection, MFSO_NonBlocking, &bTrue, sizeof(uint32));

					connectionStage = Connecting;
				}
				case Connecting:
				{
					// attempt to connect
					int error = MFSockets_Connect(connection, serverAddress);

					MFSocketError errCode = MFSockets_GetLastError();
					if(error == 0 || errCode == MFSockError_IsConnected)
					{
						MFString packet = MFString::Format("WLClient1.0\n%08x", GetGameID());
						MFSockets_Send(connection, packet.CStr(), packet.NumBytes(), 0);

						connectionStage = Authenticating;
					}
					else
					{
						if(errCode != MFSockError_WouldBlock && errCode != MFSockError_AlreadyInProgress)
							CloseConnection();
						break;
					}
				}
				case Authenticating:
				{
					int bytes = MFSockets_Recv(connection, buffer, sizeof(buffer), 0);
					if(bytes > 0)
					{
						buffer[bytes] = 0;

						if(MFString_Compare("Connection accepted", buffer))
						{
							CloseConnection();
							break;
						}

						connectionStage = Connected;
					}
					else
					{
						if(bytes == 0) // remote closed connection
							CloseConnection();
						break;
					}
				}
				case Connected:
				{
					if(pendingMessages.size() > 0)
					{
						for(int a=0; a<pendingMessages.size(); ++a)
							SendMessageToPeers(pendingMessages[a].CStr());

						pendingMessages.clear();
					}

					// poll for data
					int bytes = MFSockets_Recv(connection, buffer, sizeof(buffer), 0);
					if(bytes > 0)
					{
						buffer[bytes] = 0;

						// handle messages from other clients...
						if(!peerMessageHandler.empty())
						{
							char *pOffset = MFString_Chr(buffer, ':');
							if(pOffset)
							{
								*pOffset++ = 0;
								uint32 user = MFString_AsciiToInteger(buffer, false, 16);

								peerMessageHandler(user, pOffset);
							}
						}
					}
					else if(bytes == 0) // remote closed connection
						CloseConnection();
					break;
				}
				default:
				{
					// something went wrong!
					CloseConnection();
					break;
				}
			}
		}
	}
	else
	{
		if(connection)
			CloseConnection();
		pendingMessages.clear();
	}
}

void Session::SendMessageToPeers(const char *pBuffer)
{
	if(connectionStage != Connected)
	{
		pendingMessages.push(pBuffer);
		return;
	}

	char buffer[1024];
	int len = sprintf(buffer, "%08X:%s", user.id, pBuffer);
	MFSockets_Send(connection, buffer, (int)len, 0);
}

void Session::UpdateGames()
{
	if(updating & 3)
		return;

	updating |= 3;

	// clear the old cache...
	currentGames.clear();
	pendingGames.clear();

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
	pastGames.clear();

	getPast.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetPast));

	// get games
	WLServ_GetPastGames(getPast, user.id);
}

void Session::FindGames(FindDelegate callback)
{
	findHandler = callback;

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
	joiningStep = 1;

	WLServ_GetGameByName(join, game.CStr());
}

void Session::LeaveGame(uint32 game, SessionDelegate callback)
{
	leaveHandler = callback;

	WLServ_LeaveGame(leave, GetUserID(), game);
}

void Session::EnterGame(uint32 game, BeginDelegate callback)
{
	startHandler = callback;

	begin.SetCompleteDelegate(MakeDelegate(this, &Session::OnEnter));
	WLServ_GameState(begin, game);
}

void Session::MakeCurrent(uint32 game)
{
	for(int a=0; a<pendingGames.size(); ++a)
	{
		if(pendingGames[a].id == game)
		{
			bLocalGame = false;
			bIngame = false;
			activeGame = a;
			return;
		}
	}

	for(int a=0; a<currentGames.size(); ++a)
	{
		if(currentGames[a].id == game)
		{
			bLocalGame = false;
			bIngame = true;
			activeGame = a;
			return;
		}
	}
}

GameState *Session::GetCurrentGame(int game)
{
	if(currentGames[game].bLoaded)
		return &currentGames[game].state;
	return NULL;
}

GameDetails *Session::GetPendingGame(int game)
{
	if(pendingGames[game].bLoaded)
		return &pendingGames[game].details;
	return NULL;
}

GameDetails *Session::GetPastGame(int game)
{
	if(pastGames[game].bLoaded)
		return &pastGames[game].details;
	return NULL;
}

void Session::SetRace(uint32 game, int race, SessionDelegate callback)
{
	if(setRaceValue != -1)
		return;
	setRaceValue = race;

	setRaceHandler = callback;
	WLServ_SetRace(setRace, game, GetUserID(), setRaceValue);
}

void Session::SetColour(uint32 game, int colour, SessionDelegate callback)
{
	if(setColourValue != -1)
		return;
	setColourValue = colour;

	setColourHandler = callback;
	WLServ_SetColour(setColour, game, GetUserID(), setColourValue);
}

void Session::SetHero(uint32 game, int hero, SessionDelegate callback)
{
	if(setHeroValue != -1)
		return;
	setHeroValue = hero;

	setHeroHandler = callback;
	WLServ_SetHero(setHero, game, GetUserID(), setHeroValue);
}

void Session::BeginGame(uint32 game, uint32 *pPlayers, int numPlayers)
{
	const GameDetails *pGame = GetActiveLobby();

	begin.SetCompleteDelegate(MakeDelegate(this, &Session::OnBegin));
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

	if(findHandler)
	{
		findHandler(err, this, games, numGames);
	}
}

void Session::OnCreate(HTTPRequest::Status status)
{
	ServerError err = SE_NO_ERROR;

	GameDetails createGame;
	err = WLServResult_GetGameDetails(create, &createGame);
	if(err == SE_NO_ERROR)
	{
		// add new game to pending list
		activeGame = pendingGames.size();

		PendingGame &game = pendingGames.push();
		game.id = createGame.id;
		game.bLoaded = true;
		game.details = createGame;
	}

	if(createHandler)
		createHandler(err, this, err == SE_NO_ERROR ? &createGame : NULL);
}

void Session::OnJoined(HTTPRequest::Status status)
{
	ServerError err = SE_NO_ERROR;

	if(joiningStep == 1)
	{
		// extend pending games
		err = WLServResult_GetGameDetails(join, &joinGame);

		if(err == SE_NO_ERROR)
		{
			WLServ_JoinGame(join, GetUserID(), joinGame.id);
			joiningStep = 2;
			return;
		}
	}
	else if(joiningStep == 2)
	{
		err = WLServResult_GetError(join);

		if(err == SE_NO_ERROR)
		{
			activeGame = pendingGames.size();

			PendingGame &game = pendingGames.push();
			game.id = joinGame.id;
			game.bLoaded= true;
			game.details = joinGame;

			WLServ_GetGameByID(join, game.id);
			joiningStep = 3;
			return;
		}
	}
	else if(joiningStep == 3)
	{
		GameDetails details;
		err = WLServResult_GetGameDetails(join, &details);

		if(err == SE_NO_ERROR)
		{
			pendingGames[activeGame].details = details;
			joiningStep = 0;
		}
	}

	if(joinHandler)
		joinHandler(err, this, err == SE_NO_ERROR ? &pendingGames[activeGame].details : NULL);
}

void Session::OnLeave(HTTPRequest::Status status)
{
	ServerError err = SE_NO_ERROR;
	err = WLServResult_GetError(leave);
	if(err != SE_NO_ERROR)
		return;

	SendMessageToPeers("LEAVE");

	if(leaveHandler)
		leaveHandler(err, this);
}

void Session::OnRaceSet(HTTPRequest::Status status)
{
	ServerError err = SE_SERVER_ERROR;
	if(status == HTTPRequest::CS_Succeeded)
	{
		err = WLServResult_GetError(setRace);
		if(err == SE_NO_ERROR)
		{
			GetLobbyPlayer()->race = setRaceValue;
			SendMessageToPeers(MFStr("SETRACE:%d", setRaceValue));
		}
	}
	setRaceValue = -1;

	setRaceHandler(err, this);
}

void Session::OnColourSet(HTTPRequest::Status status)
{
	ServerError err = SE_SERVER_ERROR;
	if(status == HTTPRequest::CS_Succeeded)
	{
		err = WLServResult_GetError(setColour);
		if(err == SE_NO_ERROR)
		{
			GetLobbyPlayer()->colour = setColourValue;
			SendMessageToPeers(MFStr("SETCOLOUR:%d", setColourValue));
		}
	}
	setColourValue = -1;

	setColourHandler(err, this);
}

void Session::OnHeroSet(HTTPRequest::Status status)
{
	ServerError err = SE_SERVER_ERROR;
	if(status == HTTPRequest::CS_Succeeded)
	{
		err = WLServResult_GetError(setHero);
		if(err == SE_NO_ERROR)
		{
			GetLobbyPlayer()->hero = setHeroValue;
			SendMessageToPeers(MFStr("SETHERO:%d", setHeroValue));
		}
	}
	setHeroValue = -1;

	setHeroHandler(err, this);
}

void Session::OnBegin(HTTPRequest::Status status)
{
	uint32 gameID;
	ServerError err = WLServResult_GetGame(begin, &gameID);

	if(err == SE_NO_ERROR)
	{
		GameDetails *pGame = GetActiveLobby();
		pGame->id = gameID;
	}

	beginHandler(err, this);
}

void Session::OnEnter(HTTPRequest::Status status)
{
	GameState state;
	ServerError err = WLServResult_GetGameState(begin, &state);

	startHandler(err, this, &state);
}

void Session::OnGetCurrent(HTTPRequest::Status status)
{
	// get current games
	uint32 gameList[1024];
	int numGames = sizeof(gameList)/sizeof(gameList[0]);
	ServerError err = WLServResult_GetGameList(getCurrent, "activeGames", gameList, &numGames);
	if(err != SE_NO_ERROR)
	{
		updating ^= 1;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(err, this);
		return;
	}

	getCurrent.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetCurrentGame));

	if(numGames)
	{
		currentGames.resize(numGames);
		for(int a=0; a<numGames; ++a)
		{
			currentGames[a].id = gameList[a];
			currentGames[a].bLoaded = false;
		}

		WLServ_GameState(getCurrent, currentGames[0].id);
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
	GameState state;
	ServerError err = WLServResult_GetGameState(getCurrent, &state);
	if(err == SE_NO_ERROR)
	{
		// find game
		for(int a=0; a<currentGames.size(); ++a)
		{
			if(currentGames[a].id == state.id)
			{
				currentGames[a].state = state;
				currentGames[a].bLoaded = true;
				break;
			}
		}
	}

	// find next game
	int skip = err == SE_NO_ERROR ? 0 : 1;
	for(int a=0; a<currentGames.size(); ++a)
	{
		if(!currentGames[a].bLoaded)
		{
			if(skip == 0)
			{
				WLServ_GameState(getCurrent, currentGames[a].id);
				return;
			}
			else
			{
				currentGames.remove(a--);
				--skip;
			}
		}
	}

	updating ^= 1;
	if((updating & 3) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::OnGetPending(HTTPRequest::Status status)
{
	// get pending games
	uint32 gameList[1024];
	int numGames = sizeof(gameList)/sizeof(gameList[0]);
	ServerError err = WLServResult_GetGameList(getPending, "gamesWaiting", gameList, &numGames);
	if(err != SE_NO_ERROR)
	{
		updating ^= 2;
		if((updating & 3) == 0 && updateHandler)
			updateHandler(err, this);
		return;
	}

	getPending.SetCompleteDelegate(MakeDelegate(this, &Session::OnGetPendingGame));

	if(numGames)
	{
		pendingGames.resize(numGames);
		for(int a=0; a<numGames; ++a)
		{
			pendingGames[a].id = gameList[a];
			pendingGames[a].bLoaded = false;
		}

		WLServ_GetGameByID(getPending, pendingGames[0].id);
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
	GameDetails details;
	ServerError err = WLServResult_GetGameDetails(getPending, &details);
	if(err == SE_NO_ERROR)
	{
		// find game
		for(int a=0; a<pendingGames.size(); ++a)
		{
			if(pendingGames[a].id == details.id)
			{
				pendingGames[a].details = details;
				pendingGames[a].bLoaded = true;
				break;
			}
		}
	}

	// find next game
	int skip = err == SE_NO_ERROR ? 0 : 1;
	for(int a=0; a<pendingGames.size(); ++a)
	{
		if(!pendingGames[a].bLoaded)
		{
			if(skip == 0)
			{
				WLServ_GetGameByID(getPending, pendingGames[a].id);
				return;
			}
			else
			{
				pendingGames.remove(a--);
				--skip;
			}
		}
	}

	updating ^= 2;
	if((updating & 3) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::OnGetPast(HTTPRequest::Status status)
{
	// get past games
	uint32 gameList[1024];
	int numGames = sizeof(gameList)/sizeof(gameList[0]);
	ServerError err = WLServResult_GetGameList(getPast, "pastGames", gameList, &numGames);

	if(err == SE_NO_ERROR)
	{
		pastGames.resize(numGames);
		for(int a=0; a<numGames; ++a)
		{
			pastGames[a].id = gameList[a];
			pastGames[a].bLoaded = false;
		}
	}

	updating ^= 4;
	if((updating & 4) == 0 && updateHandler)
		updateHandler(err, this);
}

void Session::Login(const char *pUsername, const char *pPassword)
{
	login.SetCompleteDelegate(MakeDelegate(this, &Session::OnLogin));
	WLServ_Login(login, pUsername, pPassword);
}

void Session::Resume(uint32 id)
{
	login.SetCompleteDelegate(MakeDelegate(this, &Session::OnLogin));
	WLServ_GetUserByID(login, id);
}

void Session::Logout()
{
	bLoggedIn = false;

	user.id = 0;
	user.userName[0] = 0;
}

void Session::OnLogin(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetUser(login, &user);
	if(err != SE_NO_ERROR)
	{
		if(loginHandler)
			loginHandler(err, this);
		return;
	}

	// we're logged in!
	bLoggedIn = true;

	// reset the game cache
	currentGames.clear();
	pendingGames.clear();
	pastGames.clear();

	if(loginHandler)
		loginHandler(SE_NO_ERROR, this);
}

const GameState *Session::GetActiveGame() const
{
	return bLocalGame ? &offlineGames[activeGame].state : &currentGames[activeGame].state;
}

GameDetails *Session::GetActiveLobby() const
{
	return bLocalGame ? NULL : (GameDetails*)&pendingGames[activeGame].details;
}

GameDetails::Player *Session::GetLobbyPlayer(uint32 id, int *pPlayer) const
{
	GameDetails *pGame = GetActiveLobby();
	if(!pGame)
		return NULL;

	uint32 player = id == -1 ? user.id : id;

	if(pPlayer)
		*pPlayer = -1;

	for(int a=0; a<pGame->maxPlayers; ++a)
	{
		if(pGame->players[a].id == player)
		{
			if(pPlayer)
				*pPlayer = a;
			return &pGame->players[a];
		}
	}

	return NULL;
}

uint32 Session::GetGameID() const
{
	if(IsIngame())
	{
		const GameState *pGame = GetActiveGame();
		return pGame->id;
	}
	else
	{
		const GameDetails *pGame = GetActiveLobby();
		if(pGame)
			return pGame->id;
	}

	return -1;
}

bool Session::IsCreator() const
{
	const GameDetails *pGame = GetActiveLobby();
	if(!pGame)
		return true;

	return pGame->players[0].id == user.id;
}
