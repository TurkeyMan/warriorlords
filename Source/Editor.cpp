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
	bOwnsMap = false;

	if(!pMap)
	{
		pMap = Map::CreateNew(pGame, "TileSet", "Castles");
		bOwnsMap = true;
	}

	brushType[0] = OT_Terrain;
	brushType[1] = OT_Terrain;
	brushIndex[0] = 0;
	brushIndex[1] = 1;
	brush = 1;
	brushSize = 2;

	editRace = 0;
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
	uvs.x = 0.75f + (.5f/256.f); uvs.y = 0.f + (.5f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, true);
	pMiniMap->SetClickCallback(MakeDelegate(this, &Editor::ShowMiniMap));

	pos.x = 16.f;
	pos.y = 16.f;
	uvs.x = 0.75f + (.5f/256.f); uvs.y = 0.25f + (.5f/256.f);
	pModeButton = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, true);
	pModeButton->SetClickCallback(MakeDelegate(this, &Editor::ChangeMode));

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

			pBrushButton[a] = Button::Create(pTileMat, &pos, &uvs, MFVector::one, a, true);
			pBrushButton[a]->SetClickCallback(MakeDelegate(this, &Editor::BrushSelect));
		}
		else
		{
			pTiles->GetWaterUVs(&uvs, texelOffset);

			pBrushButton[a] = Button::Create(pWater, &pos, &uvs, MFVector::one, a, true);
			pBrushButton[a]->SetClickCallback(MakeDelegate(this, &Editor::BrushSelect));
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

			brushSelect.AddButton(0, pWater, &uvs, MFVector::one, (OT_Terrain << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
		}
		else
		{
			int tile = 0;
			pTiles->FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);

			pTiles->GetTileUVs(tile, &uvs, texelOffset);

			brushSelect.AddButton(0, pTileMat, &uvs, MFVector::one, (OT_Terrain << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
		}
	}

	// castle buttons
	int raceCount = pUnits->GetNumRaces();

	for(int a=0; a<raceCount; ++a)
	{
		int race = (a + 1) % raceCount;
		int player = race - 1;

		pUnits->GetCastleUVs(race, &uvs, texelOffset);

		brushSelect.AddButton(1, pCastleMat, &uvs, pGame->GetPlayerColour(player), (OT_Castle << 16) | (uint16)player, MakeDelegate(this, &Editor::ChooseBrush));
	}

	// add the merc flag
	pUnits->GetFlagUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(1, pCastleMat, &uvs, pGame->GetPlayerColour(-1), OT_Flag << 16, MakeDelegate(this, &Editor::ChooseBrush));

	// add the road
	pTiles->GetRoadUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(1, pRoadMat, &uvs, MFVector::one, OT_Road << 16, MakeDelegate(this, &Editor::ChooseBrush));

	// special buttons
	int specialCount = pUnits->GetNumSpecials();
	int numPages = 3 + (specialCount-1)/11;

	for(int a=0; a<specialCount; ++a)
	{
		pUnits->GetSpecialUVs(a, &uvs, texelOffset);

		brushSelect.AddButton(2 + a/11, pCastleMat, &uvs, MFVector::one, (OT_Special << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
	}

	// region buttons
	for(int a=0; a<8; ++a)
	{
		pUnits->GetFlagUVs(a+1, &uvs, texelOffset);

		brushSelect.AddButton(numPages, pCastleMat, &uvs, pGame->GetPlayerColour(a), (OT_Region << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
	}

	pUnits->GetFlagUVs(0, &uvs, texelOffset);
	brushSelect.AddButton(numPages, pCastleMat, &uvs, pGame->GetPlayerColour(-1), (OT_Region << 16) | 15, MakeDelegate(this, &Editor::ChooseBrush));
}

Editor::~Editor()
{
	if(bOwnsMap)
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
		int cursorX, cursorY;
		float  cx, cy;
		pMap->GetCursor(info.hover.x, info.hover.y, &cx, &cy);
		cursorX = brushType[brush] == OT_Terrain ? (int)(cx * 2.f) : (int)cx;
		cursorY = brushType[brush] == OT_Terrain ? (int)(cy * 2.f) : (int)cy;

		switch(ev)
		{
			case IE_Down:
			{
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
							if(brushSize == 1)
							{
								int terrain = brushIndex[brush];

								uint32 t = pMap->GetTerrainAt(cursorX >> 1, cursorY >> 1);
								int tl = DecodeTL(t);
								int tr = DecodeTR(t);
								int bl = DecodeBL(t);
								int br = DecodeBR(t);
								uint32 mask;

								if(cursorX & 1)
								{
									if(cursorY & 1)
									{
										br = terrain;
										mask = 0xFF000000;
									}
									else
									{
										tr = terrain;
										mask = 0xFF0000;
									}
								}
								else
								{
									if(cursorY & 1)
									{
										bl = terrain;
										mask = 0xFF00;
									}
									else
									{
										tl = terrain;
										mask = 0xFF;
									}
								}

								pMap->SetTerrain(cursorX >> 1, cursorY >> 1, tl, tr, bl, br, mask);
							}
							else
							{
								int terrain = brushIndex[brush];
								pMap->SetTerrain(cursorX >> 1, cursorY >> 1, terrain, terrain, terrain, terrain);
							}
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
		pMap->Save();

	bool bChangeRace = MFInput_Read(Key_LShift, IDD_Keyboard) || MFInput_Read(Key_RShift, IDD_Keyboard);

	if(MFInput_WasPressed(Key_Grave, IDD_Keyboard))
	{
		if(bChangeRace)
		{
			editRace = 0;
			pMap->ConstructMap(editRace);
			pMap->SetEditRace(editRace);
		}
		else
		{
			editRegion = 15;
			pMap->SetEditRegion(editRegion);
		}
	}

	for(int a=0; a<8; ++a)
	{
		if(MFInput_WasPressed(Key_1 + a, IDD_Keyboard))
		{
			if(bChangeRace)
			{
				editRace = a + 1;
				pMap->ConstructMap(editRace);
				pMap->SetEditRace(editRace);
			}
			else
			{
				editRegion = a;
				pMap->SetEditRegion(editRegion);
			}
		}
	}

	return 0;
}

void Editor::Draw()
{
	pMap->Draw();

	// draw debug
	static bool bDrawDebug = false;
	if(MFInput_WasPressed(Key_F1, IDD_Keyboard))
		bDrawDebug = !bDrawDebug;
	if(MFInput_WasPressed(Key_F2, IDD_Keyboard))
		brushSize = brushSize == 1 ? 2 : 1;

	if(bDrawDebug)
		pMap->DrawDebug();

	pMiniMap->Draw();
	pModeButton->Draw();

	pBrushButton[0]->Draw();
	pBrushButton[1]->Draw();

	brushSelect.Draw();
	castleEdit.Draw();

	const char *pText = "Race: Template";
	if(editRace > 0)
		pText = MFStr("Race: %s", pMap->GetUnitDefinitions()->GetRaceName(editRace));
	MFFont_DrawText(MFFont_GetDebugFont(), 87.f, 10.f, 32.f, MFVector::black, pText);
	MFFont_DrawText(MFFont_GetDebugFont(), 85.f, 8.f, 32.f, MFVector::white, pText);

	pText = "Region: X";
	if(editRegion != 15)
		pText = MFStr("Region: %d", editRegion + 1);
	MFFont_DrawText(MFFont_GetDebugFont(), 87.f, 47.f, 32.f, MFVector::black, pText);
	MFFont_DrawText(MFFont_GetDebugFont(), 85.f, 45.f, 32.f, MFVector::white, pText);
}

void Editor::Deselect()
{

}

void Editor::BrushSelect(int button, int buttonID)
{
	if(brush == buttonID)
	{
		// show brush selector window...
		brushSelect.Show();
	}
	else
	{
		pBrushButton[0]->SetOutline(true, buttonID == 0 ? MFVector::blue : MFVector::white);
		pBrushButton[1]->SetOutline(true, buttonID == 1 ? MFVector::blue : MFVector::white);
		brush = buttonID;
	}
}

void Editor::ChooseBrush(int button, int buttonID)
{
	ObjectType type = (ObjectType)(buttonID >> 16);
	int index = (int16)(buttonID & 0xFFFF);

	brushType[brush] = type;
	brushIndex[brush] = index;

	MFMaterial *pMat = NULL;
	MFRect rect = { 0, 0, 1, 1 };
	MFVector colour = MFVector::one;
	float texelOffset = MFRenderer_GetTexelCenterOffset();

	switch(type)
	{
		case OT_Terrain:
		{
			Tileset *pTiles = pMap->GetTileset();

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
			UnitDefinitions *pUnits = pMap->GetUnitDefinitions();
			pUnits->GetCastleUVs(index + 1, &rect, texelOffset);
			colour = Game::GetCurrent()->GetPlayerColour(index);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
			UnitDefinitions *pUnits = pMap->GetUnitDefinitions();
			pUnits->GetFlagUVs(index - 1, &rect, texelOffset);
			pMat = pUnits->GetCastleMaterial();
			--brushIndex[brush];
			break;
		}
		case OT_Special:
		{
			UnitDefinitions *pUnits = pMap->GetUnitDefinitions();
			pUnits->GetSpecialUVs(index, &rect, texelOffset);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
		case OT_Road:
		{
			Tileset *pTiles = pMap->GetTileset();
			pTiles->GetRoadUVs(index, &rect, texelOffset);
			pMat = pTiles->GetRoadMaterial();
			break;
		}
		case OT_Region:
		{
			UnitDefinitions *pUnits = pMap->GetUnitDefinitions();
			if(index == 15)
				index = -1;
			pUnits->GetFlagUVs(index + 1, &rect, texelOffset);
			colour = Game::GetCurrent()->GetPlayerColour(index);
			pMat = pUnits->GetCastleMaterial();
			break;
		}
	}

	// update the brush image
	pBrushButton[brush]->SetImage(pMat, &rect, colour);

	brushSelect.Hide();
}

void Editor::ShowMiniMap(int button, int buttonID)
{

}

void Editor::ChangeMode(int button, int buttonID)
{
	bPaintMode = !bPaintMode;
	pMap->SetMoveKey(bPaintMode);

	MFRect uvs;
	uvs.x = (bPaintMode ? 0.75f : 0.5f) + (1.f/256.f); uvs.y = 0.25f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pModeButton->SetImage(pIcons, &uvs);
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

	pFlipButton = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, true);
	pFlipButton->SetClickCallback(MakeDelegate(this, &Chooser::FlipPage));
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

Button *Chooser::AddButton(int page, MFMaterial *pImage, MFRect *pUVs, const MFVector &colour, int buttonID, Button::ClickCallback callback)
{
	MFRect rect = { 0.f, 0.f, 64.f, 64.f };
	Button *pButton = Button::Create(pImage, &rect, pUVs, colour, buttonID, true);
	pButton->SetClickCallback(callback);
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

void Chooser::FlipPage(int button, int buttonID)
{
	// pop the last page buttons
	pInputManager->PopReceiver(this);

	// update the page
	currentPage = (currentPage + 1) % numPages;

	AssembleButtons();

	// push the new page buttons
	pInputManager->PushReceiver(this);

	for(int a=0; a<numButtons[currentPage]; ++a)
		pInputManager->PushReceiver(pButtons[currentPage][a]);

	if(numPages > 1)
		pInputManager->PushReceiver(pFlipButton);
}

CastleEdit::CastleEdit()
{
	pFont = Game::GetCurrent()->GetTextFont();
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
	pBuildUnits[0] = Button::Create(NULL, &button, &uvs, MFVector::one, 0);
	pBuildUnits[0]->SetClickCallback(MakeDelegate(this, &CastleEdit::SelectUnit));
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + 5.f;
	pBuildUnits[1] = Button::Create(NULL, &button, &uvs, MFVector::one, 1);
	pBuildUnits[1]->SetClickCallback(MakeDelegate(this, &CastleEdit::SelectUnit));
	button.x = units.x + 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[2] = Button::Create(NULL, &button, &uvs, MFVector::one, 2);
	pBuildUnits[2]->SetClickCallback(MakeDelegate(this, &CastleEdit::SelectUnit));
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[3] = Button::Create(NULL, &button, &uvs, MFVector::one, 3);
	pBuildUnits[3]->SetClickCallback(MakeDelegate(this, &CastleEdit::SelectUnit));

	pName = StringBox::Create(pFont, &title);
	pName->SetChangeCallback(MakeDelegate(this, &CastleEdit::NameChangeCallback));

	// set up the unit picker
	MFRect rect = { 0.5f, 0, 0.25f, 0.25f };
	unitSelect.AddButton(0, pIcons, &rect, MFVector::one, -1, MakeDelegate(this, &CastleEdit::SetUnit));

	UnitDefinitions *pDefs = Game::GetCurrent()->GetUnitDefs();
	MFMaterial *pUnitMat = pDefs->GetUnitMaterial();
	int numUnits = pDefs->GetNumUnitTypes();

	int addedCount = 1;
	for(int a=8; a<numUnits; ++a)
	{
		pDefs->GetUnitUVs(a, false, &rect, texelOffset);
		Button *pButton = unitSelect.AddButton(addedCount / 11, pUnitMat, &rect, Game::GetCurrent()->GetPlayerColour(-1), a, MakeDelegate(this, &CastleEdit::SetUnit));
		++addedCount;
	}
}

CastleEdit::~CastleEdit()
{
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

void CastleEdit::Show(Castle *_pCastle)
{
	pCastle = _pCastle;

	Map *pMap = Game::GetCurrent()->GetMap();
	pTemplate = pMap->GetCastleTemplate(pCastle->details.x, pCastle->details.y);

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
			pBuildUnits[a]->SetOutline(true, pCastle->nextBuild == a ? MFVector::blue : MFVector::white);
		}
		else
		{
			uvs.x = 0.5f; uvs.y = 0.f;
			uvs.width = uvs.height = 0.25f;
			pBuildUnits[a]->SetImage(pIcons, &uvs);
			pBuildUnits[a]->SetOutline(true, pCastle->nextBuild == a ? MFVector::blue : MFVector::white);
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

	// update the castle template
	pTemplate->numBuildUnits = pCastle->details.numBuildUnits;
	for(int a=0; a<pTemplate->numBuildUnits; ++a)
	{
		pTemplate->buildUnits[a] = pCastle->details.buildUnits[a];

		UnitDetails *pDetails = pCastle->pUnitDefs->GetUnitDetails(pTemplate->buildUnits[a].unit);
		pTemplate->buildUnits[a].buildTime -= pDetails->buildTime;
		pTemplate->buildUnits[a].cost -= pDetails->cost;
	}
}

void CastleEdit::SelectUnit(int button, int buttonID)
{
	if(pCastle->nextBuild != buttonID)
	{
		for(int a=0; a<pCastle->details.numBuildUnits; ++a)
			pBuildUnits[a]->SetOutline(true, buttonID == a ? MFVector::blue : MFVector::white);

		pCastle->SetBuildUnit(buttonID);
	}
	else
	{
		// show chooser
		unitSelect.Show();
		bHide = true;
	}
}

void CastleEdit::SetUnit(int button, int buttonID)
{
	UnitDefinitions *pDefs = Game::GetCurrent()->GetUnitDefs();

	int selected = pCastle->nextBuild;

	// set the new unit
	BuildUnit &unit = pCastle->details.buildUnits[selected];
	UnitDetails *pDetails = pDefs->GetUnitDetails(buttonID);
	unit.unit = buttonID;
	unit.cost = buttonID >= 0 ? pDetails->cost : 0;
	unit.buildTime = buttonID >= 0 ? pDetails->buildTime : 0;

	pCastle->buildTime = buttonID >= 0 ? unit.buildTime : 0;

	// update the button image
	if(buttonID != -1)
	{
		MFRect rect;
		MFMaterial *pUnitMat = pDefs->GetUnitMaterial();
		pDefs->GetUnitUVs(buttonID, false, &rect, MFRenderer_GetTexelCenterOffset());

		pBuildUnits[selected]->SetImage(pUnitMat, &rect, Game::GetCurrent()->GetPlayerColour(pCastle->player));
	}
	else
	{
		MFRect rect = { 0.5f, 0, 0.25f, 0.25f };
		pBuildUnits[selected]->SetImage(pIcons, &rect);
	}

	unitSelect.Hide();
	bHide = false;
}

void CastleEdit::NameChangeCallback(const char *pString)
{
	pCastle->SetName(pString);
	MFString_Copy(pTemplate->name, pString);
}
