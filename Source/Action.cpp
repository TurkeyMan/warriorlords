#include "Warlords.h"
#include "Game.h"

#include "Unit.h"

#include <stdio.h>

static const char * const gActions[AT_Max] =
{
	"BeginGame",
	"BeginTurn",
	"Eliminated",
	"Victory",
	"Build",
	"Unit",
	"Group",
	"Resurrect",
	"Restack",
	"Move",
	"Search",
	"Battle",
	"CaptureCastle",
	"CaptureUnits"
};

History::~History()
{
	MFDebug_Assert(actions[0].type == AT_BeginGame, "Expected: first action is AT_BeginGame");

	MFHeap_Free((void*)actions[0].beginGame.pMap);
	MFHeap_Free(actions[0].beginGame.pCastles);
	MFHeap_Free(actions[0].beginGame.pRuins);
}

MFString History::Write(int firstAction, int lastAction)
{
	uint32 last = MFMin((uint32)actions.size(), (uint32)lastAction);
	MFDebug_Assert((uint32)firstAction <= last, "Invalid action range...");

	MFString text;

	for(uint32 i=firstAction; i<last; ++i)
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
				text += MFString::Format("%d BeginGame \"%s\", %d [ %s ], %d [ %s ], %d [ %s ]\n", i, beginGame.pMap, beginGame.numPlayers, players.CStr(), beginGame.numCastles, castles.CStr(), beginGame.numRuins, ruins.CStr());
				break;
			}
			case AT_BeginTurn:
				text += MFString::Format("%d BeginTurn %d\n", i, actions[i].beginTurn.player);
				break;
			case AT_PlayerEliminated:
				text += MFString::Format("%d Eliminated %d\n", i, actions[i].playerEliminated.player);
				break;
			case AT_Victory:
				text += MFString::Format("%d Victory %d\n", i, actions[i].victory.player);
				break;
			case AT_SetBuild:
				text += MFString::Format("%d Build %d: %d\n", i, actions[i].setBuild.castle, actions[i].setBuild.unit);
				break;
			case AT_CreateUnit:
				text += MFString::Format("%d Unit %d\n", i, actions[i].createUnit.unitType);
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
				text += MFString::Format("%d Group [ %s ]\n", i, units.CStr());
				break;
			}
			case AT_Resurrect:
				text += MFString::Format("%d Resurrect %d\n", i, actions[i].resurrect.unit);
				break;
			case AT_Restack:
			{
				auto &restack = actions[i].restack;
				MFString groups;
				for(int a=0; a<restack.numGroups; ++a)
					groups += MFString::Format(a > 0 ? ", %d" : "%d", restack.groupStack[a]);
				text += MFString::Format("%d Restack {%d,%d}, %d [ %s ]\n", i, restack.x, restack.y, restack.numGroups, groups.CStr());
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
				text += MFString::Format("%d Move %d -> {%d,%d}, [ %s ]\n", i, move.group, move.destX, move.destY, movement.CStr());
				break;
			}
			case AT_Search:
				text += MFString::Format("%d Search %d: %d\n", i, actions[i].search.place, actions[i].search.unit);
				break;
			case AT_Battle:
			{
				auto &battle = actions[i].battle;
				MFString units;
				for(int a=0; a<battle.numUnits; ++a)
					units += MFString::Format(a > 0 ? ", %s" : "%s", battle.pResults[a].Str().CStr());
				text += MFString::Format("%d Battle %d [ %s ]\n", i, battle.numUnits, units.CStr());
				break;
			}
			case AT_CaptureCastle:
				text += MFString::Format("%d CaptureCastle %d\n", i, actions[i].captureCastle.castle);
				break;
			case AT_CaptureUnits:
				text += MFString::Format("%d CaptureUnits %d\n", i, actions[i].captureUnits.group);
				break;
		}
	}

	return text;
}

