#include "Warlords.h"
#include "Game.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFRenderer.h"

Game *Game::pCurrent = NULL;

Game::Game(const char *_pMap, bool bEditable)
{
	pText = MFFont_Create("FranklinGothic");
	pBattleNumbersFont = MFFont_Create("Battle");
	pSmallNumbersFont = MFFont_Create("SmallNumbers");

	MFTexture *pTemp = MFTexture_Create("Horiz-Pirates", false);
	pWindow = MFMaterial_Create("Window-Pirates");
	pHorizLine = MFMaterial_Create("Horiz-Pirates");
	MFTexture_Destroy(pTemp);

	SetCurrent(this);

	// create the map and screens
	pMap = Map::Create(this, _pMap, bEditable);
	pUnitDefs = pMap->GetUnitDefinitions();

	pMapScreen = new MapScreen(this);
	pBattle = new Battle(this);

	pRequestBox = new RequestBox;

	currentPlayer = 0;
	currentTurn = 0;

	undoDepth = 0;

	if(bEditable)
	{
		// init the players
		for(int a=0; a<8; ++a)
		{
			players[a].race = a + 1;
			players[a].colour = pUnitDefs->GetRaceColour(a + 1);
		}

		// construct the map
		pMap->ConstructMap(0);
	}
}

Game::~Game()
{
	if(pBattle)
		delete pBattle;
	if(pMapScreen)
		delete pMapScreen;
	if(pRequestBox)
		delete pRequestBox;

	if(pMap)
		pMap->Destroy();

	MFFont_Destroy(pText);
	MFFont_Destroy(pBattleNumbersFont);
	MFFont_Destroy(pSmallNumbersFont);
}

void Game::BeginGame()
{
	const int numRaces = 4;

	// setup players
	for(int a=0; a<8; ++a)
	{
		players[a].race = 1 + (MFRand() % numRaces);
		players[a].gold = 0;
		players[a].cursorX = 0;
		players[a].cursorY = 0;
		players[a].pHero = NULL;
	}

	players[0].race = 1;
	players[1].race = 2;
	players[2].race = 1;
	players[3].race = 3;

	// construct the map
	pMap->ConstructMap();

	// select player colours
	bool bColoursTaken[8] = { false, false, false, false, false, false, false, false };

	for(int a=0; a<numRaces; ++a)
	{
		int colour = players[a].race;
		while(bColoursTaken[colour-1])
			colour = 1 + (MFRand() % numRaces);

		players[a].colour = pUnitDefs->GetRaceColour(colour);
		bColoursTaken[colour-1] = true;
	}

	// HACK
	for(int a=0; a<pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player != -1)
		{
			// set each players map focus on starting castle
			players[pCastle->player].cursorX = pCastle->details.x;
			players[pCastle->player].cursorY = pCastle->details.y;

			// produce starting hero (or flag that hero wants to join on the first turn)
			Group *pGroup = CreateUnit(players[pCastle->player].race - 1, pCastle);
			players[pCastle->player].pHero = pGroup->GetUnit(0);

			// pirates also start with a galleon
			if(players[pCastle->player].race == 3)
				CreateUnit(38, pCastle);
		}
	}

	BeginTurn(0);

	Screen::SetNext(pMapScreen);
}

void Game::BeginTurn(int player)
{
	if(player < currentPlayer)
		++currentTurn;

	currentPlayer = player;

	// heal units and restore movement
	int width, height;
	pMap->GetMapSize(&width, &height);

	for(int y=0; y<height; ++y)
	{
		for(int x=0; x<width; ++x)
		{
			MapTile *pTile = pMap->GetTile(x, y);

			int numGroups = pTile->GetNumGroups();
			if(numGroups && pTile->IsFriendlyTile(player))
			{
				for(int a=0; a<numGroups; ++a)
				{
					Group *pGroup = pTile->GetGroup(a);

					Unit *pVehicle = pGroup->GetVehicle();
					if(pVehicle)
						pVehicle->Restore();

					int numUnits = pGroup->GetNumUnits();
					for(int b=0; b<numUnits; ++b)
					{
						Unit *pUnit = pGroup->GetUnit(b);
						pUnit->Restore();
					}
				}
			}
		}
	}

	// get money + add new units
	int numCastles = 0;

	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			++numCastles;
			players[currentPlayer].gold += pCastle->details.income;

			if(pCastle->building != -1 && --pCastle->buildTime <= 0)
			{
				BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

				if(buildUnit.cost <= players[currentPlayer].gold)
				{
					Group *pGroup = CreateUnit(buildUnit.unit, pCastle);
					if(pGroup)
					{
						players[currentPlayer].gold -= buildUnit.cost;

						pCastle->SetBuildUnit(pCastle->building);

						// HACK: Skeletons build 2 at a time!
						if(buildUnit.unit == 14)
						{
							if(pGroup->GetTile()->GetNumUnits() < 10)
							{
								// if there's space on the same tile, add the second one to the same group
								Unit *pUnit = pUnitDefs->CreateUnit(buildUnit.unit, pCastle->player);
								pGroup->AddUnit(pUnit);
							}
							else
							{
								CreateUnit(buildUnit.unit, pCastle);
							}
						}
					}
				}
			}
		}
	}

	// if the player has no castles left, go to next player
	if(numCastles == 0)
	{
		int numPlayers = 6;
		BeginTurn((currentPlayer + 1) % numPlayers);
		return;
	}

	// clear the last players undo stack
	ClearUndoStack();

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
	pMap->CenterView(players[player].cursorX, players[player].cursorY);
}

