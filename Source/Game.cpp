#include "Warlords.h"
#include "Game.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

Game *Game::pCurrent = NULL;

Game::Game(const char *_pMap, bool bEditable)
{
	pMap = NULL;
	pUnitDefs = NULL;

	pMapScreen = NULL;
	pBattle = NULL;

	if(bEditable)
	{
		// create the map and screens
		pMap = Map::Create(this, "Map", bEditable);
		pUnitDefs = pMap->GetUnitDefinitions();

		pMapScreen = new MapScreen(this);
		pBattle = new Battle(this);

		currentPlayer = 0;
		currentTurn = 0;

		for(int a=0; a<8; ++a)
		{
			players[a].race = a + 1;
			players[a].colour = pUnitDefs->GetRaceColour(a + 1);
		}
	}
}

Game::~Game()
{
	if(pBattle)
		delete pBattle;
	if(pMapScreen)
		delete pMapScreen;

	if(pMap)
		pMap->Destroy();
}

void Game::BeginGame()
{
	const int numRaces = 2;

	// setup players
	for(int a=0; a<8; ++a)
	{
		players[a].race = 1 + (MFRand() % numRaces);
		players[a].gold = 0;
		players[a].cursorX = 0;
		players[a].cursorY = 0;
	}

	// create the map and screens
	pMap = Map::Create(this, "Map", false);
	pUnitDefs = pMap->GetUnitDefinitions();

	pMapScreen = new MapScreen(this);
	pBattle = new Battle(this);

	currentPlayer = 0;
	currentTurn = 0;

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
			Unit *pHero = pUnitDefs->CreateUnit(players[pCastle->player].race - 1, pCastle->player);

			Group *pGroup = Group::Create(pCastle->player);
			pGroup->AddUnit(pHero);
			MapTile *pTile = pMap->GetTile(pCastle->details.x, pCastle->details.y);
			pTile->AddGroup(pGroup);
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
				// find space in the castle for the unit
				MapTile *pTile = NULL;

				int x = pCastle->details.x;
				int y = pCastle->details.y;

				int buildUnit = pCastle->details.buildUnits[pCastle->building].unit;
				UnitDetails *pDetails = pUnitDefs->GetUnitDetails(buildUnit);
				uint32 castleTerrain = pMap->GetTerrainAt(x, y);

				if(pUnitDefs->GetMovementPenalty(pDetails->movementClass, castleTerrain & 0xFF) == 0)
				{
					// the unit can't go on the castle, it must be a boat or something
					// find a patch of terrain around the castle where it can begin
					const int searchTable[12] = { 4, 8, 7, 11, 13, 14, 1, 2, 10, 15, 0, 3 };

					int width, height;
					pMap->GetMapSize(&width, &height);

					for(int i = 0; i < 12; ++i)
					{
						int tx = x - 1 + (searchTable[i] & 3);
						int ty = y - 1 + (searchTable[i] >> 2);

						if(tx < 0 || ty < 0 || tx >= width || ty >= height)
							continue;

						MapTile *pT = pMap->GetTile(tx, ty);

						if(pT->IsEnemyTile(player))
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
					Unit *pUnit = pUnitDefs->CreateUnit(buildUnit, currentPlayer);
					pCastle->SetBuildUnit(pCastle->building);

					// create a group for the unit, and add it to the tile
					Group *pGroup = Group::Create(player);
					pGroup->AddUnit(pUnit);
					pTile->AddGroup(pGroup);
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
	pBattle->Begin(pGroup, pTarget, 0, 2, -1);
}

void Game::EndBattle(Group *pGroup, MapTile *pTarget)
{
	// destroy dead units
	for(int a=0; a<pGroup->GetNumUnits(); ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		if(pUnit->IsDead())
		{
			pGroup->RemoveUnit(pUnit);
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
			pMap->DestroyPath(pGroup->pPath);
			pGroup->pPath = NULL;
		}
	}

	Screen::SetNext(pMapScreen);
}
