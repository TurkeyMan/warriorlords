#include "Warlords.h"
#include "Game.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

#include "Path.h"

Game::Game(const char *pFilename)
{
	pMap = Map::Create(pFilename);

	pIcons = MFMaterial_Create("Icons");

	// buttons
	Tileset *pTiles = pMap->GetTileset();
  UnitDefinitions *pUnits = pMap->GetUnitDefinitions();

  MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, ShowMiniMap, this, 0, true);
}

Game::~Game()
{
	pMap->Destroy();
}

void Game::Select()
{

}

int Game::UpdateInput()
{
	// update zoom

	// update minimap
	if(pMiniMap->UpdateInput())
		return 1;

	// update map
	pMap->UpdateInput();

	return 0;
}

int Game::Update()
{
	pMap->Update();

	return 0;
}

void Game::Draw()
{
	pMap->Draw();

	pMiniMap->Draw();
}

void Game::Deselect()
{

}

void Game::ShowMiniMap(int button, void *pUserData, int buttonID)
{
	Game *pThis = (Game*)pUserData;

}
