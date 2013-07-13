#pragma once
#if !defined(_ACTION_H)
#define _ACTION_H

#include "Map.h"

#include "Fuji/MFArray.h"
#include "Fuji/MFObjectPool.h"

struct Player;
class Unit;
class Group;
class Castle;
class Place;
class MapTile;
class Game;

enum ActionType
{
	AT_Unknown = -1,

	AT_BeginGame = 0,
	AT_BeginTurn,
	AT_PlayerEliminated,
	AT_Victory,
	AT_SetBuild,
	AT_CreateUnit,
	AT_CreateGroup,
	AT_Resurrect,
	AT_Restack,
	AT_Move,
	AT_Search,
	AT_Battle,
	AT_CaptureCastle,
	AT_CaptureUnits,

	AT_Max
};

enum PendingActionType
{
	PAT_Move,
	PAT_Regroup
};

struct Action
{
	struct Player	{ uint32 id; int race, colour;		MFString Str() const { return MFString::Format("{ %08X, %d, %d }", id, race, colour); }	};
	struct Castle	{ int castle, player;				MFString Str() const { return MFString::Format("{ %d: %d }", castle, player); }				};
	struct Ruin		{ int place, item;					MFString Str() const { return MFString::Format("{ %d: %d }", place, item); }				};
	struct Unit		{ uint32 unit; int health, kills;	MFString Str() const { return MFString::Format("{ %d: %d, %d }", unit, health, kills); }	};

	ActionType type;
	union
	{
		struct BeginGame
		{
			const char *pMap;

			Player players[12];
			int numPlayers;

			Castle *pCastles;
			int numCastles;

			Ruin *pRuins;
			int numRuins;
		} beginGame;
		struct BeginTurn
		{
			int player;
		} beginTurn;
		struct PlayerEliminated
		{
			int player;
		} playerEliminated;
		struct Victory
		{
			int player;
		} victory;
		struct SetBuild
		{
			uint32 castle;
			uint32 unit;
			uint32 buildTime;
		} setBuild;
		struct CreateUnit
		{
			uint32 unitType;
		} createUnit;
		struct CreateGroup
		{
			uint32 units[11];
		} createGroup;
		struct Resurrect
		{
			uint32 unit;
		} resurrect;
		struct Restack
		{
			int x, y;
			uint32 groupStack[20 * 2];
			int numGroups;
		} restack;
		struct Move
		{
			uint32 group;
			int destX, destY;
 			int endMovement[11];
		} move;
		struct Search
		{
			uint32 unit;
			uint32 place;
		} search;
		struct Battle
		{
			Unit *pResults;
			int numUnits;
		} battle;
		struct CaptureCastle
		{
			uint32 castle;
		} captureCastle;
		struct CaptureUnits
		{
			uint32 group;
		} captureUnits;
	};
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

class History
{
public:
	History(Game *pGame) : pendingPool(sizeof(PendingAction), 1024, 1024), pGame(pGame) { }
	~History();

	// serialise
	MFString Write(int firstAction = 0, int lastAction = -1);
	void Read(MFString text);

	// undo-able
	void PushMoveAction(Group *pGroup);
	void UpdateMoveAction(Group *pGroup);

	void PushRegroup(PendingAction::Regroup *pRegroup);

	// hard actions
	void PushBeginGame(const char *pMap, Player *pPlayers, int numPlayers, Action::Castle *pCastles, int numCastles, Action::Ruin *pRuins, int numRuins);

	void PushBeginTurn(int player);
	void PushPlayerEliminated(int player);
	void PushVictory(int player);

	void PushSetBuild(uint32 castle, uint32 unitType, uint32 buildTime);

	void PushCreateUnit(Unit *pUnit);
	void PushResurrect(Unit *pUnit);
	void PushCreateGroup(Group *pGroup);

	void PushSearch(Unit *pHero, Place *pRuin);
	void PushBattle(Action::Unit *pUnits, int numUnits);
	void PushCaptureCastle(Group *pGroup, Castle *pCastle);
	void PushCaptureUnits(Group *pGroup, Group *pUnits);

	void CommitPending(Group *pGroup);
	void PopPending(PendingAction *pAction);

private:
	void CommitAction(PendingAction *pAction);
	void Push(Action &action);

	void Disconnect(PendingAction *pAction, PendingAction *pFrom);

	MFArray<Action> actions;
	MFObjectPool pendingPool;

	Game *pGame;
};

#endif
