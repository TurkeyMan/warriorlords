#include "Warlords.h"
#include "Game.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

Game::Game(const char *_pMap)
{
	pMap = Map::Create(this, "Map");
	pUnitDefs = pMap->GetUnitDefinitions();

	pMapScreen = new MapScreen(this);
	pBattle = new Battle(this);
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
	// set players
	for(int a=0; a<8; ++a)
	{
		players[a].race = a + 1;
		players[a].colour = pUnitDefs->GetRaceColour(players[a].race);
		players[a].gold = 0;

		players[a].cursorX = 0;
		players[a].cursorY = 0;
	}

	// HACK
	for(int a=0; a<pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player != -1)
		{
			pCastle->building = 24;
			pCastle->buildTime = 2;

			// set each players map focus on starting castle
			players[pCastle->player].cursorX = pCastle->details.x;
			players[pCastle->player].cursorY = pCastle->details.y;

			// produce starting hero (or flag that hero wants to join on the first turn)
			Unit *pHero = pUnitDefs->CreateUnit(pCastle->player, pCastle->player);
			
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
	currentPlayer = player;

	// heal units

	// get money + add new units
	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			players[currentPlayer].gold += pCastle->details.income;

			if(pCastle->building != -1 && --pCastle->buildTime == 0)
			{
				// find space in the castle for the unit
				MapTile *pTile = NULL;
				for(int i = 0; i < 4; ++i)
				{
					MapTile *pT = pMap->GetTile(pCastle->details.x + (i & 1), pCastle->details.y + (1 >> 1));
					int numUnits = pT->GetNumUnits();
					if(numUnits < 10)
					{
						pTile = pT;
						break;
					}
				}

				if(pTile)
				{
					// create the unit
					Unit *pUnit = pUnitDefs->CreateUnit(pCastle->building, currentPlayer);
					pCastle->buildTime += 1;// TODO: get unit build time

					// create a group for the unit, and add it to the tile
					Group *pGroup = Group::Create(player);
					pGroup->AddUnit(pUnit);
					pTile->AddGroup(pGroup);
				}
			}
		}
	}

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
	pMap->CenterView(players[player].cursorX, players[player].cursorY);
}

void Game::BeginBattle(Group *pGroup1, Group *pGroup2)
{
	// enter the battle
	pBattle->Begin(pGroup1, pGroup2, 0, 2, -1);
}

void Game::EndBattle(Group *pGroup1, Group *pGroup2)
{
	// destroy dead units

	// group1 is the artificial group, so we need to find the units in their real groups and remove them from there

	if(pGroup1)
	{
		// check if the target is clear, and move there
		//...

		// if the target is an empty castle, capture it
		//...
	}

	Screen::SetNext(pMapScreen);
}
