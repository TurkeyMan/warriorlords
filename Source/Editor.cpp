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
		pMap = Map::CreateNew("TileSet");

	brushType[0] = 0;
	brushType[1] = 1;
	brush = 1;

	bIsPainting = false;

	tileChooser = 0;

	pIcons = MFMaterial_Create("Icons");

	// buttons
	int tileWidth, tileHeight;
	Tileset *pTiles = pMap->GetTileset();
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
	pos.y = 16.f;
	uvs.x = 0.75f; uvs.y = 0.f;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, ShowMiniMap, this, 0, true);

	// brush buttons
	MFMaterial *pTileMat = pTiles->GetMaterial();


	for(int a=0; a<2; ++a)
	{
		int tile = 0;
		pTiles->FindBestTiles(&tile, EncodeTile(brushType[a], brushType[a], brushType[a], brushType[a]), 0xFFFFFFFF, 1);
		pTiles->GetTileUVs(tile, &uvs);

		pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
		pos.y = (float)(gDefaults.display.displayHeight - (16 + tileHeight)*(2-a));

		pBrushButton[a] = Button::Create(pTileMat, &pos, &uvs, BrushSelect, this, a, true);
		pBrushButton[a]->SetOutline(true, brush == a ? MFVector::blue : MFVector::white);
	}

	// terrain selector
	static const int numRows[16]    = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
	static const int numColumns[16] = { 0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5 };

	int terrainCount = pTiles->GetNumTerrainTypes();

	int rows = numRows[terrainCount];
	int columns = numColumns[terrainCount];

	float width = (float)(tileWidth*columns + 16*(columns-1));
	float height = (float)(tileHeight*rows + 16*(rows-1));
	float left = gDefaults.display.displayWidth*0.5f - width*0.5f;
	float top = gDefaults.display.displayHeight*0.5f - height*0.5f;

	for(int a=0; a<terrainCount; ++a)
	{
		int tile = 0;
		pTiles->FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);
		pTiles->GetTileUVs(tile, &uvs);

		pos.x = left + (a % columns)*(tileWidth+16);
		pos.y = top + (a / columns)*(tileHeight+16);

		pChooserButtons[a] = Button::Create(pTileMat, &pos, &uvs, ChooseBrush, this, a, true);
		pChooserButtons[a]->SetOutline(true, MFVector::black);
	}

	// page flip button
	pos.x = left + (11 % columns)*(tileWidth+16);
	pos.y = top + (11 / columns)*(tileHeight+16);
	uvs.x = 0.0f; uvs.y = 0.0f;
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
		int terrainCount = pTiles->GetNumTerrainTypes();

		for(int a=0; a<terrainCount; ++a)
			pChooserButtons[a]->UpdateInput();

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
			int type = brushType[brush];
			pMap->SetTerrain(cursorX, cursorY, type, type, type, type);
			change = 0;
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
		int terrainCount = pTiles->GetNumTerrainTypes();

		for(int a=0; a<terrainCount; ++a)
			pChooserButtons[a]->Draw();

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

	pThis->brushType[pThis->brush] = buttonID;

	int tile = 0;
	Tileset *pTiles = pThis->pMap->GetTileset();
	pTiles->FindBestTiles(&tile, EncodeTile(buttonID, buttonID, buttonID, buttonID), 0xFFFFFFFF, 1);

	MFRect rect;
	pTiles->GetTileUVs(tile, &rect);
	pThis->pBrushButton[pThis->brush]->SetImage(pTiles->GetMaterial(), &rect);

	pThis->tileChooser = 0;
}

void Editor::FlipPage(int button, void *pUserData, int buttonID)
{

}

void Editor::ShowMiniMap(int button, void *pUserData, int buttonID)
{

}