void History::Read(MFString text)
{
	actions.clear();

	// split text into lines
	MFArray<MFString> lines;
	text.SplitLines(lines);

	// parse lines
	for(int l=0; l<lines.size(); ++l)
	{
		// identify action type
		int lineNumber;
		MFString action, line;
		lines[l].Parse("%d %s %s", &lineNumber, &action, &line);

		ActionType at = (ActionType)action.Enumerate(gActions, AT_Max);
		MFDebug_Assert(at != AT_Unknown, "Unknown action type");

		// parse action
		Action a;
		a.type = at;

		switch(at)
		{
			case AT_BeginGame:
			{
				MFString map;
				MFString playerText;
				MFString castleText;
				MFString ruinText;

				// parse the action
				line.Parse("\"%s\", %d [ %s ], %d [ %s ], %d [ %s ]", &map, &a.beginGame.numPlayers, &playerText, &a.beginGame.numCastles, &castleText, &a.beginGame.numRuins, &ruinText);

				// get map
				a.beginGame.pMap = MFString_Dup(map.CStr());

				// parse players
				MFArray<MFString> playerList;
				playerText.Split(playerList, "}");
				for(int i=0; i<playerList.size(); ++i)
				{
					playerList[i].Trim(true, false, " ,");
					playerList[i].Parse("{ %x, %d, %d", &a.beginGame.players[i].id, &a.beginGame.players[i].race, &a.beginGame.players[i].colour);
				}
				MFDebug_Assert(a.beginGame.numPlayers == playerList.size(), "Incorrect number of players!");

				// parse castles
				MFArray<MFString> castleList;
				MFArray<Action::Castle> castles;
				castleText.Split(castleList, "}");
				for(int i=0; i<castleList.size(); ++i)
				{
					Action::Castle c;
					castleList[i].Trim(true, false, " ,");
					castleList[i].Parse("{ %d: %d", &c.castle, &c.player);
					castles.push(c);
				}
				MFDebug_Assert(a.beginGame.numCastles == castles.size(), "Incorrect number of castles!");
				a.beginGame.pCastles = castles.getCopy();

				// parse ruins
				MFArray<MFString> ruinList;
				MFArray<Action::Ruin> ruins;
				ruinText.Split(ruinList, "}");
				for(int i=0; i<ruinList.size(); ++i)
				{
					Action::Ruin r;
					ruinList[i].Trim(true, false, " ,");
					ruinList[i].Parse("{ %d: %d", &r.place, &r.item);
					ruins.push(r);
				}
				MFDebug_Assert(a.beginGame.numRuins == ruins.size(), "Incorrect number of ruins!");
				a.beginGame.pRuins = ruins.getCopy();
				break;
			}
			case AT_BeginTurn:
				line.Parse("%d", &a.beginTurn.player);
				break;
			case AT_PlayerEliminated:
				line.Parse("%d", &a.playerEliminated.player);
				break;
			case AT_Victory:
				line.Parse("%d", &a.victory.player);
				break;
			case AT_SetBuild:
				line.Parse("%d: %d", &a.setBuild.castle, &a.setBuild.unit);
				break;
			case AT_CreateUnit:
				line.Parse("%d", &a.createUnit.unitType);
  				break;
			case AT_CreateGroup:
			{
				line.Trim(true, true, " []");

				MFArray<MFString> units;
				line.Split(units, ",");

				MFDebug_Assert(units.size() == 11, "Incorrect length!");
				for(int i=0; i<11; ++i)
				{
					if(units[i] == "_")
						a.createGroup.units[i] = -1;
					else
						units[i].Parse("%d", &a.createGroup.units[i]);
				}
				break;
			}
			case AT_Resurrect:
				line.Parse("%d", &a.resurrect.unit);
				break;
			case AT_Restack:
			{
				MFString groups;
				line.Parse("{%d,%d}, %d [ %s ]", &a.restack.x, &a.restack.y, &a.restack.numGroups, &groups);

				MFArray<MFString> groupList;
				groups.Split(groupList, ", ");
				MFDebug_Assert(groupList.size() == a.restack.numGroups, "Incorrect length!");

				for(int i=0; i<a.restack.numGroups; ++i)
					groupList[i].Parse("%d", &a.restack.groupStack[i]);
				break;
			}
			case AT_Move:
			{
				MFString moveLeft;
				line.Parse("%d -> {%d,%d}, [ %s ]", &a.move.group, &a.move.destX, &a.move.destY, &moveLeft);

				MFArray<MFString> moveLeftList;
				moveLeft.Split(moveLeftList, ", ");

				int i=0;
				for(; i<moveLeftList.size(); ++i)
					moveLeftList[i].Parse("%d", &a.move.endMovement[i]);
				for(; i<11; ++i)
					a.move.endMovement[i] = -1;
				break;
			}
			case AT_Search:
				line.Parse("%d: %d", &a.search.place, &a.search.unit);
				break;
			case AT_Battle:
			{
				MFString results;
				line.Parse("%d [ %s ]", &a.battle.numUnits, &results);

				MFArray<MFString> resultsList;
				MFArray<Action::Unit> units;
				results.Split(resultsList, "}");
				for(int i=0; i<resultsList.size(); ++i)
				{
					Action::Unit u;
					resultsList[i].Trim(true, false, " ,");
					resultsList[i].Parse("{ %d: %d, %d", &u.unit, &u.health, &u.kills);
					units.push(u);
				}
				MFDebug_Assert(units.size() == a.battle.numUnits, "Incorrect length!");

				a.battle.pResults = units.getCopy();
				break;
			}
			case AT_CaptureCastle:
				line.Parse("%d", &a.captureCastle.castle);
				break;
			case AT_CaptureUnits:
				line.Parse("%d", &a.captureUnits.group);
				break;
		}

		actions.push(a);
	}
}

