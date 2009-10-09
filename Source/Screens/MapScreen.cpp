#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

#include "Path.h"

MapScreen::MapScreen(GameData *_pGameData)
{
  pGameData = _pGameData;

	pIcons = MFMaterial_Create("Icons");

	// buttons
	Tileset *pTiles = pGameData->GetMap()->GetTileset();
  UnitDefinitions *pUnits = pGameData->GetUnitDefs();

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

MapScreen::~MapScreen()
{
}

void MapScreen::Select()
{
}

int MapScreen::UpdateInput()
{
	// update zoom

	// update minimap
	if(pMiniMap->UpdateInput())
		return 1;

	// update map
	pGameData->GetMap()->UpdateInput();

	return 0;
}

int MapScreen::Update()
{
	pGameData->GetMap()->Update();
	return 0;
}

void MapScreen::Draw()
{
	pGameData->GetMap()->Draw();
	pMiniMap->Draw();
}

void MapScreen::Deselect()
{

}

void MapScreen::ShowMiniMap(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;

}