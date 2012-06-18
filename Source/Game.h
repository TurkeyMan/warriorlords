#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"
#include "Menu/Game/GameUI.h"

#include "ServerRequest.h"

#include "MFPoolHeap.h"


// *** REMOVE ME ***
#include "Screens/GroupConfig.h"


struct MFFont;

struct GameState;
class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int race;
	int startingHero;

	int cursorX, cursorY;

	Unit *pHero[4];
	Castle *pHeroReviveLocation[4];
	int numHeroes;

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
		int hero;
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
	union U
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
			Place *pRuin;
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
	friend class GameUI;
	friend class MapScreen;
public:
	static void SetCurrent(Game *pGame) { pCurrent = pGame; }
	static Game *GetCurrent() { return pCurrent; }

	Game(GameParams *pParams);
	Game(GameState *pState);
	~Game();

	void Update();
	void Draw();

	void SetInputSource(HKWidget *pWidget);
	GameUI *GetUI() { return pGameUI; }

	void ShowMiniMap();

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
	MFVector GetPlayerColour(int player) { return player == -1 ? pUnitDefs->GetRaceColour(0) : players[player].colour; }
	int GetPlayerHeroCount(int player) { return players[player].numHeroes; }
	Unit *GetPlayerHero(int player, int hero) { return players[player].pHero[hero]; }
	bool PlayerHasHero(int player, int hero);

	bool CanCastleBuildHero(Castle *pCastle, int hero);
	void SetHeroRebuildCastle(int player, int hero, Castle *pCastle);

	Unit *AllocUnit();
	void DestroyUnit(Unit *pUnit);
	Group *AllocGroup();
	void DestroyGroup(Group *pGroup);

	Group *CreateUnit(int unit, Castle *pCastle, Place *pPlace, bool bCommitUnit = false);

	bool MoveGroupToTile(Group *pGroup, MapTile *pTile);

	void SelectGroup(Group *pGroup);
	Group *GetSelected() { return pSelection; }

	void PushMoveAction(Group *pGroup);
	void UpdateMoveAction(Group *pGroup);
	void PushRearrange(Group *pGroup, Unit **ppNewOrder);
	void PushRegroup(Group **ppBefore, int numBefore, Group **ppAfter, int numAfter);
	void PushSearch(Group *pGroup, Place *pRuin);
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

	void ShowRequest(const char *pMessage, GameUI::MsgBoxDelegate callback, bool bNotification);

	void ApplyActions();

	void AddUnit(Unit *pUnit, bool bCommitUnit = false);
	void AddGroup(Group *pGroup, bool bCommitGroup = false);

	void ReplayActions(int stopAction = -1);
	void ReplayNextAction();
	int NumPendingActions();

	const char *GetNextActionDesc();

	GameAction *SubmitAction(GameActions action, int numArgs);
	GameAction *SubmitActionArgs(GameActions action, int numArgs, ...);

protected:
	void Init(const char *pMap, bool bEdit);

	bool HandleInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);

	void AddActions(Action *pAction, Action *pParent);
	void CommitAction(Action *pAction);
	Action *FindFirstDependency(Action *pAction);

	void ReplayAction(GameAction *pAction);

	Player* GetPlayer(uint32 user, int *pPlayer);
	void ReceivePeerMessage(uint32 user, const char *pMessage);

	void UpdateUndoButton();

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

	Player players[12];
	int numPlayers;

	Unit **ppUnits;
	uint32 numUnits;
	uint32 numUnitsAllocated;

	Group **ppGroups;
	uint32 numGroups;
	uint32 numGroupsAllocated;

	// UI
	GameUI *pGameUI;

	GroupConfig groupConfig;

	// UI data
	Group *pSelection;

	bool bMoving;
	float countdown;

	MFMaterial *pIcons;

	// screens
	MapScreen *pMapScreen;
	Battle *pBattle;

	// game state data
	bool bOnline;
	bool bUpdating;
	uint32 gameID;

	int currentPlayer;
	int currentTurn;

	// undo history
	Action **ppActionHistory;
	int numTopActions;

	// server actions
	ActionList *pActionList;
	int lastAction;

	static Game *pCurrent;
};

#endif
