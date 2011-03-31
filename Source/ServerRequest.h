#if !defined(_SERVER_H)
#define _SERVER_H

#include "HTTP.h"

enum ServerError
{
	SE_CONNECTION_REFUSED = -5,
	SE_CANT_FIND_HOST = -4,
	SE_CONNECTION_FAILED = -3,
	SE_INVALID_RESPONSE = -2,
	SE_PENDING = -1,

	SE_NO_ERROR = 0,

	SE_SERVER_ERROR,
	SE_INVALID_REQUEST,
	SE_EXPECTED_ARGUMENTS,
	SE_INVALID_ARGUMENTS,
	SE_INVALID_USER,
	SE_INVALID_GAME,
	SE_INVALID_LOGIN,
	SE_GAME_FULL,
	SE_ALREADY_PRESENT,
	SE_NOT_IN_GAME,
	SE_UNABLE_TO_START_GAME,
	SE_INVALID_ACTION
};

enum GameActions
{
	GA_UNKNOWN_ACTION = 0,

	GA_ADDCASTLES,
	GA_ADDRUINS,
	GA_CREATEUNIT,
	GA_CREATEGROUP,
	GA_DESTROYGROUP,
	GA_MOVEGROUP,
	GA_REARRANGEGROUP,
	GA_CLAIMCASTLE,
	GA_SETBUILDING,
	GA_SETBATTLEPLAN,
	GA_SEARCH,
	GA_BATTLE,
	GA_ENDTURN,
	GA_VICTORY,
	GA_CAPTUREUNITS,

	GA_MAX
};

struct GameLobby
{
	uint32 id;
	char name[64];
};

struct UserDetails
{
	uint32 id;
	char userName[64];
	char channelToken[256];
	uint32 creationDate;
	int played, won, lost;
};

struct GameCreateDetails
{
	const char *pName;
	const char *pMap;
	int numPlayers;
	int turnTime;
};

struct GameDetails
{
	struct Player
	{
		uint32 id;
		char name[52];
		int race;
		int colour;
		int hero;
	};

	uint32 id;
	char name[64];
	char map[64];
	uint32 creationDate;
	int turnTime;
	int maxPlayers;

	int numPlayers;
	Player players[16];

	MapDetails mapDetails;
	bool bMapDetailsLoaded;
};

struct GameState
{
	struct Player
	{
		uint32 id;
		char name[52];
		int team;
		int race;
		int colour;
		int hero;
		int lastAction;
	};

	uint32 id;
	char name[64];
	char map[64];
	int turnTime;

	uint32 startDate;
	uint32 endDate;

	int state;
	int currentTurn;
	int currentPlayer;
	int timeRemaining;

	int winner;

	int numPlayers;
	Player players[16];
};

class GameAction
{
public:
	GameActions action;
	int *pArgs;
	int numArgs;

	static GameAction *Create(GameActions action, int numArgs);

	bool operator ==(GameAction &action);
	const char *GetString();
};

class ActionList
{
public:
	ActionList(uint32 game);
	~ActionList();

	void Update();

	int GetNumActions() { return numActions; }
	GameAction *GetAction(int action) { return ppActionList[action]; }

	GameAction *SubmitAction(GameActions action, int numArgs);
	GameAction *SubmitActionArgs(GameActions action, int numArgs, va_list args);

	void Sync();

	GameAction **ppActionList;
	int numServerActions;
	int numActions;
	int numAllocated;
	int commitPending;

	float timeout;

	uint32 game;

	HTTPRequest update;

protected:
	void Grow(int minItems);
	void RequestCallback(HTTPRequest::Status status);
};

const char *WLServ_GetActionName(GameActions action);

void WLServ_CreateAccount(HTTPRequest &request, const char *pUsername, const char *pPassword, const char *pEmail);
void WLServ_Login(HTTPRequest &request, const char *pUsername, const char *pPassword);
void WLServ_GetUserByID(HTTPRequest &request, uint32 id);
void WLServ_GetUserByName(HTTPRequest &request, const char *pUsername);
ServerError WLServResult_GetUser(HTTPRequest &request, UserDetails *pUser);

void WLServ_GetActiveGames(HTTPRequest &request, uint32 user);
void WLServ_GetPastGames(HTTPRequest &request, uint32 user);
void WLServ_GetPendingGames(HTTPRequest &request, uint32 user);
ServerError WLServResult_GetGameList(HTTPRequest &request, const char *pList, uint32 *pGames, int *pNumGames);

void WLServ_CreateGame(HTTPRequest &request, uint32 user, GameCreateDetails *pDetails);
void WLServ_FindGames(HTTPRequest &request, uint32 user);
void WLServ_FindRandomGame(HTTPRequest &request, uint32 user);
void WLServ_BeginGame(HTTPRequest &request, uint32 game, uint32 *pPlayers, int numPlayers);
ServerError WLServResult_GetGame(HTTPRequest &request, uint32 *pGame);
ServerError WLServResult_GetLobbies(HTTPRequest &request, GameLobby *pGames, int *pLumLobbies);

void WLServ_GetGameByID(HTTPRequest &request, uint32 id);
void WLServ_GetGameByName(HTTPRequest &request, const char *pName);
ServerError WLServResult_GetGameDetails(HTTPRequest &request, GameDetails *pGame);

void WLServ_JoinGame(HTTPRequest &request, uint32 user, uint32 game);
void WLServ_LeaveGame(HTTPRequest &request, uint32 user, uint32 game);
void WLServ_SetRace(HTTPRequest &request, uint32 game, uint32 user, int race);
void WLServ_SetColour(HTTPRequest &request, uint32 game, uint32 user, int colour);
void WLServ_SetHero(HTTPRequest &request, uint32 game, uint32 user, int hero);
ServerError WLServResult_GetError(HTTPRequest &request);

void WLServ_GameState(HTTPRequest &request, uint32 game);
ServerError WLServResult_GetGameState(HTTPRequest &request, GameState *pState);

int WLServ_ApplyActions(HTTPRequest &request, uint32 game, GameAction **ppActions, int numActions);
void WLServ_UpdateState(HTTPRequest &request, uint32 game, int lastAction);
ServerError WLServResult_GetActions(HTTPRequest &request);

#endif
