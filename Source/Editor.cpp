#include "Warlords.h"
#include "Editor.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

#include "Path.h"

Editor::Editor(Game *pGame)
{
	pMap = pGame->GetMap();

	if(!pMap)
		pMap = Map::CreateNew("TileSet", "Castles");

	brushType[0] = OT_Terrain;
	brushType[1] = OT_Terrain;
	brushIndex[0] = 0;
	brushIndex[1] = 1;
	brush = 1;

	bIsPainting = false;
	bPaintMode = true;
	pMap->SetMoveKey(bPaintMode);

	tileChooser = 0;
	numPages = 0;

	pIcons = MFMaterial_Create("Icons");

	// buttons
	Tileset *pTiles = pMap->GetTileset();
	UnitDefinitions *pUnits = pMap->GetUnitDefinitions();

	MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, ShowMiniMap, this, 0, true);

	pos.x = 16.f;
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.25f + (1.f/256.f);
	pModeButton = Button::Create(pIcons, &pos, &uvs, ChangeMode, this, 0, true);

	// brush buttons
	for(int a=0; a<2; ++a)
	{
		int tile = 0;
		pTiles->FindBestTiles(&tile, EncodeTile(brushIndex[a], brushIndex[a], brushIndex[a], brushIndex[a]), 0xFFFFFFFF, 1);
		pTiles->GetTileUVs(tile, &uvs);

		pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
		pos.y = (float)(gDefaults.display.displayHeight - (16 + tileHeight)*(2-a));

		pBrushButton[a] = Button::Create(pTileMat, &pos, &uvs, BrushSelect, this, a, true);
		pBrushButton[a]->SetOutline(true, brush == a ? MFVector::blue : MFVector::white);
	}

	// terrain selector
	const int numRows[16]    = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
	const int numColumns[16] = { 0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5 };
	const int numButtons = 12; // only have 12 buttons for now

	int rows = numRows[numButtons];
	int columns = numColumns[numButtons];

	float width = (float)(tileWidth*columns + 16*(columns-1));
	float height = (float)(tileHeight*rows + 16*(rows-1));
	float left = gDefaults.display.displayWidth*0.5f - width*0.5f;
	float top = gDefaults.display.displayHeight*0.5f - height*0.5f;

	// terrain page
	int terrainCount = pTiles->GetNumTerrainTypes();
	pageButtonCount[numPages] = terrainCount;

	for(int a=0; a<terrainCount; ++a)
	{
		int tile = 0;
		pTiles->FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);
		pTiles->GetTileUVs(tile, &uvs);

		pos.x = left + (a % columns)*(tileWidth+16);
		pos.y = top + (a / columns)*(tileHeight+16);

		pChooserButtons[numPages][a] = Button::Create(pTileMat, &pos, &uvs, ChooseBrush, this, (OT_Terrain << 16) | a, true);
		pChooserButtons[numPages][a]->SetOutline(true, MFVector::black);
	}

	++numPages;

	// castle buttons
	int raceCount = pUnits->GetNumRaces();
	pageButtonCount[numPages] = raceCount + 2;

	for(int a=0; a<raceCount; ++a)
	{
		pUnits->GetCastleUVs(a, &uvs);

		pos.x = left + (a % columns)*(tileWidth+16);
		pos.y = top + (a / columns)*(tileHeight+16);

		pChooserButtons[numPages][a] = Button::Create(pCastleMat, &pos, &uvs, ChooseBrush, this, (OT_Castle << 16) | a, true);
		pChooserButtons[numPages][a]->SetOutline(true, MFVector::black);
	}

	// add the merc flag
	pUnits->GetFlagUVs(0, &uvs);

	pos.x = left + (raceCount % columns)*(tileWidth+16);
	pos.y = top + (raceCount / columns)*(tileHeight+16);

	pChooserButtons[numPages][raceCount] = Button::Create(pCastleMat, &pos, &uvs, ChooseBrush, this, (OT_Flag << 16) | 0, true);
	pChooserButtons[numPages][raceCount]->SetOutline(true, MFVector::black);

	// add the road
	pTiles->GetRoadUVs(0, &uvs);
	++raceCount;

	pos.x = left + (raceCount % columns)*(tileWidth+16);
	pos.y = top + (raceCount / columns)*(tileHeight+16);

	pChooserButtons[numPages][raceCount] = Button::Create(pRoadMat, &pos, &uvs, ChooseBrush, this, (OT_Road << 16) | 0, true);
	pChooserButtons[numPages][raceCount]->SetOutline(true, MFVector::black);

	++numPages;

	// special buttons
	int specialCount = pUnits->GetNumSpecials();
	int specialIndex = 0;
	while(specialIndex < specialCount)
	{
		pageButtonCount[numPages] = MFMin(specialCount - specialIndex, 11);

		for(int a=specialIndex; a<specialIndex + pageButtonCount[numPages]; ++a)
		{
			pUnits->GetSpecialUVs(a, &uvs);

			pos.x = left + (a % columns)*(tileWidth+16);
			pos.y = top + (a / columns)*(tileHeight+16);

			pChooserButtons[numPages][a] = Button::Create(pCastleMat, &pos, &uvs, ChooseBrush, this, (OT_Special << 16) | a, true);
			pChooserButtons[numPages][a]->SetOutline(true, MFVector::black);
		}

		specialIndex += pageButtonCount[numPages];
		++numPages;
	}

	// page flip button
	pos.x = left + (11 % columns)*(tileWidth+16);
	pos.y = top + (11 / columns)*(tileHeight+16);
	uvs.x = 0.0f + (1.f/256.f); uvs.y = 0.0f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pFlipButton = Button::Create(pIcons, &pos, &uvs, FlipPage, this, 0, true);

	terrainSelectWindowWidth = width + 32.f;
	terrainSelectWindowHeight = height + 32.f;
}

