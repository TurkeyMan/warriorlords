#include "Warlords.h"
#include "HTTP.h"
#include "ServerRequest.h"
#include "Session.h"
#include "Profile.h"

#include "Fuji/MFSystem.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if 1//defined(_DEBUG)
	const char *pHostname = "127.0.0.1";
	const int port = 8888;
#else
	const char *pHostname = "warriorlordsserv.appspot.com";
	const int port = 80;
#endif

ServerRequest::ServerRequest(EventDelegate completeDelegate)
: completeDelegate(completeDelegate)
, error(SE_PENDING)
, pStatusString(NULL)
, pErrorString(NULL)
, pJson(NULL)
{
	pRequest = new HTTPRequest(MakeDelegate(this, &ServerRequest::OnHTTPCompleted), MakeDelegate(this, &ServerRequest::OnHTTPEvent));
}

ServerRequest::~ServerRequest()
{
	pRequest->Destroy();

	if(pJson)
		MFParseJSON_DestroyDocument(pJson);
}

void ServerRequest::CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("user", pUsername);
	args[1].SetString("pass", pPassword);
	args[1].SetString("verify", pPassword);
	args[2].SetString("email", pEmail);

	pRequest->Post(pHostname, port, "/api/createaccount", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::Login(const char *pUsername, const char *pPassword)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("user", pUsername);
	args[1].SetString("pass", pPassword);

	pRequest->Post(pHostname, port, "/api/login", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::GetUserByID(uint32 id)
{
	MFDebug_Assert(false, "Not suppoprted!");

	pRequest->Post(pHostname, port, MFStr("/users/%d", id));
}

void ServerRequest::GetUserByName(const char *pUsername)
{
	pRequest->Post(pHostname, port, MFStr("/users/%s", pUsername));
}

void ServerRequest::CreateGame(Session *pSession, const GameCreateDetails *pDetails)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[5];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());
	args[2].SetString("map", pDetails->pMap);
	args[3].SetInt("numplayers", pDetails->numPlayers);
	args[4].SetInt("turntime", pDetails->turnTime);

	pRequest->Post(pHostname, port, "/api/creategame", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::GetGame(Session *pSession, uint32 id, bool bActions, bool bPlayers, int firstAction)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[3];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());

	pRequest->Post(pHostname, port, MFStr("/api/games/%d", id), args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::JoinGame(Session *pSession, uint32 game)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[3];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());
	args[2].SetInt("game", game);

	pRequest->Post(pHostname, port, "/api/joingame", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::LeaveGame(Session *pSession, uint32 game)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[3];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());
	args[2].SetInt("game", game);

	pRequest->Post(pHostname, port, "/api/leavegame", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::ConfigureGame(Session *pSession, uint32 game, int race, int colour, int hero, bool bReady)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[7];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());
	args[2].SetInt("game", game);
	args[3].SetInt("race", race);
	args[4].SetInt("colour", colour);
	args[5].SetInt("hero", hero);
	args[6].SetInt("ready", bReady ? 1 : 0);

	pRequest->Post(pHostname, port, "/api/configuregame", args, sizeof(args)/sizeof(args[0]));
}

void ServerRequest::CommitActions(Session *pSession, uint32 game, Action *pActions, int numActions)
{
	Profile *pProfile = pSession->User();
	MFString key = pSession->SessionKey();

	MFFileHTTPRequestArg args[2];
	args[0].SetInt("user", pProfile->ID());
	args[1].SetString("key", key.CStr());
}

MFJSONValue *ServerRequest::Json()
{
	return MFParseJSON_Root(pJson);
}

void ServerRequest::OnHTTPEvent(HTTPRequest *pReq)
{
}

