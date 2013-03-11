#pragma once
#if !defined(_ACTION_H)
#define _ACTION_H

#include "Map.h"

#include "Fuji/MFArray.h"

class Unit;
class Group;

enum ActionType
{
	AT_Move,
	AT_Rearrange,
	AT_Regroup,
	AT_Restack,
	AT_Search,
	AT_CaptureCastle,
	AT_CaptureUnits,
	AT_CreateUnit,
	AT_Battle,
	AT_BuildUnit,
	AT_EndTurn
};

struct Action
{
	ActionType type;
	union
	{
		struct Move
		{
			uint32 group;
			int destX, destY;
			int amountMoved[11];
		} move;
		struct Regroup
		{
			uint32 newGroups[20 * 2][11];
			int numNewGroups;
			uint32 groupStack[20 * 2];
			int numGroups;
		} regroup;
		struct Search
		{
			uint32 unit;
			uint32 place;
			int item;
		} search;
		struct CaptureCastle
		{
			uint32 castle;
		} captureCastle;
		struct CaptureUnits
		{
			uint32 group;
		} captureUnits;
		struct CreateUnit
		{
			uint32 unitType;
		} create;
		struct Battle
		{
			struct Damage
			{
				uint32 unit;
				uint32 life;
			} damage[20 * 2];
			int numUnits;
		} battle;
	};
};

struct PendingAction
{
	ActionType type;
	bool bCommitted;
	union
	{
		struct Move
		{
			Group *pGroup;
			int startX, startY;
			int destX, destY;
			int amountMoved[11];
			Action *pPreviousAction;
		} move;
		struct Regroup
		{
			Group *pBefore[20 * 2];
			int numBefore;
			Group *pAfter[20 * 2];
			int numAfter;
			Action *pPreviousActions[20 * 2];
		} regroup;
	};
};

class History
{
public:

private:
	MFArray<Action> actions;
};

#endif