void Game::EndTurn()
{
	// begin the next players turn
	int numPlayers = 6;
	BeginTurn((currentPlayer + 1) % numPlayers);
}

void Game::BeginBattle(Group *pGroup, MapTile *pTarget)
{
	// enter the battle
	pMapScreen->DeselectGroup();
	pBattle->Begin(pGroup, pTarget);
}

void Game::EndBattle(Group *pGroup, MapTile *pTarget)
{
	pBattle->End();

	// destroy dead units
	for(int a=0; a<pGroup->GetNumUnits(); ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		if(pUnit->IsDead())
		{
			pGroup->RemoveUnit(pUnit);
			if(!pUnit->IsHero())
				pUnit->Destroy();
			--a;
		}
	}

	// if all units in the group were destroyed
	MapTile *pCurrentTile = pGroup->GetTile();
	if(pGroup->GetNumUnits() == 0)
	{
		pCurrentTile->RemoveGroup(pGroup);
		pGroup->Destroy();
		pGroup = NULL;
	}

	// remove units from all groups on the target tile
	for(int a=0; a<pTarget->GetNumGroups();)
	{
		Group *pG = pTarget->GetGroup(a);
		for(int b=0; b<pG->GetNumUnits(); ++b)
		{
			Unit *pUnit = pG->GetUnit(b);
			if(pUnit->IsDead() || pUnit->GetPlayer() == -1)
			{
				pG->RemoveUnit(pUnit);
				if(!pUnit->IsHero())
					pUnit->Destroy();
				--b;
			}
		}

		if(pG->GetNumUnits() == 0)
		{
			pTarget->RemoveGroup(pG);
			pG->Destroy();
		}
		else
			++a;
	}

	if(pGroup)
	{
		// select the victorious group
		pMapScreen->SelectGroup(pGroup);

		// if the target is an empty castle, capture it
		Castle *pCastle = pTarget->GetCastle();
		if(pCastle && pCastle->IsEmpty())
			pCastle->Capture(pGroup->GetPlayer());

		// check if the target is clear, and move there
		if(pTarget->CanMove(pGroup))
		{
			pCurrentTile->RemoveGroup(pGroup);
			pTarget->AddGroup(pGroup);
			pMap->ClaimFlags(pTarget->GetX(), pTarget->GetY(), pGroup->GetPlayer());
		}

		pGroup->pPath->Destroy();
		pGroup->pPath = NULL;
	}

	Screen::SetNext(pMapScreen);
}

Group *Game::CreateUnit(int unit, Castle *pCastle)
{
	// find space in the castle for the unit
	MapTile *pTile = NULL;

	int x = pCastle->details.x;
	int y = pCastle->details.y;

	UnitDetails *pDetails = pUnitDefs->GetUnitDetails(unit);
	uint32 castleTerrain = pMap->GetTerrainAt(x, y);

	if(pUnitDefs->GetMovementPenalty(pDetails->movementClass, castleTerrain & 0xFF) == 0)
	{
		// the unit can't go on the castle, it must be a boat or something
		// find a patch of terrain around the castle where it can begin
		const int searchTable[12] = { 4, 8, 7, 11, 13, 14, 1, 2, 12, 15, 0, 3 };

		int width, height;
		pMap->GetMapSize(&width, &height);

		for(int i = 0; i < 12; ++i)
		{
			int tx = x - 1 + (searchTable[i] & 3);
			int ty = y - 1 + (searchTable[i] >> 2);

			if(tx < 0 || ty < 0 || tx >= width || ty >= height)
				continue;

			MapTile *pT = pMap->GetTile(tx, ty);

			if(pT->IsEnemyTile(pCastle->player))
				continue;

			uint32 terrain = pT->GetTerrain();
			for(int j=0; j<4; ++j)
			{
				if(pUnitDefs->GetMovementPenalty(pDetails->movementClass, terrain & 0xFF) != 0)
				{
					if((pDetails->type == UT_Vehicle && pT->GetNumGroups() < 10) || pT->GetNumUnits() < 10)
					{
						if(!pTile || pT->GetType() == OT_Road)
							pTile = pT;
						break;
					}
				}
				terrain >>= 8;
			}
		}
	}
	else
	{
		for(int i = 0; i < 4; ++i)
		{
			MapTile *pT = pMap->GetTile(x + (i & 1), y + (i >> 1));
			int numUnits = pT->GetNumUnits();
			if(numUnits < 10)
			{
				pTile = pT;
				break;
			}
		}
	}

	if(pTile)
	{
		// create the unit
		Unit *pUnit;
		if(players[pCastle->player].pHero && pDetails->type == UT_Hero)
		{
			pUnit = players[pCastle->player].pHero;
			pUnit->Revive();

			pCastle->building = -1;
		}
		else
		{
			pUnit = pUnitDefs->CreateUnit(unit, pCastle->player);
		}

		// create a group for the unit, and add it to the tile
		Group *pGroup = Group::Create(pCastle->player);
		pGroup->AddUnit(pUnit);
		pTile->AddGroup(pGroup);

		return pGroup;
	}

	return NULL;
}

