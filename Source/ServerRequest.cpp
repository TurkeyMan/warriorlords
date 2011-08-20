#include "Warlords.h"
#include "HTTP.h"
#include "ServerRequest.h"

#include "MFSystem.h"

#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include <libjson/libjson.h>

#if 0//defined(_DEBUG)
	const char *pHostname = "10.0.0.27";
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

static GameAction **gppActionList = NULL;
static int gNumActions = 0;
static int gNumAllocated = 0;

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

class Result
{
public:
	class Item
	{
	public:
		Item(JSONNODE *_pNode)
		{
			pNode = _pNode;
			pArray = NULL;
		}

		JSONNODE *pNode;
		JSONNODE *pArray;

		bool AsBool()
		{
			return !!json_as_bool(pNode);
		}

		int AsInt()
		{
			return json_as_int(pNode);
		}

		float AsFloat()
		{
			return json_as_float(pNode);
		}

		const char *AsString()
		{
			return json_as_string(pNode);
		}

		Item Get(const char *pField)
		{
			return Item(json_get(pNode, pField));
		}

		int Size()
		{
			if(!pArray)
				pArray = json_as_array(pNode);

			return json_size(pArray);
		}

		Item operator [](int index)
		{
			if(!pArray)
				pArray = json_as_array(pNode);

			return Item(json_at(pArray, index));
		}
	};

	Result(const char *pJson)
	: data(NULL)
	{
		error = SE_INVALID_RESPONSE;

		pRoot = json_parse(pJson);
		if(!pRoot)
			return;

		JSONNODE *pReq = json_get(pRoot, "request");
		JSONNODE *pErr = json_get(pRoot, "error");

		if(pReq != NULL && pErr != NULL)
		{
			pRequest = json_as_string(pReq);

			error = (ServerError)json_as_int(pErr);
			pErrorMessage = json_as_string(json_get(pRoot, "message"));
		}

		data.pNode = json_get(pRoot, "response");
	}

	~Result()
	{
		json_delete(pRoot);
	}

	const char *pRequest;

	ServerError error;
	const char *pErrorMessage;

	JSONNODE *pRoot;
	Item data;

	Item Data(const char *pField = NULL)
	{
		if(!pField)
			return data;
		return data.Get(pField);
	}
};

void WLServ_CreateAccount(HTTPRequest &request, const char *pUsername, const char *pPassword, const char *pEmail)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "CREATEACCOUNT");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);
	args[3].SetString("email", pEmail);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_Login(HTTPRequest &request, const char *pUsername, const char *pPassword)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LOGIN");
	args[1].SetString("username", pUsername);
	args[2].SetString("password", pPassword);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetUserByID(HTTPRequest &request, uint32 id)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetInt("playerid", id);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetUserByName(HTTPRequest &request, const char *pUsername, UserDetails *pUser)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETUSER");
	args[1].SetString("username", pUsername);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

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
		MFString_Copy(pUser->userName, result.Data("accountName").AsString());

		pUser->creationDate = 0; // result.Data("dateCreated").AsString();

		pUser->played = result.Data("gamesPlayed").AsInt();
		pUser->won = result.Data("gamesWon").AsInt();
		pUser->lost = result.Data("gamesLost").AsInt();

		MFString_Copy(pUser->channelToken, result.Data("channelToken").AsString());
	}

	return result.error;
}

static void WLServ_GetGames(HTTPRequest &request, const char *pRequest, uint32 user)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", pRequest);
	args[1].SetInt("playerid", user);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetActiveGames(HTTPRequest &request, uint32 user)
{
	WLServ_GetGames(request, "GETACTIVEGAMES", user);
}

void WLServ_GetPastGames(HTTPRequest &request, uint32 user)
{
	WLServ_GetGames(request, "GETPASTGAMES", user);
}

void WLServ_GetPendingGames(HTTPRequest &request, uint32 user)
{
	WLServ_GetGames(request, "GETWAITINGGAMES", user);
}

ServerError WLServResult_GetGameList(HTTPRequest &request, const char *pList, uint32 *pGames, int *pNumGames)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

	HTTPResponse *pResponse = request.GetResponse();
	Result result(pResponse->GetData());

	if(result.error == SE_NO_ERROR)
	{
		Result::Item games = result.Data(pList);

		*pNumGames = games.Size();
		for(int a=0; a<*pNumGames; ++a)
			pGames[a] = games[a].AsInt();
	}

	return result.error;
}

void WLServ_CreateGame(HTTPRequest &request, uint32 user, const GameCreateDetails *pDetails)
{
	MFFileHTTPRequestArg args[6];
	args[0].SetString("request", "CREATEGAME");
	args[1].SetString("name", pDetails->pName);
	args[2].SetString("map", pDetails->pMap);
	args[3].SetInt("players", pDetails->numPlayers);
	args[4].SetInt("creator", user);
	args[5].SetInt("turntime", pDetails->turnTime);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_FindGames(HTTPRequest &request, uint32 user)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "SEARCHGAMES");
	args[1].SetInt("playerid", user);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_FindRandomGame(HTTPRequest &request, uint32 user, uint32 *pGame)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "FINDGAME");
	args[1].SetInt("playerid", user);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
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

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
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

