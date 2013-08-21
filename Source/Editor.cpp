#include "Warlords.h"
#include "Editor.h"
#include "Display.h"

#include "Fuji/MFInput.h"
#include "Fuji/MFSystem.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFFont.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFRenderer.h"

#include "Path.h"

Editor::Editor(MFString mapFilename)
: gameState(map.edit, map)
, Game(gameState)
, map(gameState, mapFilename)
, castleEdit(*this)
{
	brushType[0] = OT_Terrain;
	brushType[1] = OT_Terrain;
	brushIndex[0] = 0;
	brushIndex[1] = 1;
	brush = 1;
	brushSize = 2;

	editRace = 0;
	editRegion = 15;

	bPaintMode = true;
	mapView.SetMoveKey(bPaintMode);

	pIcons = MFMaterial_Create("Icons");

	// buttons
	const Tileset &tileset = map.Tileset();
	const UnitDefinitions &units = map.UnitDefs();

	MFMaterial *pTileMat[2];
	pTileMat[0] = tileset.GetTileMaterial(0);
	pTileMat[1] = tileset.GetTileMaterial(1);
	MFMaterial *pWater = tileset.GetWaterMaterial();
	MFMaterial *pRoadMat = tileset.GetRoadMaterial();
	MFMaterial *pCastleMat = units.GetCastleMaterial();
	MFMaterial *pMiscMat = units.GetMiscMaterial();

	TileSize tileSize = tileset.GetTileSize();
	float texelOffset = MFRenderer_GetTexelCenterOffset();

	MFRect display, uvs, pos = { 0, 0, (float)tileSize.width, (float)tileSize.height };
	MFRect water = { 0, 0, 1, 1 };

	GetDisplayRect(&display);

	pos.x = (float)(display.width - (16 + tileSize.width));
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
		pos.x = (float)(display.width - (16 + tileSize.width));
		pos.y = (float)(display.height - (16 + tileSize.height)*(2-a));

		if(a == 0)
		{
			int tile = 0;
			tileset.FindBestTiles(&tile, EncodeTile(brushIndex[a], brushIndex[a], brushIndex[a], brushIndex[a]), 0xFFFFFFFF, 1);

			uvs = tileset.GetTileUVs(tile, texelOffset);
			pBrushButton[a] = Button::Create(pTileMat[tile >> 8], &pos, &uvs, MFVector::one, a, true);
			pBrushButton[a]->SetClickCallback(MakeDelegate(this, &Editor::BrushSelect));
		}
		else
		{
			uvs = tileset.GetWaterUVs(texelOffset);
			pBrushButton[a] = Button::Create(pWater, &pos, &uvs, MFVector::one, a, true);
			pBrushButton[a]->SetClickCallback(MakeDelegate(this, &Editor::BrushSelect));
		}

		pBrushButton[a]->SetOutline(true, brush == a ? MFVector::blue : MFVector::white);
	}

	// init terrain selector:
	// terrain page
	int terrainCount = tileset.NumTerrainTypes();

	for(int a=0; a<terrainCount; ++a)
	{
		if(a == 1)
		{
			uvs = tileset.GetWaterUVs(texelOffset);
			brushSelect.AddButton(0, pWater, &uvs, MFVector::one, (OT_Terrain << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
		}
		else
		{
			int tile = 0;
			tileset.FindBestTiles(&tile, EncodeTile(a, a, a, a), 0xFFFFFFFF, 1);

			uvs = tileset.GetTileUVs(tile, texelOffset);
			brushSelect.AddButton(0, pTileMat[tile >> 8], &uvs, MFVector::one, (OT_Terrain << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
		}
	}

	// castle buttons
	int raceCount = units.GetNumRaces();

	for(int a=0; a<raceCount; ++a)
	{
		int race = (a + 1) % raceCount;
		int player = race - 1;

		uvs = units.GetCastleUVs(race, texelOffset);
		brushSelect.AddButton(1, pCastleMat, &uvs, GetPlayerColour(player), (OT_Castle << 16) | (uint16)player, MakeDelegate(this, &Editor::ChooseBrush));
	}

	// add the merc flag
	uvs = units.GetFlagUVs(0, texelOffset);
	brushSelect.AddButton(2, pCastleMat, &uvs, GetPlayerColour(-1), OT_Flag << 16, MakeDelegate(this, &Editor::ChooseBrush));

	// add the road
	uvs = tileset.GetRoadUVs(0, texelOffset);
	brushSelect.AddButton(2, pRoadMat, &uvs, MFVector::one, OT_Road << 16, MakeDelegate(this, &Editor::ChooseBrush));

	// special buttons
	int specialCount = units.GetNumSpecials();
	int numPages = 3 + (specialCount-1)/11;

	for(int a=0; a<specialCount; ++a)
	{
		uvs = units.GetSpecialUVs(a, texelOffset);
		brushSelect.AddButton(2 + a/11, pMiscMat, &uvs, MFVector::one, (OT_Place << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
	}

	// region buttons
	for(int a=0; a<10; ++a)
	{
		uvs = units.GetFlagUVs(a+1, texelOffset);
		brushSelect.AddButton(numPages, pCastleMat, &uvs, GetPlayerColour(a), (OT_Region << 16) | a, MakeDelegate(this, &Editor::ChooseBrush));
	}

	uvs = units.GetFlagUVs(0, texelOffset);
	brushSelect.AddButton(numPages, pCastleMat, &uvs, GetPlayerColour(-1), (OT_Region << 16) | 15, MakeDelegate(this, &Editor::ChooseBrush));
}

Editor::~Editor()
{
}

void Editor::Select()
{
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(&mapView);

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
		mapView.GetCursor(info.hover.x, info.hover.y, &cx, &cy);
		cursorX = brushType[brush] == OT_Terrain ? (int)(cx * 2.f) : (int)cx;
		cursorY = brushType[brush] == OT_Terrain ? (int)(cy * 2.f) : (int)cy;

		switch(ev)
		{
			case IE_Down:
			{
				ObjectType detail = map.GetDetailType(cursorX, cursorY);

				if(brushType[brush] == OT_Road)
					bRemoveRoad = (detail == OT_Road);

				switch(brushType[brush])
				{
					case OT_Castle:
					{
						if(detail == OT_Castle)
							castleEdit.Show(map.GetTile(cursorX, cursorY)->GetCastle());
						else
							map.PlaceCastle(cursorX, cursorY, brushIndex[brush]);
						break;
					}
					case OT_Flag:
					{
						if(detail == OT_Flag)
							map.ClearDetail(cursorX, cursorY);
						else
							map.PlaceFlag(cursorX, cursorY, brushIndex[brush]);
						break;
					}
					case OT_Place:
					{
						if(detail == OT_Place)
							map.ClearDetail(cursorX, cursorY);
						else
							map.PlaceSpecial(cursorX, cursorY, brushIndex[brush]);
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
							map.ClearDetail(cursorX, cursorY);
							break;
						}
						case OT_Terrain:
						{
							if(brushSize == 1)
							{
								int terrain = brushIndex[brush];

								uint32 t = map.GetTerrainAt(cursorX >> 1, cursorY >> 1);
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

								map.SetTerrain(cursorX >> 1, cursorY >> 1, tl, tr, bl, br, mask);
							}
							else
							{
								int terrain = brushIndex[brush];
								map.SetTerrain(cursorX >> 1, cursorY >> 1, terrain, terrain, terrain, terrain);
							}
							break;
						}
						case OT_Road:
						{
							if(bRemoveRoad)
							{
								if(map.GetDetailType(cursorX, cursorY) == OT_Road)
									map.ClearDetail(cursorX, cursorY);
							}
							else
							{
								map.PlaceRoad(cursorX, cursorY);
							}
							break;
						}
						case OT_Region:
						{
							map.SetRegion(cursorX, cursorY, brushIndex[brush]);
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
	mapView.Update();

	castleEdit.Update();

	if(MFInput_WasPressed(Key_S, IDD_Keyboard) && MFInput_Read(Key_LControl, IDD_Keyboard))
		map.Save();

	bool bChangeRace = MFInput_Read(Key_LShift, IDD_Keyboard) || MFInput_Read(Key_RShift, IDD_Keyboard);

	if(MFInput_WasPressed(Key_Grave, IDD_Keyboard))
	{
		if(bChangeRace)
		{
			editRace = 0;
			map.ConstructMap(editRace);
			map.SetEditRace(editRace);
		}
		else
		{
			editRegion = 15;
			map.SetEditRegion(editRegion);
		}
	}

	const UnitDefinitions &units = map.UnitDefs();
	int maxRace = units.GetNumRaces() - 1;

	MFDebug_Assert(maxRace <= 12, "Too many races!!");

	MFKeyboardButton keys[12] = { Key_1, Key_2, Key_3, Key_4, Key_5, Key_6, Key_7, Key_8, Key_9, Key_0, Key_Hyphen, Key_Equals };
	for(int a=0; a<maxRace; ++a)
	{
		if(MFInput_WasPressed(keys[a], IDD_Keyboard))
		{
			if(bChangeRace)
			{
				editRace = a + 1;
				map.ConstructMap(editRace);
				map.SetEditRace(editRace);
			}
			else
			{
				editRegion = a;
				map.SetEditRegion(editRegion);
			}
		}
	}

	return 0;
}

void Editor::Draw()
{
	mapView.Draw();

	// draw debug
	static bool bDrawDebug = false;
	if(MFInput_WasPressed(Key_F1, IDD_Keyboard))
		bDrawDebug = !bDrawDebug;
	if(MFInput_WasPressed(Key_F2, IDD_Keyboard))
		brushSize = brushSize == 1 ? 2 : 1;

	if(bDrawDebug)
		mapView.DrawDebug();

	pMiniMap->Draw();
	pModeButton->Draw();

	pBrushButton[0]->Draw();
	pBrushButton[1]->Draw();

	brushSelect.Draw();
	castleEdit.Draw();

	const char *pText = "Race: Template";
	if(editRace > 0)
		pText = MFStr("Race: %s", map.UnitDefs().GetRaceName(editRace));
	MFFont_DrawText2(MFFont_GetDebugFont(), 87.f, 10.f, 32.f, MFVector::black, pText);
	MFFont_DrawText2(MFFont_GetDebugFont(), 85.f, 8.f, 32.f, MFVector::white, pText);

	pText = "Region: X";
	if(editRegion != 15)
		pText = MFStr("Region: %d", editRegion + 1);
	MFFont_DrawText2(MFFont_GetDebugFont(), 87.f, 47.f, 32.f, MFVector::black, pText);
	MFFont_DrawText2(MFFont_GetDebugFont(), 85.f, 45.f, 32.f, MFVector::white, pText);
}

void Editor::Deselect()
{
	pInputManager->PopReceiver(this);
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
			const Tileset &tileset = map.Tileset();

			if(index == 1)
			{
				rect = tileset.GetWaterUVs(texelOffset);
				pMat = tileset.GetWaterMaterial();
			}
			else
			{
				int tile = 0;
				tileset.FindBestTiles(&tile, EncodeTile(index, index, index, index), 0xFFFFFFFF, 1);

				rect = tileset.GetTileUVs(tile, texelOffset);
				pMat = tileset.GetTileMaterial();
			}
			break;
		}
		case OT_Castle:
		{
			const UnitDefinitions &units = map.UnitDefs();
			rect = units.GetCastleUVs(index + 1, texelOffset);
			colour = GetPlayerColour(index);
			pMat = units.GetCastleMaterial();
			break;
		}
		case OT_Flag:
		{
			const UnitDefinitions &units = map.UnitDefs();
			rect = units.GetFlagUVs(index - 1, texelOffset);
			pMat = units.GetCastleMaterial();
			--brushIndex[brush];
			break;
		}
		case OT_Place:
		{
			const UnitDefinitions &units = map.UnitDefs();
			rect = units.GetSpecialUVs(index, texelOffset);
			pMat = units.GetMiscMaterial();
			break;
		}
		case OT_Road:
		{
			const Tileset &tileset = map.Tileset();
			rect = tileset.GetRoadUVs(index, texelOffset);
			pMat = tileset.GetRoadMaterial();
			break;
		}
		case OT_Region:
		{
			const UnitDefinitions &units = map.UnitDefs();
			if(index == 15)
				index = -1;
			rect = units.GetFlagUVs(index + 1, texelOffset);
			colour = GetPlayerColour(index);
			pMat = units.GetCastleMaterial();
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
	mapView.SetMoveKey(bPaintMode);

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

	MFMaterial_Release(pIcons);
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

CastleEdit::CastleEdit(Editor &editor)
: editor(editor)
{
	pFont = editor.GetTextFont();
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

	const UnitDefinitions &defs = editor.Map().UnitDefs();
	MFMaterial *pUnitMat = defs.GetUnitMaterial();
	int numUnits = defs.GetNumUnitTypes();

	int addedCount = 1;
	for(int a=0; a<numUnits; ++a)
	{
		// don't show heroes
		if(defs.GetUnitType(a) == UT_Hero)
			continue;

		rect = defs.GetUnitUVs(a, false, texelOffset);
		Button *pButton = unitSelect.AddButton(addedCount / 11, pUnitMat, &rect, editor.GetPlayerColour(-1), a, MakeDelegate(this, &CastleEdit::SetUnit));
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
		const UnitDefinitions &unitDefs = editor.Map().UnitDefs();
		const UnitDetails &unit = unitDefs.GetUnitDetails(building);

		int height = (int)MFFont_GetFontHeight(pFont);
		MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 5, MFVector::white, unit.name.CStr());
		if(unit.type == UT_Vehicle)
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height, MFVector::white, "Mov: %d%s", unit.movement, unit.movementClass > 0 ? MFStr(" (%s)", unitDefs.GetMovementClassName(unit.movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*2, MFVector::white, "Turns: %d", pCastle->buildTime);
		}
		else
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height, MFVector::white, "Type: %s", unitDefs.GetArmourClassName(unit.armour));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*2, MFVector::white, "Atk: %d - %d %s %s", unit.attackMin, unit.attackMax, unitDefs.GetWeaponClassName(unit.attack), unitDefs.GetAttackTypeName(unit.atkType));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 10 + height*3, MFVector::white, "Mov: %d%s", unit.movement, unit.movementClass > 0 ? MFStr(" (%s)", unitDefs.GetMovementClassName(unit.movementClass)) : "");
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

	pTemplate = editor.Map().GetCastleTemplate(pCastle->details.x, pCastle->details.y);

	bVisible = true;

	pInputManager->PushReceiver(this);

	const UnitDefinitions &unitDefs = pCastle->UnitDefs();

	MFMaterial *pUnitMat = unitDefs.GetUnitMaterial();
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
			uvs = unitDefs.GetUnitUVs(pCastle->details.buildUnits[a].unit, false, texelOffset);
			pBuildUnits[a]->SetImage(pUnitMat, &uvs, editor.GetPlayerColour(pCastle->player));
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

		const UnitDetails &details = pCastle->UnitDefs().GetUnitDetails(pTemplate->buildUnits[a].unit);
		pTemplate->buildUnits[a].buildTime -= details.buildTime;
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
	const UnitDefinitions &defs = editor.Map().UnitDefs();

	int selected = pCastle->nextBuild;

	// set the new unit
	BuildUnit &unit = pCastle->details.buildUnits[selected];
	const UnitDetails &details = defs.GetUnitDetails(buttonID);
	unit.unit = buttonID;
	unit.buildTime = buttonID >= 0 ? details.buildTime : 0;

	pCastle->buildTime = buttonID >= 0 ? unit.buildTime : 0;

	// update the button image
	if(buttonID != -1)
	{
		
		MFMaterial *pUnitMat = defs.GetUnitMaterial();
		MFRect rect = defs.GetUnitUVs(buttonID, false, MFRenderer_GetTexelCenterOffset());

		pBuildUnits[selected]->SetImage(pUnitMat, &rect, editor.GetPlayerColour(pCastle->player));
	}
	else
	{
		MFRect rect = { 0.5f, 0, 0.25f, 0.25f };
		pBuildUnits[selected]->SetImage(pIcons, &rect);
	}

	unitSelect.Hide();
	bHide = false;
}

void CastleEdit::NameChangeCallback(MFString string)
{
	pCastle->SetName(string);
	pTemplate->name = string;
}
