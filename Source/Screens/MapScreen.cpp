#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"
#include "Display.h"

#include "Menu/Game/GameUI.h"

#include "Fuji/MFSystem.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFView.h"
#include "Fuji/MFRenderer.h"

MapScreen::MapScreen(Game *_pGame)
{
	pGame = _pGame;

	// buttons
	const Tileset &tileSet = pGame->Map().Tileset();
	UnitDefinitions *pUnits = pGame->Map().UnitDefs();

	MFMaterial *pTileMat = tileSet.GetTileMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();
	MFMaterial *pRoadMat = tileSet.GetRoadMaterial();
}

MapScreen::~MapScreen()
{
}

void MapScreen::Select()
{
	pInputManager->PushReceiver(this);

	pCurrent = this;
}

bool MapScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	return false;
}

int MapScreen::Update()
{
	pGame->Update();

#if defined(_DEBUG)
	Map &map = pGame->Map();

	int w, h;
	map.GetMapSize(&w, &h);
	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			MapTile *pTile = map.GetTile(x, y);

			int numGroups = pTile->GetNumGroups();
			for(int g=0; g<numGroups; ++g)
				Group *pGroup = pTile->GetGroup(g);
		}
	}
#endif

	return 0;
}

void MapScreen::Draw()
{
	pGame->Draw();
}

void MapScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}