void WLServ_GetGameByID(HTTPRequest &request, uint32 id)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetInt("gameid", id);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_GetGameByName(HTTPRequest &request, const char *pName)
{
	MFFileHTTPRequestArg args[2];
	args[0].SetString("request", "GETGAME");
	args[1].SetString("gamename", pName);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

ServerError WLServResult_GetGameDetails(HTTPRequest &request, GameDetails *pGame)
{
	ServerError err = CheckHTTPError(request.GetStatus());
	if(err != SE_NO_ERROR)
		return err;

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

void WLServ_JoinGame(HTTPRequest &request, uint32 user, uint32 game)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "JOINGAME");
	args[1].SetInt("game", game);
	args[2].SetInt("playerid", user);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_LeaveGame(HTTPRequest &request, uint32 user, uint32 game)
{
	MFFileHTTPRequestArg args[3];
	args[0].SetString("request", "LEAVEGAME");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_SetRace(HTTPRequest &request, uint32 game, uint32 user, int race)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETRACE");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("race", race);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_SetColour(HTTPRequest &request, uint32 game, uint32 user, int colour)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETCOLOUR");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("colour", colour);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
}

void WLServ_SetHero(HTTPRequest &request, uint32 game, uint32 user, int hero)
{
	MFFileHTTPRequestArg args[4];
	args[0].SetString("request", "SETHERO");
	args[1].SetInt("gameid", game);
	args[2].SetInt("playerid", user);
	args[3].SetInt("hero", hero);

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
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

int WLServ_ApplyActions(HTTPRequest &request, uint32 game, GameAction **ppActions, int numActions)
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

		GameAction &action = *ppActions[a];
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

	request.Post(pHostname, port, "/warriorlordsserv", args, sizeof(args)/sizeof(args[0]));
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

GameAction *GameAction::Create(GameActions action, int numArgs)
{
	int bytes = sizeof(GameAction) + sizeof(int)*numArgs;
	GameAction *pAction = (GameAction*)MFHeap_Alloc(bytes);
	pAction->action = action;
	pAction->pArgs = (int*)((char*)pAction + sizeof(GameAction));
	pAction->numArgs = numArgs;
	return pAction;
}

bool GameAction::operator ==(GameAction &action)
{
	if(this->action != action.action || numArgs != action.numArgs)
		return false;
	for(int a=0; a<numArgs; ++a)
		if(pArgs[a] != action.pArgs[a])
			return false;
	return true;
}

const char *GameAction::GetString()
{
	const char *pDesc = MFStr("%s: ", gpActions[action]);
	for(int b=0; b<numArgs; ++b)
		pDesc = MFStr(b > 0 ? "%s, %d" : "%s%d", pDesc, pArgs[b]);
	return pDesc;
}

ActionList::ActionList(uint32 _game)
{
	ppActionList = NULL;
	numServerActions = 0;
	numActions = 0;
	numAllocated = 0;
	commitPending = 0;

	game = _game;

	timeout = 10.f;

	Grow(1024);

	update.SetCompleteDelegate(MakeDelegate(this, &ActionList::RequestCallback));

	Sync();
}

ActionList::~ActionList()
{
	if(ppActionList)
	{
		for(int a=0; a<numActions; ++a)
			MFHeap_Free(ppActionList[a]);
		MFHeap_Free(ppActionList);
	}
}

void ActionList::Update()
{
	timeout -= MFSystem_TimeDelta();
	if(timeout <= 0.f)
	{
		Sync();
		timeout = 10.f;
	}
}

GameAction *ActionList::SubmitAction(GameActions action, int numArgs)
{
	Grow(numActions + 1);

	GameAction *pNew = GameAction::Create(action, numArgs);
	ppActionList[numActions++] = pNew;

	return pNew;
}

GameAction *ActionList::SubmitActionArgs(GameActions action, int numArgs, va_list args)
{
	Grow(numActions + 1);

	GameAction *pNew = GameAction::Create(action, numArgs);
	ppActionList[numActions++] = pNew;

	if(numArgs)
	{
		for(int a=0; a<numArgs; ++a)
			pNew->pArgs[a] = va_arg(args, int);
	}

	return pNew;
}

void ActionList::Sync()
{
	if(update.RequestPending())
		return;

	if(numActions > numServerActions)
	{
		// commit pending actions
		commitPending = WLServ_ApplyActions(update, game, ppActionList + numServerActions, numActions - numServerActions);
	}
	else
	{
		// check for updates from the server
		WLServ_UpdateState(update, game, numActions);
	}

	timeout = 10.f;
}

void ActionList::Grow(int minItems)
{
	// assure there's enough memory
	if(minItems < numAllocated)
		return;

	while(minItems > numAllocated)
		numAllocated = numAllocated ? numAllocated * 2 : 1024;

	ppActionList = (GameAction**)MFHeap_Realloc(ppActionList, sizeof(GameAction*) * numAllocated);
}

void ActionList::RequestCallback(HTTPRequest::Status status)
{
	if(commitPending)
	{
		// finalise a commit
		ServerError err = WLServResult_GetError(update);
		if(err != SE_NO_ERROR)
			return;

		numServerActions += commitPending;
		commitPending = 0;
	}
	else
	{
		// finalise an update
		ServerError err = CheckHTTPError(update.GetStatus());
		if(err != SE_NO_ERROR)
			return;

		HTTPResponse *pResponse = update.GetResponse();
		Result result(pResponse->GetData());

		if(result.error != SE_NO_ERROR)
			return;

		numServerActions = result.Data("totalActions").AsInt();
		int firstAction = result.Data("firstAction").AsInt();

		MFDebug_Assert(firstAction  <= numActions, "Trying to load future actions!!");

		Result::Item actionList = result.Data("actions");
		int actionCount = actionList.Size();

		Grow(numActions + actionCount);

		// process the actions
		for(int a=0; a<actionCount; ++a)
		{
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

			if(firstAction + a < numActions)
			{
				// compare the action that already exists is the same as the one the server's just given us...
				MFDebug_Assert(*pNew == *ppActionList[firstAction + a], "The server record is different than the runtime!");
				delete pNew;
			}
			else
			{
				// append to the back of the action list
				ppActionList[numActions++] = pNew;
			}
		}
	}

	// just in case we didn't get eveything...
	if(numActions != numServerActions)
		Sync();
}
