#pragma once
#if !defined(_GAMESTATE_H)
#define _GAMESTATE_H

#include "Fuji/MFObjectPool.h"

#include "Map.h"
#include "Unit.h"
#include "Action.h"
#include "Menu/Game/GameUI.h"
#include "MapView.h"

#include "ServerRequest.h"

// *** REMOVE ME ***
#include "Screens/GroupConfig.h"

struct MFFont;
class MFJSONValue;

class Profile;
class Lobby;

class GameState;
class MapScreen;
class Battle;

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

struct PendingAction
{
	struct Move
	{
		Group *pGroup;
		int startX, startY;
		int destX, destY;
		int startMovement[11];
		int endMovement[11];
		PendingAction *pPreviousAction;
		PendingAction *pNextAction;
	};
	struct Regroup
	{
		struct SubRegroup
		{
			Group **ppBefore;
			int numBefore;
			Group **ppAfter;
			int numAfter;
		};

		struct Before
		{
			Group *pGroup;
			PendingAction *pPreviousAction;
		};

		struct After
		{
			Group *pGroup;
			PendingAction *pNextAction;
			int sub;
		};

		int x, y;

		void *pMem;
		Before *pBefore;
		int numBefore;
		After *pAfter;
		int numAfter;
		SubRegroup *pSubRegroups;
		int numSubRegroups;
	};

	PendingActionType type;
	union
	{
		Move move;
		Regroup regroup;
	};
};

struct Player
{
	MFVector colour;

	Profile *pUser;
	int race;
	int colourID;

	bool bEliminated;

	int cursorX, cursorY;

	Unit *pHero[4];
	Castle *pHeroReviveLocation[4];
	int numHeroes;
};

class GameState
{
	friend class Game;
	friend class Editor;
public:
	static void Init();
	static void Deinit();

	static GameState *Get(uint32 id);

	static GameState *NewGame(Lobby *pLobby);
	static GameState *FromJson(MFJSONValue *pJson);

	::History& History()						{ return history; }

	::Map& Map()								{ return map; }
	const ::Map& Map() const					{ return map; }

	MFArray<::Player>& Players()				{ return players; }
	::Player& Player(int player)				{ return players[player]; }
	const ::Player& Player(int player) const	{ return players[player]; }

	uint32 ID() const							{ return gameID; }
	bool IsOnline() const						{ return gameID != 0; }

	Statistics& Stats()							{ return gameStats; }
	const Statistics& Stats() const				{ return gameStats; }

	int CurrentPlayer() const					{ return currentPlayer; }
	int CurrentRound() const					{ return currentRound; }

	bool IsCurrentPlayer(int player) const;
	bool IsMyTurn() const						{ return IsCurrentPlayer(currentPlayer); }

	int NextPlayer();

	int PlayerRace(int player) const			{ return player == -1 ? 0 : players[player].race; }
	MFVector PlayerColour(int player) const		{ return player == -1 ? Map().UnitDefs().GetRaceColour(0) : players[player].colour; }
	int PlayerHeroCount(int player) const		{ return players[player].numHeroes; }
	Unit *PlayerHero(int player, int hero)		{ return players[player].pHero[hero]; }
	bool PlayerHasHero(int player, int hero) const;

	void BeginTurn(int player);

	Group *CreateUnit(int unit, Castle *pCastle, Place *pPlace);

	int AddUnit(Unit *pUnit)		{ int numUnits = units.size(); units.push(pUnit); return numUnits; }
	int AddGroup(Group *pGroup)		{ int numGroups = groups.size(); groups.push(pGroup); return numGroups; }

	bool CanCastleBuildHero(Castle *pCastle, int hero) const;
	void SetHeroRebuildCastle(int player, int hero, Castle *pCastle);

protected:
	GameState(MapTemplate &mapTemplate);
	GameState(MapTemplate &mapTemplate, ::Map &map);
	~GameState();

	void CatchUp() { ReplayUntil(history.NumActions()); }
	void ReplayUntil(int action);
	void ReplayAction(Action *pAction);

	Group *AllocGroup();
	void DestroyGroup(Group *pGroup);

	Unit *CreateUnit(int unit, int player);

	uint32 gameID;

	::Map &map;

	MFArray<::Player> players;

	MFArray<Unit*> units;
	MFArray<Group*> groups;

	int currentPlayer;
	int currentRound;

	::History history;

	Statistics gameStats;

private:
	::Map *pMap;

	static MFObjectPool groupPool;
};

#endif
