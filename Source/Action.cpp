#include "Warlords.h"
#include "Game.h"

#include "Unit.h"

History::~History()
{
	MFDebug_Assert(actions[0].type == AT_BeginGame, "Expected: first action is AT_BeginGame");

	MFHeap_Free(actions[0].beginGame.pCastles);
	MFHeap_Free(actions[0].beginGame.pRuins);
}


MFString History::Write(int firstAction, int lastAction)
{
	MFString text;

	for(int i=0; i<actions.size(); ++i)
	{
		switch(actions[i].type)
		{
			case AT_BeginGame:
			{
				auto &beginGame = actions[i].beginGame;
				MFString players, castles, ruins;
				for(int a=0; a<beginGame.numPlayers; ++a)
					players += MFString::Format(a > 0 ? ", %s" : "%s", beginGame.players[a].Str().CStr());
				for(int a=0; a<beginGame.numCastles; ++a)
					castles += MFString::Format(a > 0 ? ", %s" : "%s", beginGame.pCastles[a].Str().CStr());
				for(int a=0; a<beginGame.numRuins; ++a)
					ruins += MFString::Format(a > 0 ? ", %s" : "%s", beginGame.pRuins[a].Str().CStr());
				text += MFString::Format("BeginGame \"%s\", %d [ %s ], %d [ %s ], %d [ %s ]\n", beginGame.pMap, beginGame.numPlayers, players.CStr(), beginGame.numCastles, castles.CStr(), beginGame.numRuins, ruins.CStr());
				break;
			}
			case AT_BeginTurn:
				text += MFString::Format("BeginTurn %d\n", actions[i].beginTurn.player);
				break;
			case AT_PlayerEliminated:
				text += MFString::Format("Eliminated %d\n", actions[i].playerEliminated.player);
				break;
			case AT_Vectory:
				text += MFString::Format("Victory %d\n", actions[i].victory.player);
				break;
			case AT_SetBuild:
				text += MFString::Format("Build %d: %d\n", actions[i].setBuild.castle, actions[i].setBuild.unit);
				break;
			case AT_CreateUnit:
				text += MFString::Format("Unit %d\n", actions[i].createUnit.unitType);
  				break;
			case AT_CreateGroup:
			{
				auto &createGroup = actions[i].createGroup;
				MFString units;
				for(int a=0; a<11; ++a)
				{
					if(createGroup.units[a] == -1)
						units += a > 0 ? ",_" : "_";
					else
						units += MFString::Format(a > 0 ? ",%d" : "%d", createGroup.units[a]);
				}
				text += MFString::Format("Group [ %s ]\n", units.CStr());
				break;
			}
			case AT_Resurrect:
				text += MFString::Format("Resurrect %d\n", actions[i].resurrect.unit);
				break;
			case AT_Restack:
			{
				auto &restack = actions[i].restack;
				MFString groups;
				for(int a=0; a<restack.numGroups; ++a)
					groups += MFString::Format(a > 0 ? ", %d" : "%d", restack.groupStack[a]);
				text += MFString::Format("Restack {%d,%d}, %d [ %s ]\n", restack.x, restack.y, restack.numGroups, groups.CStr());
				break;
			}
			case AT_Move:
			{
				auto &move = actions[i].move;
				MFString movement;
				bool bFirst = true;
				for(int a=0; a<11; ++a)
				{
					if(move.endMovement[a] == -1)
						continue;
					movement += MFString::Format(bFirst ? "%d" : ", %d", move.endMovement[a]);
					bFirst = false;
				}
				text += MFString::Format("Move %d -> {%d,%d}, [ %s ]\n", move.group, move.destX, move.destY, movement.CStr());
				break;
			}
			case AT_Search:
				text += MFString::Format("Search %d: %d\n", actions[i].search.place, actions[i].search.unit);
				break;
			case AT_Battle:
			{
				auto &battle = actions[i].battle;
				MFString units;
				for(int a=0; a<battle.numUnits; ++a)
					units += MFString::Format(a > 0 ? ", %s" : "%s", battle.pResults[a].Str().CStr());
				text += MFString::Format("Battle %d [ %s ]\n", battle.numUnits, units.CStr());
				break;
			}
			case AT_CaptureCastle:
				text += MFString::Format("CaptureCastle %d\n", actions[i].captureCastle.castle);
				break;
			case AT_CaptureUnits:
				text += MFString::Format("CaptureUnits %d\n", actions[i].captureUnits.group);
				break;
		}
	}

	return text;
}

void History::Read(MFString text)
{

}

void History::PushBeginGame(const char *pMap, Player *pPlayers, int numPlayers, Action::Castle *pCastles, int numCastles, Action::Ruin *pRuins, int numRuins)
{
	Action a;
	a.type = AT_BeginGame;
	a.beginGame.pMap = pMap;
	for(int i=0; i<numPlayers; ++i)
	{
		a.beginGame.players[i].player = pPlayers[i].playerID;
		a.beginGame.players[i].race = pPlayers[i].race;
		a.beginGame.players[i].colour = pPlayers[i].colourID;
	}
	a.beginGame.numPlayers = numPlayers;
	a.beginGame.pCastles = pCastles;
	a.beginGame.numCastles = numCastles;
	a.beginGame.pRuins = pRuins;
	a.beginGame.numRuins = numRuins;
	Push(a);
}

