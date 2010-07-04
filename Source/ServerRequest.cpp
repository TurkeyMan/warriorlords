#include "Warlords.h"
#include "HTTP.h"
#include "ServerRequest.h"

#include "string.h"

#if defined(_DEBUG)
	const char *pHostname = "localhost";
	const int port = 8888;
#else
	const char *pHostname = "warriorlordsserv.appspot.com";
	const int port = 80;
#endif

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

ServerError WLServ_BeginGame(uint32 game, uint32 *pGame)
{
	ServerError err = SE_INVALID_RESPONSE;

	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "BEGINGAME");
	args[2].SetInt("game", game);

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
		else if(!MFString_CaseCmpN(pLine, "ERROR", 5) && pLine[5] == '=')
		{
			err = (ServerError)MFString_AsciiToInteger(pLine + 6);
		}

		pLine = strtok(NULL, "\n");
	}

	return err;
}
