#include "Warlords.h"
#include "Editor.h"
#include "Display.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFRenderer.h"

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

	editRegion = 15;

	bPaintMode = true;
	pMap->SetMoveKey(bPaintMode);

	pIcons = MFMaterial_Create("Icons");

	// buttons
	Tileset *pTiles = pMap->GetTileset();
	UnitDefinitions *pUnits = pMap->GetUnitDefinitions();

	MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pWater = pTiles->GetWaterMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);
	float texelOffset = MFRenderer_GetTexelCenterOffset();

	MFRect display, uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };
	MFRect water = { 0, 0, 1, 1 };

	GetDisplayRect(&display);

	pos.x = (float)(display.width - (16 + tileWidth));
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, MFVector::one, ShowMiniMap, this, 0, true);

	pos.x = 16.f;
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.25f + (1.f/256.f);
	pModeButton = Button::Create(pIcons, &pos, &uvs, MFVector::one, ChangeMode, this, 0, true);

	// brush buttons
	for(int a=0; a<2; ++a)
	{
		pos.x = (float)(display.width - (16 + tileWidth));
		pos.y = (float)(display.height - (16 + tileHeight)*(2-a));

		if(a == 0)
		{
			int tile = 0;
			pTiles->FindBestTiles(&tile, EncodeTile(brushIndex[a], brushIndex[a], brushIndex[a], brushIndex[a]), 0xFFFFFFFF, 1);
			pTiles->GetTileUVs(tile, &uvs, texelOffset);

			pBrushButton[a] = Button::Create(pTileMat, &pos, &uvs, MFVector::one, BrushSelect, this, a, true);
		}
		else
		{
			pTiles->GetWaterUVs(&uvs, texelOffset);

			pBrushButton[a] = Button::Create(pWater, &pos, &uvs, MFVector::one, BrushSelect, this, a, true);
		}

		pBrushButton[a]->SetOutline(true, brush == a ? MFVector::blue : MFVector::white);
	}

	// init terrain selector:
	// terrain page
	int terrainCount = pTiles->GetNumTerrainTypes();

	for(int a=0; a<terrainCount; ++a)
	{
		if(a == 1)
		{
			pTiles->GetWaterUVs(&uvs, texelOffset);

			brushSelect.AddButton(0, pWater, &uvs, MFVector::one, (OT_Terrain << 16) | a, ChooseBrush, this);
		}
		else
		{
			int tile = 0;
			pTiles->FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);

			pTiles->GetTileUVs(tile, &uvs, texelOffset);

			brushSelect.AddButton(0, pTileMat, &uvs, MFVector::one, (OT_Terrain << 16) | a, ChooseBrush, this);
		}
	}

	// castle buttons
	int raceCount = pUnits->GetNumRaces();

	for(int a=0; a<raceCount; ++a)
	{
		int race = (a + 1) % raceCount;
		int player = race - 1;

		pUnits->GetCastleUVs(race, &uvs, texelOffset);

		brushSelect.AddButton(1, pCastleMat, &uvs, pGame->GetPlayerColour(player), (OT_Castle << 16) | (uint16)player, ChooseBrush, this);
	}

	// add the merc flag
	pUnits->GetFlagUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(1, pCastleMat, &uvs, pGame->GetPlayerColour(-1), OT_Flag << 16, ChooseBrush, this);

	// add the road
	pTiles->GetRoadUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(1, pRoadMat, &uvs, MFVector::one, OT_Road << 16, ChooseBrush, this);

	// special buttons
	int specialCount = pUnits->GetNumSpecials();
	int numPages = 3 + (specialCount-1)/11;

	for(int a=0; a<specialCount; ++a)
	{
		pUnits->GetSpecialUVs(a, &uvs, texelOffset);

		brushSelect.AddButton(2 + a/11, pCastleMat, &uvs, MFVector::one, (OT_Special << 16) | a, ChooseBrush, this);
	}

	// region buttons
	for(int a=0; a<8; ++a)
	{
		pUnits->GetFlagUVs(a+1, &uvs, texelOffset);

		brushSelect.AddButton(numPages, pCastleMat, &uvs, pGame->GetPlayerColour(a), (OT_Region << 16) | a, ChooseBrush, this);
	}

	pUnits->GetFlagUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(numPages, pCastleMat, &uvs, pGame->GetPlayerColour(-1), (OT_Region << 16) | 15, ChooseBrush, this);
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
							castleEdit.Show(pMap->GetTile(cursorX, cursorY)->GetCastle());
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
						case OT_Region:
						{
							pMap->SetRegion(cursorX, cursorY, brushIndex[brush]);
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

int Editor::Update()
{
	pMap->Update();

	castleEdit.Update();

	if(MFInput_WasPressed(Key_S, IDD_Keyboard) && MFInput_Read(Key_LControl, IDD_Keyboard))
		pMap->Save("Map.ini");

	if(MFInput_WasPressed(Key_Grave, IDD_Keyboard))
		editRegion = 15;

	for(int a=0; a<8; ++a)
	{
		if(MFInput_WasPressed(Key_1 + a, IDD_Keyboard))
			editRegion = a;
	}

	pMap->SetEditRegion(editRegion);

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
	castleEdit.Draw();

	const char *pText = "Edit Region: X";
	if(editRegion != 15)
		pText = MFStr("Edit Region: %d", editRegion + 1);
	MFFont_DrawText(MFFont_GetDebugFont(), 87.f, 27.f, 32.f, MFVector::black, pText);
	MFFont_DrawText(MFFont_GetDebugFont(), 85.f, 25.f, 32.f, MFVector::white, pText);
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
	MFVector colour = MFVector::one;
	float texelOffset = MFRenderer_GetTexelCenterOffset();

	switch(type)
	{
		case OT_Terrain:
		{
			Tileset *pTiles = pThis->pMap->GetTileset();

			if(index == 1)
			{
				pTiles->GetWaterUVs(&rect, texelOffset);
				pMat = pTiles->GetWaterMaterial();
			}
			else
			{
				int tile = 0;
				pTiles->FindBestTiles(&tile, EncodeTile(index, index, index, index), 0xFFFFFFFF, 1);

				pTiles->GetTileUVs(tile, &rect, texelOffset);
				pMat = pTiles->GetTileMaterial();
			}
			break;
		}
		case OT_Castle:
		{
			UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetCastleUVs(index + 1, &rect, texelOffset);
			colour = Game::GetCurrent()->GetPlayerColour(index);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
			UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetFlagUVs(index - 1, &rect, texelOffset);
			pMat = pUnits->GetCastleMaterial();
			--pThis->brushIndex[pThis->brush];
			break;
		}
		case OT_Special:
		{
			UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			pUnits->GetSpecialUVs(index, &rect, texelOffset);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Road:
		{
			Tileset *pTiles = pThis->pMap->GetTileset();
			pTiles->GetRoadUVs(index, &rect, texelOffset);
			pMat = pTiles->GetRoadMaterial();
			break;
		}
		case OT_Region:
		{
			UnitDefinitions *pUnits = pThis->pMap->GetUnitDefinitions();
			if(index == 15)
				index = -1;
			pUnits->GetFlagUVs(index + 1, &rect, texelOffset);
			colour = Game::GetCurrent()->GetPlayerColour(index);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
	}

	// update the brush image
	pThis->pBrushButton[pThis->brush]->SetImage(pMat, &rect, colour);

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

	pFlipButton = Button::Create(pIcons, &pos, &uvs, MFVector::one, FlipPage, this, 0, true);
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

Button *Chooser::AddButton(int page, MFMaterial *pImage, MFRect *pUVs, const MFVector &colour, int buttonID, Button::TriggerCallback *pCallback, void *pUserData)
{
	MFRect rect = { 0.f, 0.f, 64.f, 64.f };
	Button *pButton = Button::Create(pImage, &rect, pUVs, colour, pCallback, pUserData, buttonID, true);
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
		MFRect display;
		GetDisplayRect(&display);

		// render background
		float x = display.width*0.5f - windowWidth*0.5f;
		float y = display.height*0.5f - windowHeight*0.5f;
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

	MFRect display;
	GetDisplayRect(&display);

	float width = (float)(tileWidth*columns + 16*(columns-1));
	float height = (float)(tileHeight*rows + 16*(rows-1));
	float left = display.width*0.5f - width*0.5f;
	float top = display.height*0.5f - height*0.5f;

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

CastleEdit::CastleEdit()
{
	pFont = MFFont_Create("Charlemagne");
	pIcons = MFMaterial_Create("Icons");

	bVisible = false;
	bHide = false;

	float texelOffset = MFRenderer_GetTexelCenterOffset();

	float margin = 5.f;
	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;
	AdjustRect_Margin(&window, margin*2.f);

	MFRect body, upperBody;
	DivideRect_Vert(window, MFFont_GetFontHeight(pFont) + margin*2, 0.f, &title, &body, true);
	DivideRect_Vert(body, 64*2 + margin*6, 0.f, &upperBody, &lower, true);
	DivideRect_Horiz(upperBody, 64*2 + margin*6, 0.f, &units, &right, true);
	AdjustRect_Margin(&title, margin);
	AdjustRect_Margin(&units, margin);
	AdjustRect_Margin(&right, margin);
	AdjustRect_Margin(&lower, margin);

	MFRect button, uvs = { 0, 0, 1, 1 };
	button.width = 64.f; button.height = 64.f;

	button.x = units.x + 5.f; button.y = units.y + 5.f;
	pBuildUnits[0] = Button::Create(NULL, &button, &uvs, MFVector::one, SelectUnit, this, 0);
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + 5.f;
	pBuildUnits[1] = Button::Create(NULL, &button, &uvs, MFVector::one, SelectUnit, this, 1);
	button.x = units.x + 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[2] = Button::Create(NULL, &button, &uvs, MFVector::one, SelectUnit, this, 2);
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[3] = Button::Create(NULL, &button, &uvs, MFVector::one, SelectUnit, this, 3);

	pName = StringBox::Create(pFont, &title, ChangeCallback, this);

	// set up the unit picker
	MFRect rect = { 0.5f, 0, 0.25f, 0.25f };
	unitSelect.AddButton(0, pIcons, &rect, MFVector::one, -1, SetUnit, this);

	UnitDefinitions *pDefs = Game::GetCurrent()->GetUnitDefs();
	MFMaterial *pUnitMat = pDefs->GetUnitMaterial();
	int numUnits = pDefs->GetNumUnitTypes();

	int addedCount = 1;
	for(int a=8; a<numUnits; ++a)
	{
		pDefs->GetUnitUVs(a, false, &rect, texelOffset);
		Button *pButton = unitSelect.AddButton(addedCount / 11, pUnitMat, &rect, Game::GetCurrent()->GetPlayerColour(-1), a, SetUnit, this);
		++addedCount;
	}
}

CastleEdit::~CastleEdit()
{
	MFFont_Destroy(pFont);
}

void CastleEdit::Update()
{
	pName->Update();
}

void CastleEdit::Draw()
{
	if(!bVisible)
		return;

	unitSelect.Draw();

	if(bHide)
		return;

	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0, 0, 0, 0.8f));

	MFPrimitive_DrawUntexturedQuad(title.x, title.y, title.width, title.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(units.x, units.y, units.width, units.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(right.x, right.y, right.width, right.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(lower.x, lower.y, lower.width, lower.height, MakeVector(0, 0, 0, .8f));

	pName->Draw();

	int building = pCastle->GetBuildUnit();
	if(building > -1)
	{
		UnitDefinitions *pUnitDefs = Game::GetCurrent()->GetUnitDefs();
		UnitDetails *pDetails = pUnitDefs->GetUnitDetails(building);

		int height = (int)MFFont_GetFontHeight(pFont);
		MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 5, MFVector::white, pDetails->pName);
		if(pDetails->type == UT_Vehicle)
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*2, MFVector::white, "Turns: %d", pCastle->buildTime);
		}
		else
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height, MFVector::white, "Type: %s", pUnitDefs->GetArmourClassName(pDetails->defenceClass));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*2, MFVector::white, "Atk: %d - %d %s", pDetails->attackMin, pDetails->attackMax, pUnitDefs->GetWeaponClassName(pDetails->attackClass));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*3, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*4, MFVector::white, "Turns: %d", pCastle->buildTime);
		}
	}

	for(int a=0; a<pCastle->details.numBuildUnits; ++a)
		pBuildUnits[a]->Draw();
}

