#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"
#include "Screens/RequestBox.h"

#include "ServerRequest.h"

#include "MFPoolHeap.h"

struct MFFont;

struct GameState;
class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int race;
	int gold;

	int cursorX, cursorY;

	Unit *pHero;

	uint32 playerID;
};

struct GameParams
{
	uint32 gameID;
	bool bOnline;

	struct Player
	{
		uint32 id;
		int race;
		uint32 colour;
	} players[16];
	int numPlayers;

	const char *pMap;
	bool bEditMap;
};

struct Action
{
	enum ActionType
	{
		AT_Move,
		AT_Rearrange,
		AT_Regroup,
		AT_Search,
		AT_CaptureCastle,
		AT_CaptureUnits
	};

	ActionType type;
	Group *pGroup;
	union Properties
	{
		struct Move
		{
			int startX, startY;
			int startMove[11];
			int destX, destY;
			int destMove[11];
		} move;
		struct Rearrange
		{
			Unit *pBefore[10];
			int beforeForward, beforeRear;
			Unit *pAfter[10];
			int afterForward, afterRear;
		} rearrange;
		struct Regroup
		{
			Group *pBefore[MapTile::MaxUnitsOnTile * 2];
			int numBefore;
			Group *pAfter[MapTile::MaxUnitsOnTile * 2];
			int numAfter;
		} regroup;
		struct Search
		{
			Unit *pUnit;
			Ruin *pRuin;
		} search;
		struct CaptureCastle
		{
			Castle *pCastle;
		} captureCastle;
		struct CaptureUnits
		{
			Group *pUnits;
		} captureUnits;
	} prop;

	Action *pParent;
	Action **ppChildren;
	int numChildren;
};

class Game
{
public:
	Game(GameParams *pParams);
	Game(GameState *pState);
	~Game();

	MapScreen *GetMapScreen() { return pMapScreen; }
	Battle *GetBattleScreen() { return pBattle; }

	void BeginGame();
	void BeginTurn(int player);
	void EndTurn();
	void BeginBattle(Group *pGroup, MapTile *pTarget);
	void EndBattle(Group *pGroup, MapTile *pTarget);

	Map *GetMap() { return pMap; }
	UnitDefinitions *GetUnitDefs() { return pUnitDefs; }

	bool IsOnline() { return bOnline; }

	int GetCurrentTurn() { return currentTurn; }
	int CurrentPlayer() { return currentPlayer; }
	bool IsCurrentPlayer(int player);
	bool IsMyTurn() { return IsCurrentPlayer(CurrentPlayer()); }

	int GetPlayerRace(int player) { return player == -1 ? 0 : players[player].race; }
	int GetPlayerGold(int player) { return player == -1 ? 0 : players[player].gold; }
	MFVector GetPlayerColour(int player) { return player == -1 ? pUnitDefs->GetRaceColour(0) : players[player].colour; }
	Unit *GetPlayerHero(int player) { return players[player].pHero; }

	static void SetCurrent(Game *pGame) { pCurrent = pGame; }
	static Game *GetCurrent() { return pCurrent; }

	Unit *AllocUnit();
	void DestroyUnit(Unit *pUnit);
	Group *AllocGroup();
	void DestroyGroup(Group *pGroup);

	Group *CreateUnit(int unit, Castle *pCastle, bool bCommitUnit = false);

	bool MoveGroupToTile(Group *pGroup, MapTile *pTile);

	void PushMoveAction(Group *pGroup);
	void UpdateMoveAction(Group *pGroup);
	void PushRearrange(Group *pGroup, Unit **ppNewOrder);
	void PushRegroup(Group **ppBefore, int numBefore, Group **ppAfter, int numAfter);
	void PushSearch(Group *pGroup, Ruin *pRuin);
	void PushCaptureCastle(Group *pGroup, Castle *pCastle);
	void PushCaptureUnits(Group *pGroup, Group *pUnits);

	Group *RevertAction(Group *pGroup);
	void CommitActions(Group *pGroup);
	void CommitAllActions();
	void DestroyAction(Action *pAction);

	void DrawWindow(const MFRect &rect);
	void DrawLine(float sx, float sy, float dx, float dy);

	MFFont *GetTextFont() { return pText; }
	MFFont *GetBattleNumbersFont() { return pBattleNumbersFont; }
	MFFont *GetSmallNumbersFont() { return pSmallNumbersFont; }

	void ShowRequest(const char *pMessage, RequestBox::SelectCallback callback, bool bNotification);
	bool DrawRequest();

	void PushAction(GameActions action);
	void PushActionArg(int arg);
	void PushActionArgs(int *pArgs, int numArgs);

	void ApplyActions();

	bool IsCommitPending() { return GetNumPendingActions() > 0; }
	void CommitPending();

	void AddUnit(Unit *pUnit, bool bCommitUnit = false);
	void AddGroup(Group *pGroup, bool bCommitGroup = false);

	void UpdateGameState();
	void ReplayActions(int stopAction = -1);
	void ReplayNextAction();
	int NumPendingActions();

	const char *GetNextActionDesc();

protected:
	void Init(const char *pMap, bool bEdit);

	void AddActions(Action *pAction, Action *pParent);
	void CommitAction(Action *pAction);
	Action *FindFirstDependency(Action *pAction);

	void ReplayAction(GameAction *pAction);

	void Commit(HTTPRequest::Status status);
	void Update(HTTPRequest::Status status);

	MFPoolHeapExpanding units;
	MFPoolHeapExpanding groups;
	MFPoolHeapExpanding actionCache;
	MFPoolHeapCollection actionList;

	// game resources
	MFFont *pText;
	MFFont *pBattleNumbersFont;
	MFFont *pSmallNumbersFont;

	MFMaterial *pWindow;
	MFMaterial *pHorizLine;

	// game data
	Map *pMap;
	UnitDefinitions *pUnitDefs;

	Player players[8];
	int numPlayers;

	Unit **ppUnits;
	uint32 numUnits;
	uint32 numUnitsAllocated;

	Group **ppGroups;
	uint32 numGroups;
	uint32 numGroupsAllocated;

	// screens
	MapScreen *pMapScreen;
	Battle *pBattle;

	RequestBox *pRequestBox;

	// game state data
	bool bOnline;
	bool bUpdating;
	uint32 gameID;

	int lastAction;
	int currentPlayer;
	int currentTurn;

	// commit queue
	static const int MAX_PENDING_ACTIONS = 256;
	static const int MAX_PENDING_ACTION_ARGS = 2048;

	GameAction pendingActions[MAX_PENDING_ACTIONS];
	int actionArgs[MAX_PENDING_ACTION_ARGS];
	int lastCommit, lastCommitArg;
	int nextAction, nextArg;
	int attemptCommit, pendingCommit;

	int GetNumRemainingActions();
	int GetNumRemainingActionArgs();
	int GetNumPendingActions();
	int GetPrevAction();

	// undo history
	Action **ppActionHistory;
	int numTopActions;

	// server actions
	GameAction *pServerActions;
	int firstServerAction, numServerActions, serverActionCount;

	HTTPRequest update;
	HTTPRequest commit;

	static Game *pCurrent;
};

#endif