Editor::~Editor()
{
	pMap->Destroy();
}

void Editor::Select()
{

}

int Editor::UpdateInput()
{
	if(tileChooser)
	{
		Tileset *pTiles = pMap->GetTileset();
		int chooserPage = tileChooser - 1;

		for(int a=0; a<pageButtonCount[chooserPage]; ++a)
			pChooserButtons[chooserPage][a]->UpdateInput();

		pFlipButton->UpdateInput();
	}
	else
	{
		// update buttons
		if(pBrushButton[0]->UpdateInput() ||
			pBrushButton[1]->UpdateInput())
			return 1;

		// update zoom

		// update minimap
		if(pMiniMap->UpdateInput())
			return 1;
		if(pModeButton->UpdateInput())
			return 1;

		// update map
		pMap->UpdateInput();

		if(MFInput_WasPressed(Key_S, IDD_Keyboard))
			pMap->Save("Map.ini");

		// handle tile placement
		if(bPaintMode)
		{
			int cursorX, cursorY;
			pMap->GetCursor(&cursorX, &cursorY);

			bool bWasPressed = MFInput_WasPressed(Mouse_LeftButton, IDD_Mouse);
			ObjectType detail = pMap->GetDetailType(cursorX, cursorY);

			if(bWasPressed)
			{
				if(brushType[brush] == OT_Road)
					bRemoveRoad = (detail == OT_Road);

				switch(brushType[brush])
				{
					case OT_Castle:
					{
						if(detail == OT_Castle)
						{
							//if(castle selected)
							//  castle properties
							//else
							//	select castle
						}
						else
							pMap->PlaceCastle(cursorX, cursorY, brushIndex[brush]);
						break;
					}
					case OT_Flag:
					{
						if(detail == OT_Flag)
							pMap->ClearDetail(cursorX, cursorY);
						else
							pMap->PlaceFlag(cursorX, cursorY, brushIndex[brush]);
						break;
					}
					case OT_Special:
					{
						if(detail == OT_Special)
							pMap->ClearDetail(cursorX, cursorY);
						else
							pMap->PlaceSpecial(cursorX, cursorY, brushIndex[brush]);
						break;
					}
				}
			}

			if(MFInput_Read(Mouse_LeftButton, IDD_Mouse))
			{
				switch(brushType[brush])
				{
					case OT_None:
					{
						pMap->ClearDetail(cursorX, cursorY);
						break;
					}
					case OT_Terrain:
					{
						int terrain = brushIndex[brush];
						pMap->SetTerrain(cursorX, cursorY, terrain, terrain, terrain, terrain);
						break;
					}
					case OT_Road:
					{
						if(bRemoveRoad)
						{
							if(pMap->GetDetailType(cursorX, cursorY) == OT_Road)
								pMap->ClearDetail(cursorX, cursorY);
						}
						else
						{
							pMap->PlaceRoad(cursorX, cursorY);
						}
						break;
					}
				}
			}
		}
	}

	return 0;
}