void ServerRequest::OnHTTPCompleted(HTTPRequest *pReq)
{
	HTTPRequest::Status status = pReq->GetStatus();
	switch(status)
	{
		case HTTPRequest::CS_Succeeded:
		{
			// parse response
			HTTPResponse &response = *pReq->GetResponse();
			pJson = MFParseJSON_Parse(response.GetData());
			if(!pJson)
			{
				error = SE_INVALID_RESPONSE;
				break;
			}

			MFJSONValue *pRoot = Json();
			MFJSONValue *pStatus = pRoot->Member("status");

			if(!pStatus || pStatus->Type() != MFJT_StringType)
			{
				error = SE_INVALID_RESPONSE;
				break;
			}

			pStatusString = pStatus->String();
			if(!MFString_Compare(pStatusString, "error"))
			{
				error = SE_SERVER_ERROR;

				MFJSONValue *pError = pRoot->Member("error");
				if(pError && pError->Type() == MFJT_StringType)
					pErrorString = pError->String();
				if(!pErrorString)
					break;

				if(!MFString_Compare(pErrorString, "missing arguments"))
					error = SE_EXPECTED_ARGUMENTS;
				else if(!MFString_Compare(pErrorString, "invalid username") || !MFString_Compare(pErrorString, "invalid password"))
					error = SE_INVALID_ARGUMENTS;
				else if(!MFString_Compare(pErrorString, "incorrect password") || !MFString_Compare(pErrorString, "not logged in"))
				{
					error = SE_INVALID_LOGIN;

					// TODO: end the current session?

					// trigger a log-out event...
				}
				else if(!MFString_Compare(pErrorString, "user does not exist"))
					error = SE_INVALID_USER;
				else if(!MFString_Compare(pErrorString, "already exists"))
					error = SE_ALREADY_EXISTS;
				else if(!MFString_Compare(pErrorString, "not in game"))
					error = SE_NOT_IN_GAME;
				else if(!MFString_Compare(pErrorString, "already in game"))
					error = SE_ALREADY_PRESENT;
				else if(!MFString_Compare(pErrorString, "game full"))
					error = SE_GAME_FULL;
				else if(!MFString_Compare(pErrorString, "game does not exist"))
					error = SE_INVALID_GAME;
				else if(!MFString_Compare(pErrorString, "no friend request"))
					error = SE_INVALID_USER;
				break;
			}

			error = SE_NO_ERROR;
			break;
		}
		case HTTPRequest::CS_CouldntResolveHost:
			error = SE_CANT_FIND_HOST;
			break;
		case HTTPRequest::CS_CouldntConnect:
			error = SE_CONNECTION_REFUSED;
			break;
		case HTTPRequest::CS_ConnectionLost:
			error = SE_CONNECTION_FAILED;
			break;
		case HTTPRequest::CS_HTTPError:
			error = SE_INVALID_RESPONSE;
			break;
	}

	if(completeDelegate)
		completeDelegate(this);
}

