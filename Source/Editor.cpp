#include "Warlords.h"
#include "Editor.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

Editor::Editor(const char *pFilename)
{
	pMap = NULL;

	if(pFilename)
		pMap = Map::Create(pFilename);
	if(!pMap)
		pMap = Map::CreateNew("TileSet", "Castles");

	brushType[0] = OT_Terrain;
	brushType[1] = OT_Terrain;
	brushIndex[0] = 0;
	brushIndex[1] = 1;
	brush = 1;

	bIsPainting = false;

	tileChooser = 0;
	numPages = 0;

	pIcons = MFMaterial_Create("Icons");

	// buttons
	int tileWidth, tileHeight;
	Tileset *pTiles = pMap->GetTileset();
	MFMaterial *pTileMat = pTiles->GetMaterial();
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	CastleSet *pCastles = pMap->GetCastleSet();
	MFMaterial *pCastleMat = pCastles->GetCastleMaterial();
	MFMaterial *pRoadMat = pCastles->GetRoadMaterial();

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, ShowMiniMap, this, 0, true);

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
	int raceCount = pCastles->GetNumRaces();
	pageButtonCount[numPages] = raceCount + 2;

	for(int a=0; a<raceCount; ++a)
	{
		pCastles->GetCastleUVs(a, &uvs);

		pos.x = left + (a % columns)*(tileWidth+16);
		pos.y = top + (a / columns)*(tileHeight+16);

		pChooserButtons[numPages][a] = Button::Create(pCastleMat, &pos, &uvs, ChooseBrush, this, (OT_Castle << 16) | a, true);
		pChooserButtons[numPages][a]->SetOutline(true, MFVector::black);
	}

	// add the merc flag
	pCastles->GetFlagUVs(0, &uvs);

	pos.x = left + (raceCount % columns)*(tileWidth+16);
	pos.y = top + (raceCount / columns)*(tileHeight+16);

	pChooserButtons[numPages][raceCount] = Button::Create(pCastleMat, &pos, &uvs, ChooseBrush, this, (OT_Flag << 16) | 0, true);
	pChooserButtons[numPages][raceCount]->SetOutline(true, MFVector::black);

	// add the road
	pCastles->GetRoadUVs(0, &uvs);
	++raceCount;

	pos.x = left + (raceCount % columns)*(tileWidth+16);
	pos.y = top + (raceCount / columns)*(tileHeight+16);

	pChooserButtons[numPages][raceCount] = Button::Create(pRoadMat, &pos, &uvs, ChooseBrush, this, (OT_Road << 16) | 0, true);
	pChooserButtons[numPages][raceCount]->SetOutline(true, MFVector::black);

	++numPages;

	// special buttons
	int specialCount = pCastles->GetNumSpecials();
	int specialIndex = 0;
	while(specialIndex < specialCount)
	{
		pageButtonCount[numPages] = MFMin(specialCount - specialIndex, 11);

		for(int a=specialIndex; a<specialIndex + pageButtonCount[numPages]; ++a)
		{
			pCastles->GetSpecialUVs(a, &uvs);

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
		CastleSet *pCastles = pMap->GetCastleSet();
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

		// update map
		pMap->UpdateInput();

		// handle tile placement
		int cursorX, cursorY;
		pMap->GetCursor(&cursorX, &cursorY);

		static int change = 0;
		if(MFInput_Read(Mouse_LeftButton, IDD_Mouse))
		{
			switch(brushType[brush])
			{
				case OT_Terrain:
				{
					int terrain = brushIndex[brush];
					pMap->SetTerrain(cursorX, cursorY, terrain, terrain, terrain, terrain);
					change = 0;
				}
				case OT_Castle:
				{
					// place a castle
					break;
				}
				case OT_Flag:
				{
					break;
				}
				case OT_Special:
				{
					break;
				}
				case OT_Road:
				{
					// place a road, and connect neighbouring roads to it...
					break;
				}
			}
		}

		if(MFInput_WasPressed(Key_Space, IDD_Keyboard))
			change = pMap->UpdateChange(change);
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
		CastleSet *pCastles = pMap->GetCastleSet();
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
			pMat = pTiles->GetMaterial();
			break;
		}
		case OT_Castle:
		{
			CastleSet *pCastles = pThis->pMap->GetCastleSet();
			pCastles->GetCastleUVs(index, &rect);
			pMat = pCastles->GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
			CastleSet *pCastles = pThis->pMap->GetCastleSet();
			pCastles->GetFlagUVs(index, &rect);
			pMat = pCastles->GetCastleMaterial();
			break;
		}
		case OT_Special:
		{
			CastleSet *pCastles = pThis->pMap->GetCastleSet();
			pCastles->GetSpecialUVs(index, &rect);
			pMat = pCastles->GetCastleMaterial();
			break;
		}
		case OT_Road:
		{
			CastleSet *pCastles = pThis->pMap->GetCastleSet();
			pCastles->GetRoadUVs(index, &rect);
			pMat = pCastles->GetRoadMaterial();
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