bool CastleEdit::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			pName->Enable(false);
			break;
	}

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	return true;
}

void CastleEdit::Show(Castle *pCastle)
{
	this->pCastle = pCastle;
	bVisible = true;

	pInputManager->PushReceiver(this);

	UnitDefinitions *pUnitDefs = pCastle->pUnitDefs;
	Game *pGame = pUnitDefs->GetGame();

	MFMaterial *pUnitMat = pUnitDefs->GetUnitMaterial();
	float texelOffset = MFRenderer_GetTexelCenterOffset();

	for(int b=pCastle->details.numBuildUnits; b<4; ++b)
		pCastle->details.buildUnits[b].unit = -1;

	pCastle->details.numBuildUnits = 4;

	for(int a=0; a<4; ++a)
	{
		MFRect uvs;
		int unit = pCastle->details.buildUnits[a].unit;

		if(unit != -1)
		{
			pUnitDefs->GetUnitUVs(pCastle->details.buildUnits[a].unit, false, &uvs, texelOffset);
			pBuildUnits[a]->SetImage(pUnitMat, &uvs, pGame->GetPlayerColour(pCastle->player));
			pBuildUnits[a]->SetOutline(true, pCastle->building == a ? MFVector::blue : MFVector::white);
		}
		else
		{
			uvs.x = 0.5f; uvs.y = 0.f;
			uvs.width = uvs.height = 0.25f;
			pBuildUnits[a]->SetImage(pIcons, &uvs);
			pBuildUnits[a]->SetOutline(true, pCastle->building == a ? MFVector::blue : MFVector::white);
		}

		pInputManager->PushReceiver(pBuildUnits[a]);
	}

	pName->SetString(pCastle->GetName());

	pInputManager->PushReceiver(pName);
}

