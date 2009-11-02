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
	pMap = Map::Create("Map");
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

	// set each players map focus on starting castle

	// produce starting hero (or flag that hero wants to join on the first turn)

	BeginTurn(0);

	Screen::SetNext(pMapScreen);
}

void Game::BeginTurn(int player)
{
	currentPlayer = player;

	// heal units

	// get money

	// add new units

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
}
