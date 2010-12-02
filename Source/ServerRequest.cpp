#include "Warlords.h"
#include "HTTP.h"
#include "ServerRequest.h"

#include "string.h"
#include "stdio.h"

#if 0//defined(_DEBUG)
	const char *pHostname = "10.0.0.10";
	const int port = 8888;
#else
	const char *pHostname = "warriorlordsserv.appspot.com";
	const int port = 80;
#endif

static const char *gpActions[GA_MAX] = 
{
	"UNKNOWN_ACTION",

	"ADDCASTLES",
	"ADDRUINS",
	"CREATEUNIT",
	"CREATEGROUP",
	"DESTROYGROUP",
	"MOVEGROUP",
	"REARRANGEGROUP",
	"CLAIMCASTLE",
	"SETBUILDING",
	"SETBATTLEPLAN",
	"SEARCH",
	"BATTLE",
	"ENDTURN",
	"VICTORY",
	"CAPTUREUNITS"
};

const char *WLServ_GetActionName(GameActions action)
{
	return gpActions[action];
}

static ServerError CheckHTTPError(HTTPRequest::Status status)
{
	switch(status)
	{
		case HTTPRequest::CS_ResolvingHost:
		case HTTPRequest::CS_WaitingForHost:
		case HTTPRequest::CS_Pending:
			return SE_PENDING;
		case HTTPRequest::CS_CouldntResolveHost:
			return SE_CANT_FIND_HOST;
		case HTTPRequest::CS_CouldntConnect:
			return SE_CONNECTION_REFUSED;
		case HTTPRequest::CS_ConnectionLost:
			return SE_CONNECTION_FAILED;
		case HTTPRequest::CS_HTTPError:
			return SE_INVALID_RESPONSE;
		case HTTPRequest::CS_Succeeded:
		default:
			break;
	}

	return SE_NO_ERROR;
}