void CastleEdit::Hide()
{
	pInputManager->PopReceiver(this);

	bVisible = false;

	// collate the build units into the lower slots
	pCastle->details.numBuildUnits = 0;
	for(int a=0; a<4; ++a)
	{
		if(pCastle->details.buildUnits[a].unit != -1)
			pCastle->details.buildUnits[pCastle->details.numBuildUnits++] = pCastle->details.buildUnits[a];
	}
}

void CastleEdit::SelectUnit(int button, void *pUserData, int buttonID)
{
	CastleEdit *pThis = (CastleEdit*)pUserData;
	Castle *pCastle = pThis->pCastle;

	if(pCastle->building != buttonID)
	{
		for(int a=0; a<pCastle->details.numBuildUnits; ++a)
			pThis->pBuildUnits[a]->SetOutline(true, buttonID == a ? MFVector::blue : MFVector::white);

		pCastle->SetBuildUnit(buttonID);
	}
	else
	{
		// show chooser
		pThis->unitSelect.Show();
		pThis->bHide = true;
	}
}

void CastleEdit::SetUnit(int button, void *pUserData, int buttonID)
{
	CastleEdit *pThis = (CastleEdit*)pUserData;
	UnitDefinitions *pDefs = Game::GetCurrent()->GetUnitDefs();
	Castle *pCastle = pThis->pCastle;

	int selected = pThis->pCastle->building;

	// set the new unit
	BuildUnit &unit = pThis->pCastle->details.buildUnits[selected];
	unit.unit = buttonID;
	unit.cost = pDefs->GetUnitDetails(buttonID)->cost;
	unit.buildTimeMod = 0;

	pThis->pCastle->buildTime = pDefs->GetUnitDetails(buttonID)->buildTime + unit.buildTimeMod;

	// update the button image
	if(buttonID != -1)
	{
		MFRect rect;
		MFMaterial *pUnitMat = pDefs->GetUnitMaterial();
		pDefs->GetUnitUVs(buttonID, false, &rect, MFRenderer_GetTexelCenterOffset());

		pThis->pBuildUnits[selected]->SetImage(pUnitMat, &rect, Game::GetCurrent()->GetPlayerColour(pCastle->player));
	}
	else
	{
		MFRect rect = { 0.5f, 0, 0.25f, 0.25f };
		pThis->pBuildUnits[selected]->SetImage(pThis->pIcons, &rect);
	}

	pThis->unitSelect.Hide();
	pThis->bHide = false;
}

void CastleEdit::ChangeCallback(const char *pString, void *pUserData)
{
	CastleEdit *pThis = (CastleEdit*)pUserData;
	Castle *pCastle = pThis->pCastle;

	pCastle->SetName(pString);
}