void History::PushBeginTurn(int player)
{
	Action a;
	a.type = AT_BeginTurn;
	a.beginTurn.player = player;
	Push(a);
}

void History::PushPlayerEliminated(int player)
{
	Action a;
	a.type = AT_PlayerEliminated;
	a.playerEliminated.player = player;
	Push(a);
}

void History::PushVictory(int player)
{
	Action a;
	a.type = AT_Vectory;
	a.victory.player = player;
	Push(a);
}

void History::PushSetBuild(uint32 castle, uint32 unitType, uint32 buildTime)
{
	Action a;
	a.type = AT_SetBuild;
	a.setBuild.castle = castle;
	a.setBuild.unit = unitType;
	a.setBuild.buildTime = buildTime;
	Push(a);
}

void History::PushCreateUnit(Unit *pUnit)
{
	pUnit->id = pGame->AddUnit(pUnit);

	Action a;
	a.type = AT_CreateUnit;
	a.createUnit.unitType = pUnit->GetType();
	Push(a);
}

void History::PushCreateGroup(Group *pGroup)
{
	pGroup->id = pGame->AddGroup(pGroup);

	Action a;
	a.type = AT_CreateGroup;
	for(int i=0; i<11; ++i)
	{
		if(i < 5)
			a.createGroup.units[i] = i < pGroup->numForwardUnits ? pGroup->pUnits[i]->GetID() : -1;
		else if(i < 10)
			a.createGroup.units[i] = i - 5 < pGroup->numRearUnits ? pGroup->pUnits[i]->GetID() : -1;
		else
			a.createGroup.units[i] = pGroup->pUnits[i] ? pGroup->pUnits[i]->GetID() : -1;
	}
	Push(a);
}

void History::PushResurrect(Unit *pUnit)
{
	Action a;
	a.type = AT_Resurrect;
	a.resurrect.unit = pUnit->GetID();
	Push(a);
}

void History::PushMoveAction(Group *pGroup)
{
	PendingAction *pA = (PendingAction*)pendingPool.AllocAndZero();
	pA->type = PAT_Move;
	pA->move.pGroup = pGroup;
	pA->move.startX = pGroup->x;
	pA->move.startY = pGroup->y;
	pA->move.destX = pGroup->x;
	pA->move.destY = pGroup->y;
	for(int a=0; a<11; ++a)
	{
		if(a < 5)
			pA->move.startMovement[a] = a < pGroup->numForwardUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else if(a < 10)
			pA->move.startMovement[a] = a - 5 < pGroup->numRearUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else
			pA->move.startMovement[a] = pGroup->pUnits[a] ? pGroup->pUnits[a]->GetMovement() : -1;
		pA->move.endMovement[a] = pA->move.startMovement[a];
	}

	// couple the action to the previous action
	pA->move.pPreviousAction = pGroup->pLastAction;
	if(pGroup->pLastAction)
	{
		PendingAction *pPrev = pGroup->pLastAction;
		switch(pPrev->type)
		{
			case PAT_Move:
				pPrev->move.pNextAction = pA;
				break;

			case PAT_Regroup:
				for(int a=0; a<pPrev->regroup.numAfter; ++a)
				{
					if(pPrev->regroup.pAfter[a].pGroup == pGroup)
						pPrev->regroup.pAfter[a].pNextAction = pA;
				}
				break;
		}
	}

	pGroup->pLastAction = pA;
}

void History::UpdateMoveAction(Group *pGroup)
{
	MapTile *pTile = pGroup->GetTile();
	PendingAction *pAction = pGroup->pLastAction;
	MFDebug_Assert(pAction && pAction->type == PAT_Move, "!?");

	pAction->move.destX = pTile->GetX();
	pAction->move.destY = pTile->GetY();
	for(int a=0; a<11; ++a)
	{
		if(a < 5)
			pAction->move.endMovement[a] = a < pGroup->numForwardUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else if(a < 10)
			pAction->move.endMovement[a] = a - 5 < pGroup->numRearUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else
			pAction->move.endMovement[a] = pGroup->pUnits[a] ? pGroup->pUnits[a]->GetMovement() : -1;
	}
}

void History::PushRegroup(PendingAction::Regroup *pRegroup)
{
	PendingAction *pA = (PendingAction*)pendingPool.AllocAndZero();
	pA->type = PAT_Regroup;

	pA->regroup = *pRegroup;

	// couple the action to each before group's previous action
	for(int a=0; a<pA->regroup.numBefore; ++a)
	{
		PendingAction::Regroup::Before &before = pA->regroup.pBefore[a];

		before.pPreviousAction = before.pGroup->pLastAction;
		if(before.pPreviousAction)
		{
			// couple the action to the previous action
			PendingAction *pPrev = before.pPreviousAction;
			switch(pPrev->type)
			{
				case PAT_Move:
					pPrev->move.pNextAction = pA;
					break;

				case PAT_Regroup:
					for(int a=0; a<pPrev->regroup.numAfter; ++a)
					{
						if(pPrev->regroup.pAfter[a].pGroup == before.pGroup)
							pPrev->regroup.pAfter[a].pNextAction = pA;
					}
					break;
			}
		}
	}
	for(int a=0; a<pRegroup->numAfter; ++a)
	{
		pRegroup->pAfter[a].pGroup->pLastAction = pA;
		pRegroup->pAfter[a].pNextAction = NULL;
	}
}