void WLServ_CreateAccount(HTTPRequest &request, const char *pUsername, const char *pPassword, const char *pEmail)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "CREATEACCOUNT");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);
	args[3].SetString("email", pEmail);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_Login(HTTPRequest &request, const char *pUsername, const char *pPassword)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LOGIN");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetUser(HTTPRequest &request, uint32 *pUserID)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(pUserID && !MFString_CaseCmpN(pLine, "USERID", 6) && pLine[6] == '=')
		{
			*pUserID = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

void WLServ_GetUserByID(HTTPRequest &request, uint32 id)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetInt("playerid", id);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetUserByName(HTTPRequest &request, const char *pUsername, UserDetails *pUser)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetString("username", pUsername);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetUserDetails(HTTPRequest &request, UserDetails *pUser)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "ID", 2) && pLine[2] == '=')
		{
			pUser->id = MFString_AsciiToInteger(pLine + 3);
		}
		else if(!MFString_CaseCmpN(pLine, "USERNAME", 8) && pLine[8] == '=')
		{
			MFString_Copy(pUser->userName, pLine + 9);
		}
		else if(!MFString_CaseCmpN(pLine, "CREATION", 8) && pLine[8] == '=')
		{
			pUser->creationDate = MFString_AsciiToInteger(pLine + 9);
		}
		else if(!MFString_CaseCmpN(pLine, "PLAYED", 6) && pLine[6] == '=')
		{
			pUser->played = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "WON", 3) && pLine[3] == '=')
		{
			pUser->won = MFString_AsciiToInteger(pLine + 4);
		}
		else if(!MFString_CaseCmpN(pLine, "LOST", 4) && pLine[4] == '=')
		{
			pUser->lost = MFString_AsciiToInteger(pLine + 5);
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

static void WLServ_GetGames(HTTPRequest &request, const char *pRequest, uint32 user)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", pRequest);
	args[1].SetInt("playerid", user);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetActiveGames(HTTPRequest &request, uint32 user)
{
	return WLServ_GetGames(request, "GETACTIVEGAMES", user);
}

void WLServ_GetPastGames(HTTPRequest &request, uint32 user)
{
	return WLServ_GetGames(request, "GETPASTGAMES", user);
}

void WLServ_GetPendingGames(HTTPRequest &request, uint32 user)
{
	return WLServ_GetGames(request, "GETWAITINGGAMES", user);
}

ServerError WLServResult_GetGameList(HTTPRequest &request, uint32 *pGames, int *pNumGames)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	int maxGames = *pNumGames;
	*pNumGames = 0;

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "GAME", 4))
		{
			int index = MFString_AsciiToInteger(pLine + 4);
			const char *pGame = MFString_Chr(pLine + 5, '=');
			if(pGame && index < maxGames)
			{
				pGames[index] = MFString_AsciiToInteger(pGame + 1);
				*pNumGames = index + 1;
			}
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

void WLServ_CreateGame(HTTPRequest &request, uint32 user, GameCreateDetails *pDetails)
{
	MFFileHTTPRequestArg args[6];
	args[0].SetString("request", "CREATEGAME");
	args[1].SetString("name", pDetails->pName);
	args[2].SetString("map", pDetails->pMap);
	args[3].SetInt("players", pDetails->numPlayers);
	args[4].SetInt("creator", user);
	args[5].SetInt("turntime", pDetails->turnTime);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_FindRandomGame(HTTPRequest &request, uint32 user, uint32 *pGame)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "FINDGAME");
	args[2].SetInt("playerid", user);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_BeginGame(HTTPRequest &request, uint32 game, uint32 *pPlayers, int numPlayers)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "BEGINGAME");
	args[1].SetInt("game", game);

	char players[256] = "";
	int len = 0;
	for(int a=0; a<numPlayers; ++a)
		len += sprintf(players + len, a > 0 ? ",%d" : "%d", pPlayers[a]);
	args[2].SetString("players", players);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGame(HTTPRequest &request, uint32 *pGame)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(pGame && !MFString_CaseCmpN(pLine, "GAMEID", 6) && pLine[6] == '=')
		{
			*pGame = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

void WLServ_GetGameByID(HTTPRequest &request, uint32 id)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetInt("gameid", id);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetGameByName(HTTPRequest &request, const char *pName)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetString("gamename", pName);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGameDetails(HTTPRequest &request, GameDetails *pGame)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "ID", 2) && pLine[2] == '=')
		{
			pGame->id = MFString_AsciiToInteger(pLine + 3);
		}
		else if(!MFString_CaseCmpN(pLine, "NAME", 4) && pLine[4] == '=')
		{
			MFString_Copy(pGame->name, pLine + 5);
		}
		else if(!MFString_CaseCmpN(pLine, "MAP", 3) && pLine[3] == '=')
		{
			MFString_Copy(pGame->map, pLine + 4);
		}
		else if(!MFString_CaseCmpN(pLine, "CREATION", 8) && pLine[8] == '=')
		{
			pGame->creationDate = MFString_AsciiToInteger(pLine + 9);
		}
		else if(!MFString_CaseCmpN(pLine, "TURNTIME", 8) && pLine[8] == '=')
		{
			pGame->turnTime = MFString_AsciiToInteger(pLine + 9);
		}
		else if(!MFString_CaseCmpN(pLine, "MAXPLAYERS", 10) && pLine[10] == '=')
		{
			pGame->maxPlayers = MFString_AsciiToInteger(pLine + 11);
		}
		else if(!MFString_CaseCmpN(pLine, "NUMPLAYERS", 10) && pLine[10] == '=')
		{
			pGame->numPlayers = MFString_AsciiToInteger(pLine + 11);
		}
		else if(!MFString_CaseCmpN(pLine, "PLAYER", 6))
		{
			int index = MFString_AsciiToInteger(pLine + 6);
			const char *pPlayer = MFString_Chr(pLine + 7, '=');
			if(pPlayer && index < pGame->numPlayers)
				pGame->players[index].id = MFString_AsciiToInteger(pPlayer + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "RACE", 4))
		{
			int index = MFString_AsciiToInteger(pLine + 4);
			const char *pRace = MFString_Chr(pLine + 5, '=');
			if(pRace && index < pGame->numPlayers)
				pGame->players[index].race = MFString_AsciiToInteger(pRace + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "COLOUR", 6))
		{
			int index = MFString_AsciiToInteger(pLine + 6);
			const char *pColour = MFString_Chr(pLine + 7, '=');
			if(pColour && index < pGame->numPlayers)
				pGame->players[index].colour = MFString_AsciiToInteger(pColour + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "USERNAME", 8))
		{
			int index = MFString_AsciiToInteger(pLine + 8);
			const char *pName = MFString_Chr(pLine + 9, '=');
			if(pName && index < pGame->numPlayers)
			{
				MFString_CopyN(pGame->players[index].name, pName + 1, sizeof(pGame->players[index].name));
				pGame->players[index].name[sizeof(pGame->players[index].name) - 1] = 0;
			}
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

void WLServ_JoinGame(HTTPRequest &request, uint32 user, uint32 game)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "JOINGAME");
	args[1].SetInt("game", game);
	args[2].SetInt("playerid", user);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_LeaveGame(HTTPRequest &request, uint32 user, uint32 game)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LEAVEGAME");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_SetRace(HTTPRequest &request, uint32 game, uint32 user, int race)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETRACE");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("race", race);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_SetColour(HTTPRequest &request, uint32 game, uint32 user, int colour)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETCOLOUR");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("colour", colour);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetError(HTTPRequest &request)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

