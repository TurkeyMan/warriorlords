#if !defined(_SERVER_H)
#define _SERVER_H

enum ServerError
{
	SE_CONNECTION_REFUSED = -4,
	SE_CANT_FIND_HOST = -3,
	SE_CONNECTION_FAILED = -2,
	SE_INVALID_RESPONSE = -1,

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

struct UserDetails
{
	uint32 id;
	char userName[64];
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
	};

	uint32 id;
	char name[64];
	char map[64];
	uint32 creationDate;
	int turnTime;
	int maxPlayers;

	int numPlayers;
	Player players[16];
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

struct GameAction
{
	GameActions action;
	int *pArgs;
	int numArgs;
};

ServerError WLServ_CreateAccount(const char *pUsername, const char *pPassword, const char *pEmail, uint32 *pUserID);
ServerError WLServ_Login(const char *pUsername, const char *pPassword, uint32 *pUserID);

ServerError WLServ_GetUserByID(uint32 id, UserDetails *pUser);
ServerError WLServ_GetUserByName(const char *pUsername, UserDetails *pUser);

ServerError WLServ_GetActiveGames(uint32 user, uint32 *pGames, int *pNumGames);
ServerError WLServ_GetPastGames(uint32 user, uint32 *pGames, int *pNumGames);
ServerError WLServ_GetPendingGames(uint32 user, uint32 *pGames, int *pNumGames);

ServerError WLServ_CreateGame(uint32 user, GameCreateDetails *pDetails, uint32 *pGame);

ServerError WLServ_GetGameByID(uint32 id, GameDetails *pGame);
ServerError WLServ_GetGameByName(const char *pName, GameDetails *pGame);

ServerError WLServ_JoinGame(uint32 user, uint32 game);
ServerError WLServ_FindRandomGame(uint32 user, uint32 *pGame);
ServerError WLServ_LeaveGame(uint32 user, uint32 game);

ServerError WLServ_SetRace(uint32 game, uint32 user, int race);
ServerError WLServ_SetColour(uint32 game, uint32 user, int colour);

ServerError WLServ_BeginGame(uint32 game, uint32 *pGame);

ServerError WLServ_GameState(uint32 game, GameState *pState);

ServerError WLServ_ApplyActions(uint32 game, GameAction *pActions, int numActions);
ServerError WLServ_UpdateState(uint32 game, int lastAction, GameAction **ppActions, int *pNumActions, int *pActionCount);

#endif