void History::PushSearch(Unit *pHero, Place *pRuin)
{
	CommitPending(pHero->GetGroup());

	Action a;
	a.type = AT_Search;
	a.search.unit = pHero->GetID();
	a.search.place = pRuin->GetID();
//	a.search.item = pRuin->ruin.item;
	Push(a);
}

void History::PushBattle(Action::Unit *pUnits, int numUnits)
{
	Action a;
	a.type = AT_Battle;
	a.battle.pResults = pUnits;
	a.battle.numUnits = numUnits;
	Push(a);
}

void History::PushCaptureCastle(Group *pGroup, Castle *pCastle)
{
	CommitPending(pGroup);

	Action a;
	a.type = AT_CaptureCastle;
	a.captureCastle.castle = pCastle->id;
	Push(a);
}

void History::PushCaptureUnits(Group *pGroup, Group *pUnits)
{
	CommitPending(pGroup);

	Action a;
	a.type = AT_CaptureUnits;
	a.captureUnits.group = pUnits->GetID();
	Push(a);
}

void History::CommitPending(Group *pGroup)
{
	if(pGroup->pLastAction)
	{
		CommitAction(pGroup->pLastAction);
		pGroup->pLastAction = NULL;
	}
}

void History::CommitAction(PendingAction *pAction)
{
	switch(pAction->type)
	{
		case PAT_Move:
		{
			PendingAction::Move &move = pAction->move;

			if(move.pPreviousAction)
				CommitAction(move.pPreviousAction);

			Action a;
			a.type = AT_Move;
			a.move.group = move.pGroup->GetID();
			a.move.destX = move.destX;
			a.move.destY = move.destY;
			for(int i=0; i<11; ++i)
				a.move.endMovement[i] = move.endMovement[i];
			Push(a);

			// disconnect from action tree
			if(pAction->move.pNextAction)
			{
				PendingAction *pNext = pAction->move.pNextAction;

				switch(pNext->type)
				{
					case PAT_Move:
						pNext->move.pPreviousAction = NULL;
						break;

					case PAT_Regroup:
						for(int a=0; a<pNext->regroup.numBefore; ++a)
						{
							if(pNext->regroup.pBefore[a].pPreviousAction == pAction)
								pNext->regroup.pBefore[a].pPreviousAction = NULL;
						}
						break;
				}
			}
			else
			{
				MFDebug_Assert(pAction->move.pGroup->pLastAction == pAction, "Something's gone wonky?");
				pAction->move.pGroup->pLastAction = NULL;
			}

			break;
		}

		case PAT_Regroup:
		{
			PendingAction::Regroup &regroup = pAction->regroup;

			// commit previous actions for each source group
			for(int i=0; i<regroup.numBefore; ++i)
			{
				if(regroup.pBefore[i].pPreviousAction)
					CommitAction(regroup.pBefore[i].pPreviousAction);
			}

			Action a;
			a.type = AT_Restack;
			a.restack.numGroups = regroup.numAfter;
			a.restack.x = regroup.x;
			a.restack.y = regroup.x;

			// create new groups
			for(int i=0; i<regroup.numAfter; ++i)
			{
				PendingAction::Regroup::After &newGroup = regroup.pAfter[i];
				int sub = newGroup.sub;

				// check if the group wasn't changed
				if(regroup.pSubRegroups[sub].ppBefore[0] != regroup.pSubRegroups[sub].ppAfter[0])
				{
					// if it was, create the new group
					PushCreateGroup(newGroup.pGroup);
				}

				// add group to stack
				a.restack.groupStack[i] = newGroup.pGroup->GetID();

				// disconnect from action tree
				if(newGroup.pNextAction)
				{
					PendingAction *pNext = newGroup.pNextAction;

					switch(pNext->type)
					{
						case PAT_Move:
							pNext->move.pPreviousAction = NULL;
							break;

						case PAT_Regroup:
							for(int a=0; a<pNext->regroup.numBefore; ++a)
							{
								if(pNext->regroup.pBefore[a].pPreviousAction == pAction)
									pNext->regroup.pBefore[a].pPreviousAction = NULL;
							}
							break;
					}
				}
				else
				{
					MFDebug_Assert(newGroup.pGroup->pLastAction == pAction, "Something's gone wonky?");
					newGroup.pGroup->pLastAction = NULL;
				}
			}

			// push the restack action
			Push(a);

			MFHeap_Free(pAction->regroup.pMem);
			break;
		}
	}

	pendingPool.Free(pAction);
}

void History::Push(Action &action)
{
	actions.push(action);

	MFString text = Write();
	MFFileSystem_Save("history.txt", text.CStr(), text.NumBytes());
}