/*
ServerError WLServResult_GetUser(HTTPRequest &request, UserDetails *pUser)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		pUser->id = result.Data("id").AsInt();
		MFString_Copy(pUser->userName, result.Data("name").AsString());

		pUser->creationDate = 0; // result.Data("dateCreated").AsString();

		pUser->won = result.Data("won").AsInt();
		pUser->lost = result.Data("lost").AsInt();
		pUser->played = pUser->won + pUser->lost;

//		MFString_Copy(pUser->channelToken, result.Data("channelToken").AsString());

		// all the game info and stuff...
	}

	return result.error;
}

ServerError WLServResult_GetGame(HTTPRequest &request, uint32 *pGame)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		*pGame = result.Data("id").AsInt();
	}

	return result.error;
}

ServerError WLServResult_GetLobbies(HTTPRequest &request, GameLobby *pGames, int *pLumLobbies)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		*pLumLobbies = MFMin(*pLumLobbies, result.Data("numGames").AsInt());

		Result::Item games = result.Data("games");

		for(int a=0; a<*pLumLobbies; ++a)
		{
			Result::Item game = games[a];
			pGames[a].id = game.Get("id").AsInt();
			MFString_Copy(pGames[a].name, game.Get("name").AsString());
		}
	}

	return result.error;
}

void WLServ_GetGame(HTTPRequest &request, uint32 id)
{
	MFFileHTTPRequestArg args[3];
//	args[0].SetInt("user", );
//	args[1].SetString("key", );

	request.Post(pHostname, port, MFStr("/api/games/%d", id), args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGameDetails(HTTPRequest &request, GameDetails *pGame)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	MFZeroMemory(pGame, sizeof(*pGame));

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		pGame->id = result.Data("id").AsInt();
		MFString_Copy(pGame->name, result.Data("name").AsString());
		MFString_Copy(pGame->map, result.Data("map").AsString());
		pGame->creationDate = 0; //result.Data("dateCreated").AsString();
		pGame->turnTime = result.Data("turnTime").AsInt();
		pGame->maxPlayers = result.Data("maxPlayers").AsInt();

		Result::Item players = result.Data("players");

		pGame->numPlayers = players.Size();
		for(int a=0; a<pGame->numPlayers; ++a)
		{
			Result::Item player = players[a];

			pGame->players[a].id = player.Get("id").AsInt();
			pGame->players[a].race = player.Get("race").AsInt();
			pGame->players[a].colour = player.Get("colour").AsInt();
			pGame->players[a].hero = player.Get("hero").AsInt();
			MFString_CopyN(pGame->players[a].name, player.Get("name").AsString(), sizeof(pGame->players[a].name));
		}
	}

	return result.error;
}

ServerError WLServResult_GetError(HTTPRequest &request)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	return result.error;
}

void WLServ_GameState(HTTPRequest &request, uint32 game)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAMEDETAILS");
	args[1].SetInt("game", game);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGameState(HTTPRequest &request, GameState *pState)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		pState->id = result.Data("id").AsInt();
		MFString_Copy(pState->name, result.Data("name").AsString());
		MFString_Copy(pState->map, result.Data("map").AsString());
		pState->turnTime = result.Data("turnTime").AsInt();
		pState->startDate = 0;//result.Data("startTime").AsString();
		pState->endDate = 0;//result.Data("endTime").AsString();
		pState->state = 0;//result.Data("state").AsString();
		pState->currentPlayer = result.Data("currentPlayer").AsInt();
		pState->currentTurn = result.Data("currentTurn").AsInt();
		pState->timeRemaining = result.Data("timeRemaining").AsInt();
		pState->winner = result.Data("winner").AsInt();

		Result::Item players = result.Data("players");

		pState->numPlayers = players.Size();
		for(int a=0; a<pState->numPlayers; ++a)
		{
			Result::Item player = players[a];

			pState->players[a].id = player.Get("id").AsInt();
			pState->players[a].team = player.Get("team").AsInt();
			pState->players[a].race = player.Get("race").AsInt();
			pState->players[a].colour = player.Get("colour").AsInt();
			pState->players[a].hero = player.Get("hero").AsInt();
			MFString_CopyN(pState->players[a].name, player.Get("name").AsString(), sizeof(pState->players[a].name));
		}
	}

	return result.error;
}

int WLServ_CommitActions(HTTPRequest &request, uint32 game, Action *pActions, int numActions)
{
	if(numActions == 0)
		return 0;

	MFFileHTTPRequestArg args[4];
//	args[0].SetInt("user", user);
//	args[1].SetString("key", user);
	args[2].SetInt("game", game);

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

		Action &action = pActions[a];
		len += sprintf(actionList + len, gpActions[action.action]);

		for(int b=0; b<action.numArgs; ++b)
			len += sprintf(actionList + len, "%s%d", b == 0 ? ":" : ",", action.pArgs[b]);
	}

	args[3].SetString("actions", actionList);

	MFDebug_Log(3, actionList);

	// send the request
	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));

	// return the number of actions actually submitted
	return numActions;
}

ServerError WLServResult_GetActions(HTTPRequest &request)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		int firstAction = result.Data("firstAction").AsInt();
		int totalActions = result.Data("totalActions").AsInt();

		Result::Item actionList = result.Data("actions");
		int numActions = actionList.Size();

		while(gNumActions + numActions > gNumAllocated)
		{
			gNumAllocated = gNumAllocated ? gNumAllocated * 2 : 1024;
			gppActionList = (GameAction**)MFHeap_Realloc(gppActionList, sizeof(GameAction*) * gNumAllocated);
		}

		// process the actions
		for(int a=0; a<numActions; ++a)
		{
			MFDebug_Assert(firstAction + a <= gNumActions, "Trying to load future actions!!");

			if(firstAction + a < gNumActions)
			{
				// compare the action that already exists is the same as the one the server's just given us...
				continue;
			}

			Result::Item action = actionList[a];

			// get action type
			GameActions type = GA_UNKNOWN_ACTION;
			const char *pAction = action.Get("type").AsString();
			for(int b=0; b<GA_MAX; ++b)
			{
				if(!MFString_Compare(pAction, gpActions[b]))
				{
					type = (GameActions)b;
					break;
				}
			}

			MFDebug_Assert(type != GA_UNKNOWN_ACTION, "Unknown action!");

			// read args
			Result::Item args = action.Get("args");
			int numArgs = args.Size();

			GameAction *pNew = GameAction::Create(type, numArgs);

			for(int b=0; b<numArgs; ++b)
				pNew->pArgs[b] = args[b].AsInt();

			// append to the back of the action list
			gppActionList[gNumActions++] = pNew;
		}
	}

	return result.error;
}
*/