void History::PushBeginGame(const char *pMap, Action::Player *pPlayers, int numPlayers, Action::Castle *pCastles, int numCastles, Action::Ruin *pRuins, int numRuins)
{
	Action a;
	a.type = AT_BeginGame;
	a.beginGame.pMap = MFString_Dup(pMap);
	for(int i=0; i<numPlayers; ++i)
	{
		a.beginGame.players[i].id = pPlayers[i].id;
		a.beginGame.players[i].race = pPlayers[i].race;
		a.beginGame.players[i].colour = pPlayers[i].race;
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
	a.type = AT_Victory;
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
	Action a;
	a.type = AT_CreateUnit;
	a.createUnit.unitType = pUnit->GetType();
	Push(a);
}

void History::PushCreateGroup(Group *pGroup)
{
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

void History::PushRestack(MapTile *pTile)
{
	Action a;
	a.type = AT_Restack;
	a.restack.x = pTile->GetX();
	a.restack.y = pTile->GetY();
	a.restack.numGroups = pTile->GetNumGroups();
	for(int i=0; i<a.restack.numGroups; ++i)
		a.restack.groupStack[i] = pTile->GetGroup(i)->GetID();
	Push(a);
}

void History::PushResurrect(Unit *pUnit)
{
	Action a;
	a.type = AT_Resurrect;
	a.resurrect.unit = pUnit->GetID();
	Push(a);
}

void History::PushMove(PendingAction *pAction)
{
	Action a;
	a.type = AT_Move;
	a.move.group = pAction->move.pGroup->GetID();
	a.move.destX = pAction->move.destX;
	a.move.destY = pAction->move.destY;
	for(int i=0; i<11; ++i)
		a.move.endMovement[i] = pAction->move.endMovement[i];
	Push(a);
}

void History::PushSearch(Unit *pHero, Place *pRuin)
{
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
	Action a;
	a.type = AT_CaptureCastle;
	a.captureCastle.castle = pCastle->id;
	Push(a);
}

void History::PushCaptureUnits(Group *pGroup, Group *pUnits)
{
	Action a;
	a.type = AT_CaptureUnits;
	a.captureUnits.group = pUnits->GetID();
	Push(a);
}

void History::Push(Action &action)
{
	actions.push(action);

	MFString text = Write();
	MFFileSystem_Save("history.txt", text.CStr(), text.NumBytes());
}
