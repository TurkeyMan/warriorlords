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
		pMap = Map::CreateNew(pGame, "TileSet", "Castles");

	brushType[0] = OT_Terrain;
	brushType[1] = OT_Terrain;
	brushIndex[0] = 0;
	brushIndex[1] = 1;
	brush = 1;

	bPaintMode = true;
	pMap->SetMoveKey(bPaintMode);

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

	// init terrain selector:
	// terrain page
	int terrainCount = pTiles->GetNumTerrainTypes();

	for(int a=0; a<terrainCount; ++a)
	{
		int tile = 0;
		pTiles->FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);

		pTiles->GetTileUVs(tile, &uvs);

		brushSelect.AddButton(0, &uvs, pTileMat, (OT_Terrain << 16) | a, ChooseBrush, this, true);
	}

	// castle buttons
	int raceCount = pUnits->GetNumRaces();

	for(int a=0; a<raceCount; ++a)
	{
		int race = (a + 1) % raceCount;
		int player = race - 1;

		pUnits->GetCastleUVs(race, &uvs);

		brushSelect.AddButton(1, &uvs, pCastleMat, (OT_Castle << 16) | (uint16)player, ChooseBrush, this, true);
	}

	// add the merc flag
	pUnits->GetFlagUVs(0, &uvs);
	brushSelect.AddButton(1, &uvs, pCastleMat, OT_Flag << 16, ChooseBrush, this, true);

	// add the road
	pTiles->GetRoadUVs(0, &uvs);
	brushSelect.AddButton(1, &uvs, pRoadMat, OT_Road << 16, ChooseBrush, this, true);

	// special buttons
	int specialCount = pUnits->GetNumSpecials();
	for(int a=0; a<specialCount; ++a)
	{
		pUnits->GetSpecialUVs(a, &uvs);

		brushSelect.AddButton(2 + a/11, &uvs, pCastleMat, (OT_Special << 16) | a, ChooseBrush, this, true);
	}

	// init unit selector:
	int numUnits = pUnits->GetNumUnitTypes();
	for(int a=0; a<numUnits; ++a)
	{
		
	}
}

Editor::~Editor()
{
	pMap->Destroy();
}

void Editor::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pMap);

	pInputManager->PushReceiver(pBrushButton[0]);
	pInputManager->PushReceiver(pBrushButton[1]);

	pInputManager->PushReceiver(pMiniMap);
	pInputManager->PushReceiver(pModeButton);
}

bool Editor::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	if(bPaintMode)
	{
		switch(ev)
		{
			case IE_Down:
			{
				int cursorX, cursorY;
				pMap->GetCursor(info.down.x, info.down.y, &cursorX, &cursorY);

				ObjectType detail = pMap->GetDetailType(cursorX, cursorY);

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
			case IE_Drag:
			{
				int cursorX, cursorY;
				pMap->GetCursor(info.drag.x, info.drag.y, &cursorX, &cursorY);

				if(cursorX != lastX || cursorY != lastY)
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

					lastX = cursorX;
					lastY = cursorY;
				}

				break;
			}

			case IE_Up:
				lastX = lastY = -1;
				break;
		}
	}

	return false;
}