void WLServ_GameState(HTTPRequest &request, uint32 game)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAMEDETAILS");
	args[1].SetInt("game", game);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGameState(HTTPRequest &request, GameState *pState)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "ID", 2) && pLine[2] == '=')
		{
			pState->id = MFString_AsciiToInteger(pLine + 3);
		}
		else if(!MFString_CaseCmpN(pLine, "NAME", 4) && pLine[4] == '=')
		{
			MFString_Copy(pState->name, pLine + 5);
		}
		else if(!MFString_CaseCmpN(pLine, "MAP", 3) && pLine[3] == '=')
		{
			MFString_Copy(pState->map, pLine + 4);
		}
		else if(!MFString_CaseCmpN(pLine, "TURNTIME", 8) && pLine[8] == '=')
		{
			pState->turnTime = MFString_AsciiToInteger(pLine + 9);
		}
		else if(!MFString_CaseCmpN(pLine, "STARTTIME", 9) && pLine[9] == '=')
		{
			pState->startDate = MFString_AsciiToInteger(pLine + 10);
		}
		else if(!MFString_CaseCmpN(pLine, "ENDTIME", 7) && pLine[7] == '=')
		{
			pState->endDate = MFString_AsciiToInteger(pLine + 8);
		}
		else if(!MFString_CaseCmpN(pLine, "STATE", 5) && pLine[5] == '=')
		{
			pState->state = MFString_AsciiToInteger(pLine + 6);
		}
		else if(!MFString_CaseCmpN(pLine, "CURRENTPLAYER", 13) && pLine[13] == '=')
		{
			pState->currentPlayer = MFString_AsciiToInteger(pLine + 14);
		}
		else if(!MFString_CaseCmpN(pLine, "CURRENTTURN", 11) && pLine[11] == '=')
		{
			pState->currentTurn = MFString_AsciiToInteger(pLine + 12);
		}
		else if(!MFString_CaseCmpN(pLine, "TIMEREMAINING", 13) && pLine[13] == '=')
		{
			pState->timeRemaining = MFString_AsciiToInteger(pLine + 14);
		}
		else if(!MFString_CaseCmpN(pLine, "WINNER", 6) && pLine[6] == '=')
		{
			pState->winner = MFString_AsciiToInteger(pLine + 7);
		}
		else if(!MFString_CaseCmpN(pLine, "NUMPLAYERS", 10) && pLine[10] == '=')
		{
			pState->numPlayers = MFString_AsciiToInteger(pLine + 11);
		}
		else if(!MFString_CaseCmpN(pLine, "PLAYER", 6))
		{
			int index = MFString_AsciiToInteger(pLine + 6);
			const char *pPlayer = MFString_Chr(pLine + 7, '=');
			if(pPlayer && index < pState->numPlayers)
				pState->players[index].id = MFString_AsciiToInteger(pPlayer + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "TEAM", 4))
		{
			int index = MFString_AsciiToInteger(pLine + 4);
			const char *pTeam = MFString_Chr(pLine + 5, '=');
			if(pTeam && index < pState->numPlayers)
				pState->players[index].team = MFString_AsciiToInteger(pTeam + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "RACE", 4))
		{
			int index = MFString_AsciiToInteger(pLine + 4);
			const char *pRace = MFString_Chr(pLine + 5, '=');
			if(pRace && index < pState->numPlayers)
				pState->players[index].race = MFString_AsciiToInteger(pRace + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "COLOUR", 6))
		{
			int index = MFString_AsciiToInteger(pLine + 6);
			const char *pColour = MFString_Chr(pLine + 7, '=');
			if(pColour && index < pState->numPlayers)
				pState->players[index].colour = MFString_AsciiToInteger(pColour + 1);
		}
		else if(!MFString_CaseCmpN(pLine, "USERNAME", 8))
		{
			int index = MFString_AsciiToInteger(pLine + 8);
			const char *pName = MFString_Chr(pLine + 9, '=');
			if(pName && index < pState->numPlayers)
			{
				MFString_CopyN(pState->players[index].name, pName + 1, sizeof(pState->players[index].name));
				pState->players[index].name[sizeof(pState->players[index].name) - 1] = 0;
			}
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}

