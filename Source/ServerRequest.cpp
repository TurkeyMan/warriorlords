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

ServerError WLServ_CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail, uint32 *pUserID)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "CREATEACCOUNT");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);
	args[3].SetString("email", pEmail);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_Login(const char *pUsername, const char *pPassword, uint32 *pUserID)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LOGIN");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

static ServerError WLServ_GetUser(MFFileHTTPRequestArg *pArgs, UserDetails *pUser)
{
	ServerError err = SE_INVALID_RESPONSE;

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", pArgs, 2);

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_GetUserByID(uint32 id, UserDetails *pUser)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetInt("playerid", id);

	return WLServ_GetUser(args, pUser);
}

ServerError WLServ_GetUserByName(const char *pUsername, UserDetails *pUser)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetString("username", pUsername);

	return WLServ_GetUser(args, pUser);
}

static ServerError WLServ_GetGames(const char *pRequest, uint32 user, uint32 *pGames, int *pNumGames)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", pRequest);
	args[1].SetInt("playerid", user);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	int maxGames = *pNumGames;
	*pNumGames = 0;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_GetActiveGames(uint32 user, uint32 *pGames, int *pNumGames)
{
	return WLServ_GetGames("GETACTIVEGAMES", user, pGames, pNumGames);
}

ServerError WLServ_GetPastGames(uint32 user, uint32 *pGames, int *pNumGames)
{
	return WLServ_GetGames("GETPASTGAMES", user, pGames, pNumGames);
}

ServerError WLServ_GetPendingGames(uint32 user, uint32 *pGames, int *pNumGames)
{
	return WLServ_GetGames("GETWAITINGGAMES", user, pGames, pNumGames);
}

ServerError WLServ_CreateGame(uint32 user, GameCreateDetails *pDetails, uint32 *pGame)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[6];
	args[0].SetString("request", "CREATEGAME");
	args[1].SetString("name", pDetails->pName);
	args[2].SetString("map", pDetails->pMap);
	args[3].SetInt("players", pDetails->numPlayers);
	args[4].SetInt("creator", user);
	args[5].SetInt("turntime", pDetails->turnTime);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

static ServerError WLServ_GetGame(MFFileHTTPRequestArg *pArgs, GameDetails *pGame)
{
	MFZeroMemory(pGame, sizeof(GameDetails));

	ServerError err = SE_INVALID_RESPONSE;

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", pArgs, 2);

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_GetGameByID(uint32 id, GameDetails *pGame)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetInt("gameid", id);

	return WLServ_GetGame(args, pGame);
}

ServerError WLServ_GetGameByName(const char *pName, GameDetails *pGame)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetString("gamename", pName);

	return WLServ_GetGame(args, pGame);
}

ServerError WLServ_JoinGame(uint32 user, uint32 game)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "JOINGAME");
	args[1].SetInt("game", game);
	args[2].SetInt("playerid", user);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_FindRandomGame(uint32 user, uint32 *pGame)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "FINDGAME");
	args[2].SetInt("playerid", user);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_LeaveGame(uint32 user, uint32 game)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LEAVEGAME");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_SetRace(uint32 game, uint32 user, int race)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETRACE");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("race", race);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_SetColour(uint32 game, uint32 user, int colour)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETCOLOUR");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("colour", colour);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_BeginGame(uint32 game, uint32 *pPlayers, int numPlayers, uint32 *pGame)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "BEGINGAME");
	args[1].SetInt("game", game);

	char players[256] = "";
	int len = 0;
	for(int a=0; a<numPlayers; ++a)
		len += sprintf(players + len, a > 0 ? ",%d" : "%d", pPlayers[a]);
	args[2].SetString("players", players);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_GameState(uint32 game, GameState *pState)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAMEDETAILS");
	args[1].SetInt("game", game);

	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

ServerError WLServ_ApplyActions(uint32 game, GameAction *pActions, int numActions)
{
	if(numActions == 0)
		return SE_NO_ERROR;

	ServerError err = SE_INVALID_RESPONSE;

	GameAction *pRemainingActions = NULL;
	int remainingActions = 0;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "APPLYACTIONS");
	args[1].SetInt("game", game);

	// build the actions list
	char actionList[1024] = "";
	int len = 0;
	for(int a=0; a<numActions; ++a)
	{
		if(len > sizeof(actionList)-128)
		{
			pRemainingActions = pActions + a;
			remainingActions = numActions - a;
			break;
		}

		if(len > 0)
			len += sprintf_s(actionList + len, sizeof(actionList) - len, "\n");

		GameAction &action = pActions[a];
		len += sprintf_s(actionList + len, sizeof(actionList) - len, gpActions[action.action]);

		for(int b=0; b<action.numArgs; ++b)
			len += sprintf_s(actionList + len, sizeof(actionList) - len, "%s%d", b == 0 ? ":" : ",", action.pArgs[b]);
	}

	args[2].SetString("actions", actionList);

	MFDebug_Log(3, actionList);

	// send the request
	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
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

	// if we couldn't fit them all in this packet, we'll send another...
	if(remainingActions)
		return WLServ_ApplyActions(game, pRemainingActions, remainingActions);
	return err;
}

ServerError WLServ_UpdateState(uint32 game, int lastAction, GameAction **ppActions, int *pNumActions, int *pActionCount)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "UPDATE");
	args[1].SetInt("game", game);
	args[2].SetInt("firstaction", lastAction);

	const int maxActions = 256;
	static GameAction actions[maxActions];
	static int actionArgs[maxActions*10];

	int numActions = 0;
	int numArgs = 0;

	*ppActions = actions;

	// send the request
	const char *pResponse = HTTP_Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	if(!pResponse)
		return SE_CONNECTION_FAILED;

	char *pLine = strtok((char*)pResponse, "\n");
	while(pLine)
	{
		if(!MFString_CaseCmpN(pLine, "REQUEST", 7))
		{
			err = SE_NO_ERROR;
		}
		else if(!MFString_CaseCmpN(pLine, "COUNT", 5) && pLine[5] == '=')
		{
			*pActionCount = MFString_AsciiToInteger(pLine + 6);
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

	*pNumActions = numActions;

	return err;
}