int Editor::UpdateInput()
{
	if(MFInput_WasPressed(Key_S, IDD_Keyboard))
		pMap->Save("Map.ini");

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

	brushSelect.Draw();
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
		pThis->brushSelect.Show();
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
	int index = (int16)(buttonID & 0xFFFF);

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
			pUnits->GetCastleUVs(index + 1, &rect);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
			UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetFlagUVs(index - 1, &rect);
			pMat = pUnits->GetCastleMaterial();
			--pThis->brushIndex[pThis->brush];
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

	pThis->brushSelect.Hide();
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

Chooser::Chooser()
{
	MFZeroMemory(numButtons, sizeof(numButtons));
	numPages = 0;
	bVisible = false;

	pIcons = MFMaterial_Create("Icons");

	// page flip button
	MFRect pos = { 0.f, 0.f, 64.f, 64.f };
	MFRect uvs = { 1.f/256.f, 1.f/256.f, 0.25f, 0.25f};

	pFlipButton = Button::Create(pIcons, &pos, &uvs, FlipPage, this, 0, true);
}

Chooser::~Chooser()
{
	for(int p=0; p<numPages; ++p)
	{
		for(int b=0; b<numButtons[p]; ++b)
			pButtons[p][b]->Destroy();
	}

	pFlipButton->Destroy();

	MFMaterial_Destroy(pIcons);
}

Button *Chooser::AddButton(int page, MFRect *pUVs, MFMaterial *pImage, int buttonID, Button::TriggerCallback *pCallback, void *pUserData, bool bTriggerOnDown)
{
	MFRect rect = { 0.f, 0.f, 64.f, 64.f };
	Button *pButton = Button::Create(pImage, &rect, pUVs, pCallback, pUserData, buttonID, bTriggerOnDown);
	pButton->SetOutline(true, MFVector::black);

	pButtons[page][numButtons[page]++] = pButton;
	numPages = MFMax(numPages, page+1);

	return pButton;
}

void Chooser::Show()
{
	currentPage = 0;
	bVisible = true;

	AssembleButtons();

	pInputManager->PushReceiver(this);

	for(int a=0; a<numButtons[currentPage]; ++a)
		pInputManager->PushReceiver(pButtons[currentPage][a]);

	if(numPages > 1)
		pInputManager->PushReceiver(pFlipButton);
}

void Chooser::Hide()
{
	pInputManager->PopReceiver(this);
	bVisible = false;
}

void Chooser::Draw()
{
	if(bVisible)
	{
		// render background
		float x = gDefaults.display.displayWidth*0.5f - windowWidth*0.5f;
		float y = gDefaults.display.displayHeight*0.5f - windowHeight*0.5f;
		MFPrimitive_DrawUntexturedQuad(x, y, windowWidth, windowHeight, MakeVector(0,0,0,.8f));

		// render the buttons
		for(int a=0; a<numButtons[currentPage]; ++a)
			pButtons[currentPage][a]->Draw();

		if(numPages > 1)
			pFlipButton->Draw();
	}
}

bool Chooser::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.buttonID == 0)
		return true;

	return false;
}

void Chooser::AssembleButtons()
{
	const int numRows[16]    = { 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 3 };
	const int numColumns[16] = { 0, 1, 2, 2, 2, 3, 3, 3, 3, 3, 4, 4, 4, 5, 5, 5 };
	const float tileWidth = 64.f, tileHeight = 64.f;

	int buttons = (numPages == 1 ? numButtons[currentPage] : 12);
	int rows = numRows[buttons];
	int columns = numColumns[buttons];

	float width = (float)(tileWidth*columns + 16*(columns-1));
	float height = (float)(tileHeight*rows + 16*(rows-1));
	float left = gDefaults.display.displayWidth*0.5f - width*0.5f;
	float top = gDefaults.display.displayHeight*0.5f - height*0.5f;

	windowWidth = width + 32.f;
	windowHeight = height + 32.f;

	for(int a=0; a<numButtons[currentPage]; ++a)
	{
		MFRect pos;
		pos.x = left + (a % columns)*(tileWidth+16);
		pos.y = top + (a / columns)*(tileHeight+16);
		pos.width = pos.height = tileWidth;

		pButtons[currentPage][a]->SetPos(&pos);
	}

	if(numPages > 1)
	{
		MFRect pos;
		pos.x = left + (11 % columns)*(tileWidth+16);
		pos.y = top + (11 / columns)*(tileHeight+16);
		pos.width = pos.height = tileWidth;

		pFlipButton->SetPos(&pos);
	}
}

void Chooser::FlipPage(int button, void *pUserData, int buttonID)
{
	Chooser *pThis = (Chooser*)pUserData;

	// pop the last page buttons
	pInputManager->PopReceiver(pThis);

	// update the page
	pThis->currentPage = (pThis->currentPage + 1) % pThis->numPages;

	pThis->AssembleButtons();

	// push the new page buttons
	pInputManager->PushReceiver(pThis);

	for(int a=0; a<pThis->numButtons[pThis->currentPage]; ++a)
		pInputManager->PushReceiver(pThis->pButtons[pThis->currentPage][a]);

	if(pThis->numPages > 1)
		pInputManager->PushReceiver(pThis->pFlipButton);
}