int Editor::Update()
{
	pMap->Update();

	return 0;
}

void Editor::Draw()
{
	pMap->Draw();

	// draw debug
	static bool bDrawDebug = false;
	if(MFInput_WasPressed(Key_F1, IDD_Keyboard))
		bDrawDebug = !bDrawDebug;

	if(bDrawDebug)
		pMap->DrawDebug();

	pMiniMap->Draw();
	pModeButton->Draw();

	pBrushButton[0]->Draw();
	pBrushButton[1]->Draw();

	if(tileChooser)
	{
		// render background
		float x = gDefaults.display.displayWidth*0.5f - terrainSelectWindowWidth*0.5f;
		float y = gDefaults.display.displayHeight*0.5f - terrainSelectWindowHeight*0.5f;
		MFPrimitive_DrawUntexturedQuad(x, y, terrainSelectWindowWidth, terrainSelectWindowHeight, MakeVector(0,0,0,.8f));

//		Tileset *pTiles = pMap->GetTileset();
//		MFFont_DrawTextAnchored(NULL, const char *pText, const MFVector &pos, MFFontJustify justification, float lineWidth, float textHeight, const MFVector &color);

		// render the buttons
		Tileset *pTiles = pMap->GetTileset();
		int chooserPage = tileChooser - 1;

		for(int a=0; a<pageButtonCount[chooserPage]; ++a)
			pChooserButtons[chooserPage][a]->Draw();

		pFlipButton->Draw();
	}
}

void Editor::Deselect()
{

}

void Editor::BrushSelect(int button, void *pUserData, int buttonID)
{
	Editor *pThis = (Editor*)pUserData;

	if(pThis->brush == buttonID)
	{
		// show brush selector window...
		pThis->tileChooser = 1;
	}
	else
	{
		pThis->pBrushButton[0]->SetOutline(true, buttonID == 0 ? MFVector::blue : MFVector::white);
		pThis->pBrushButton[1]->SetOutline(true, buttonID == 1 ? MFVector::blue : MFVector::white);
		pThis->brush = buttonID;
	}
}

void Editor::ChooseBrush(int button, void *pUserData, int buttonID)
{
	Editor *pThis = (Editor*)pUserData;

	ObjectType type = (ObjectType)(buttonID >> 16);
	int index = buttonID & 0xFFFF;

	pThis->brushType[pThis->brush] = type;
	pThis->brushIndex[pThis->brush] = index;

	MFMaterial *pMat = NULL;
	MFRect rect = { 0, 0, 1, 1 };

	switch(type)
	{
		case OT_Terrain:
		{
			Tileset *pTiles = pThis->pMap->GetTileset();

			int tile = 0;
			pTiles->FindBestTiles(&tile, EncodeTile(buttonID, buttonID, buttonID, buttonID), 0xFFFFFFFF, 1);

			pTiles->GetTileUVs(tile, &rect);
			pMat = pTiles->GetTileMaterial();
			break;
		}
		case OT_Castle:
		{
    	UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetCastleUVs(index, &rect);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
    	UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetFlagUVs(index, &rect);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Special:
		{
    	UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetSpecialUVs(index, &rect);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Road:
		{
			Tileset *pTiles = pThis->pMap->GetTileset();
			pTiles->GetRoadUVs(index, &rect);
			pMat = pTiles->GetRoadMaterial();
			break;
		}
	}

	// update the brush image
	pThis->pBrushButton[pThis->brush]->SetImage(pMat, &rect);

	pThis->tileChooser = 0;
}

void Editor::FlipPage(int button, void *pUserData, int buttonID)
{
	Editor *pThis = (Editor*)pUserData;

	pThis->tileChooser = (pThis->tileChooser % pThis->numPages) + 1;
}

void Editor::ShowMiniMap(int button, void *pUserData, int buttonID)
{

}

void Editor::ChangeMode(int button, void *pUserData, int buttonID)
{
	Editor *pThis = (Editor*)pUserData;

	pThis->bPaintMode = !pThis->bPaintMode;
	pThis->pMap->SetMoveKey(pThis->bPaintMode);

	MFRect uvs;
	uvs.x = (pThis->bPaintMode ? 0.75f : 0.5f) + (1.f/256.f); uvs.y = 0.25f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pThis->pModeButton->SetImage(pThis->pIcons, &uvs);
}