int WLServ_ApplyActions(HTTPRequest &request, uint32 game, GameAction *pActions, int numActions)
{
	if(numActions == 0)
		return 0;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "APPLYACTIONS");
	args[1].SetInt("game", game);

	// build the actions list
	char actionList[1024 * 64] = "";
	int len = 0;
	for(int a=0; a<numActions; ++a)
	{
		if(len > sizeof(actionList)-128)
		{
			numActions = a;
			break;
		}

		if(len > 0)
			len += sprintf(actionList + len, "\n");

		GameAction &action = pActions[a];
		len += sprintf(actionList + len, gpActions[action.action]);

		for(int b=0; b<action.numArgs; ++b)
			len += sprintf(actionList + len, "%s%d", b == 0 ? ":" : ",", action.pArgs[b]);
	}

	args[2].SetString("actions", actionList);

	MFDebug_Log(3, actionList);

	// send the request
	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	// return the number of actions actually submitted
	return numActions;
}

void WLServ_UpdateState(HTTPRequest &request, uint32 game, int lastAction)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "UPDATE");
	args[1].SetInt("game", game);
	args[2].SetInt("firstaction", lastAction);

	return request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetActions(HTTPRequest &request, GameAction **ppActions, int *pNumActions, int *pActionCount)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;
	err = SE_INVALID_RESPONSE;

	HTTPResponse *pResponse = request.GetResponse();

	const int maxActions = 256;
	static GameAction actions[maxActions];
	static int actionArgs[maxActions*10];

	int numActions = 0;
	int numArgs = 0;

	*ppActions = actions;

	char *pLine = strtok(pResponse->GetData(), "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "COUNT", 5) && pLine[5] == '=')
		{
			*pNumActions = MFString_AsciiToInteger(pLine + 6);
		}
		else if(!MFString_CaseCmpN(pLine, "ACTION", 6) && pLine[6] == '=')
		{
			if(numActions < maxActions && numArgs < maxActions*10 - 20)
			{
				actions[numActions].action = GA_UNKNOWN_ACTION;
				actions[numActions].numArgs = 0;
				actions[numActions].pArgs = &actionArgs[numArgs];

				char *pArgs = MFString_Chr(pLine + 7, ':');
				if(pArgs)
					*pArgs++ = 0;
				for(int a=0; a<GA_MAX; ++a)
				{
					if(!MFString_Compare(pLine + 7, gpActions[a]))
					{
						actions[numActions].action = (GameActions)a;
						break;
					}
				}

				if(actions[numActions].action != GA_UNKNOWN_ACTION)
				{
					while(pArgs)
					{
						actions[numActions].pArgs[actions[numActions].numArgs++] = MFString_AsciiToInteger(pArgs);
						pArgs = MFString_Chr(pArgs, ',');
						pArgs = pArgs ? pArgs + 1 : NULL;
					}

					numArgs += actions[numActions].numArgs;
					++numActions;
				}
			}
		}
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	*pActionCount = *pNumActions;
	*pNumActions = numActions;

	return err;
}
