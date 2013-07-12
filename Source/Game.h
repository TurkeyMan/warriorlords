#pragma once
#if !defined(_GAME_H)
#define _GAME_H

#include "Fuji/MFObjectPool.h"

#include "Map.h"
#include "Unit.h"
#include "Action.h"
#include "Menu/Game/GameUI.h"

#include "ServerRequest.h"

// *** REMOVE ME ***
#include "Screens/GroupConfig.h"


struct MFFont;

struct GameState;
class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int colourID;

	uint32 playerID;

	int race;
	int startingHero;

	bool bEliminated;

	int cursorX, cursorY;

	Unit *pHero[4];
	Castle *pHeroReviveLocation[4];
	int numHeroes;
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

struct Statistics
{
	struct PlayerStats
	{
		int numCastles;
		int numUnits;
		int numHeroes;
		int numTransports;
		int battlesWon;
		int battlesLost;
		int castlesClaimed;
		int castlesLost;
		int transportsClaimed;
	};

	MFArray<PlayerStats[6]> turns;
};

class Game
{
	friend class GameUI;
	friend class MapScreen;
	friend class History;
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

	inline void PushRegroup(PendingAction::Regroup *pRegroup) { history.PushRegroup(pRegroup); }
	inline void PushCaptureUnits(Group *pGroup, Group *pUnits) { history.PushCaptureUnits(pGroup, pUnits); }

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

protected:
	void Init(const char *pMap, bool bEdit);

	bool HandleInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);

	void UpdateUndoButton();

	int NextPlayer();

	void ReplayAction(Action *pAction);

	Player* GetPlayer(uint32 user, int *pPlayer);
	void ReceivePeerMessage(uint32 user, const char *pMessage);

	int AddUnit(Unit *pUnit) { int numUnits = units.size(); units.push(pUnit); return numUnits; }
	int AddGroup(Group *pGroup) { int numGroups = groups.size(); groups.push(pGroup); return numGroups; }

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

	MFArray<Unit*> units;
	MFArray<Group*> groups;

	MFObjectPool unitPool;
	MFObjectPool groupPool;

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
	uint32 gameID;

	int currentPlayer;
	int currentTurn;

	Statistics gameStats;

	// server actions
	History history;
	int lastAction;

	static Game *pCurrent;
};

#endif