void Game::PushGroupPosition(Group *pGroup)
{
	// if the undo stack is full
	if(undoDepth == MaxUndo)
	{
		for(int a=1; a<MaxUndo; ++a)
			undoStack[a-1] = undoStack[a];

		--undoDepth;
	}

	UndoStack &s = undoStack[undoDepth++];

	// push group
	s.pGroup = pGroup;

	// push position
	MapTile *pTile = pGroup->GetTile();
	s.x = pTile->GetX();
	s.y = pTile->GetY();

	// push unit movements
	if(pGroup->pVehicle)
	{
		// push vehicle movement
		s.vehicleMove = pGroup->pVehicle->GetMovement();
	}
	else
	{
		for(int a=0; a<pGroup->GetNumUnits(); ++a)
		{
			// push unit movement
			Unit *pUnit = pGroup->GetUnit(a);
			s.unitMove[a] = pUnit->GetMovement();
		}
	}
}

Group *Game::UndoLastMove()
{
	if(!undoDepth)
		return NULL;

	UndoStack &s = undoStack[--undoDepth];

	// revert to old tile
	MapTile *pTile = s.pGroup->GetTile();
	pTile->RemoveGroup(s.pGroup);
	MapTile *pOldTile = pMap->GetTile(s.x, s.y);
	pOldTile->AddGroup(s.pGroup);

	// restore movement
	if(s.pGroup->pVehicle)
	{
		// restore vehicle movement
		s.pGroup->pVehicle->SetMovement(s.vehicleMove);
	}
	else
	{
		for(int a=0; a<s.pGroup->GetNumUnits(); ++a)
		{
			// restore unit movement
			Unit *pUnit = s.pGroup->GetUnit(a);
			pUnit->SetMovement(s.unitMove[a]);
		}
	}

	// set the groups path target to the position we undid
	s.pGroup->FindPath(pTile->GetX(), pTile->GetY());

	return s.pGroup;
}

void Game::ClearUndoStack()
{
	undoDepth = 0;
}

void Game::DrawWindow(const MFRect &rect)
{
	MFMaterial_SetMaterial(pWindow);
	MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height);
}

void Game::DrawLine(float sx, float sy, float dx, float dy)
{
	MFMaterial_SetMaterial(pHorizLine);

	float texelCenter = MFRenderer_GetTexelCenterOffset() / 16.f;

	if(sx == dx)
	{
		MFPrimitive(PT_TriStrip | PT_Prelit);
		MFBegin(4);

		MFSetTexCoord1(0, 1.f+texelCenter);
		MFSetPosition(sx-8.f, sy, 0);

		MFSetTexCoord1(0, texelCenter);
		MFSetPosition(sx+8.f, sy, 0);

		MFSetTexCoord1(1, 1.f+texelCenter);
		MFSetPosition(sx-8.f, dy, 0);

		MFSetTexCoord1(1, texelCenter);
		MFSetPosition(sx+8.f, dy, 0);

		MFEnd();
	}
	else if(sy == dy)
		MFPrimitive_DrawQuad(sx, sy-8.f, dx-sx, 16.f, MFVector::one, 0.f, texelCenter, 1.f, 1.f + (texelCenter));
}

void Game::ShowRequest(const char *pMessage, RequestBox::SelectCallback *pCallback, bool bNotification, void *pUserData)
{
	pRequestBox->Show(pMessage, pCallback, bNotification, pUserData);
}

bool Game::DrawRequest()
{
	return pRequestBox->Draw();
}
