#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"
#include "Display.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"
#include "MFRenderer.h"

#include "Path.h"

#include "stdio.h"

void DrawHealthBar(int x, int y, int maxHealth, float currentHealth);

MapScreen::MapScreen(Game *_pGame)
{
	pGame = _pGame;
	pSelection = NULL;

	bMoving = false;

	pIcons = MFMaterial_Create("Icons");
	pFont = pGame->GetTextFont();

	// buttons
	Map *pMap = pGame->GetMap();
	Tileset *pTiles = pMap->GetTileset();
	UnitDefinitions *pUnits = pGame->GetUnitDefs();

	MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	// end turn button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = display.width - (16.f + (float)tileWidth);
	pos.y = display.height - (16.f + (float)tileHeight);
	uvs.x = 0.25f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pEndTurn = Button::Create(pIcons, &pos, &uvs, MFVector::one, EndTurn, this, 0, false);

	// minimap button
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	pMiniMap = Button::Create(pIcons, &pos, &uvs, MFVector::one, ShowMiniMap, this, 0, false);
}

MapScreen::~MapScreen()
{
	MFMaterial_Destroy(pIcons);
}

void MapScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pGame->GetMap());
	pInputManager->PushReceiver(pEndTurn);
	pInputManager->PushReceiver(pMiniMap);
}

bool MapScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// only handle left clicks
	if(info.buttonID != 0)
	{
		if(info.buttonID == 1 && ev == IE_Tap && !bMoving)
			DeselectGroup();
		return false;
	}

	switch(ev)
	{
		case IE_Tap:
		{
			// if a unit is moving, disable interaction
			if(bMoving)
				return true;

			Map *pMap = pGame->GetMap();

			// calculate the cursor position
			int cursorX, cursorY;
			pMap->GetCursor(info.tap.x, info.tap.y, &cursorX, &cursorY);

			// get the tile
			MapTile *pTile = pMap->GetTile(cursorX, cursorY);

			if(pSelection && pSelection->GetPlayer() == pGame->CurrentPlayer())
			{
				// if we've already selected a group on this tile
				if(pTile == pSelection->pTile)
				{
					DeselectGroup();

					// show group config screen
					groupConfig.Show(pTile);
					return true;
				}

				// get the selected groups tile
				pTile = pSelection->GetTile();

				// if the selected group has a path already planned
				if(pSelection->pPath)
				{
					Step *pLast = pSelection->pPath;

					// first check for an attack command
					if(!pLast->pNext && cursorX == pLast->x && cursorY == pLast->y)
					{
						// the path is only a single item long, it may be an attack or search command
						MapTile *pTile = pMap->GetTile(cursorX, cursorY);

						if(pTile->IsEnemyTile(pSelection->GetPlayer()))
						{
							// we have an attack command!
							Castle *pCastle = pTile->GetCastle();

							// check the castle is occuppied
							if(pCastle && pTile->GetNumUnits() == 0)
							{
								// search castle squares for units
								for(int a=0; a<4; ++a)
								{
									MapTile *pCastleTile = pCastle->GetTile(a);
									if(pCastleTile->GetNumUnits() != 0)
									{
										pTile = pCastleTile;
										break;
									}
								}
							}

							if(pTile->GetNumUnits() == 0)
							{
								if(pCastle)
								{
									// if it's a merc castle, we need to fight the mercs
									if(pCastle->player == -1)
									{
										// create merc group
										Group *pGroup = pCastle->GetMercGroup();
										pTile->AddGroup(pGroup);
										pGame->BeginBattle(pSelection, pTile);
										break;
									}
									else
									{
										// the castle is empty! claim that shit!
										pCastle->Capture(pGame->CurrentPlayer());
									}
								}
								else
								{
									// there must be empty enemy vehicles on the tile, we'll capture the empty vehicles
									for(int a=0; a<pTile->GetNumGroups(); ++a)
										pTile->GetGroup(a)->SetPlayer(pGame->CurrentPlayer());
								}
							}
							else
							{
								// begin the battle!
								pGame->BeginBattle(pSelection, pTile);
								break;
							}
						}
						else if(pTile->GetType() == OT_Special)
						{
							// search command
							Unit *pHero = pSelection->GetHero();
							if(pHero)
							{
								if(pTile->GetNumUnits() < 10 - pSelection->GetNumUnits())
								{
									// move to ruin
									pSelection->GetTile()->RemoveGroup(pSelection);
									pTile->AddGroup(pSelection);
									pMap->ClaimFlags(pTile->GetX(), pTile->GetY(), pSelection->GetPlayer());

									// strip the step from the path
									pSelection->pPath = pMap->StripStep(pSelection->pPath);

									// TODO: random encounter?

									// get an item
									int item = MFRand() % pHero->GetDefs()->GetNumItems();
									pHero->AddItem(item);

									// TODO: show dialog box mentioning what you got
								}
							}
							break;
						}
					}

					// find the final destination
					while(pLast->pNext)
						pLast = pLast->pNext;

					// and see if we've commanded to move there
					if(cursorX == pLast->x && cursorY == pLast->y)
					{
						// move to destination...
						bMoving = true;
						countdown = 0.f;
						break;
					}
				}

				// plot a path to the new location
				if(pSelection->pPath)
					pMap->DestroyPath(pSelection->pPath);
				pSelection->pPath = pMap->FindPath(pSelection, cursorX, cursorY);
			}
			else
			{
				// if there is a group on the tile
				Group *pGroup = pTile->GetGroup(0);
				if(pGroup)
				{
					// select the group
					SelectGroup(pGroup);
				}
				else
				{
					// see if there's a castle on the square
					Castle *pCastle = pTile->GetCastle();
					if(pCastle && pCastle->GetPlayer() == pGame->CurrentPlayer())
					{
						// enter the castle config menu
						castleConfig.Show(pCastle);
					}
				}
			}
			break;
		}
	}

	return false;
}

int MapScreen::UpdateInput()
{

	return 0;
}

int MapScreen::Update()
{
	pGame->GetMap()->Update();

	if(bMoving)
	{
		countdown -= MFSystem_TimeDelta();

		while(countdown <= 0.f)
		{
			Map *pMap = pGame->GetMap();
			int x = pSelection->pPath->x;
			int y = pSelection->pPath->y;

			// validate we can move to the new square, and subtract movement penalty
			MapTile *pNewTile = pMap->GetTile(x, y);
			if(!pNewTile->CanMove(pSelection) || !pSelection->SubtractMovementCost(pNewTile))
			{
				bMoving = false;
				break;
			}

			// remove the group from the current tile
			MapTile *pOldTile = pSelection->GetTile();
			pOldTile->RemoveGroup(pSelection);

			// add the group to the new tile
			pNewTile->AddGroup(pSelection);
			pMap->ClaimFlags(x, y, pSelection->GetPlayer());

			// strip the step from the path
			pSelection->pPath = pMap->StripStep(pSelection->pPath);

			// center the map on the guy moving
			pMap->CenterView(x, y);

			countdown += 0.15f;

			// if we have reached our destination
			if(!pSelection->pPath)
			{
				bMoving = false;
				break;
			}
		}
	}

	return 0;
}

void MapScreen::Draw()
{
	Map *pMap = pGame->GetMap();

	pMap->Draw();

	if(pSelection)
	{
		// render the path
		if(pSelection->GetPlayer() == pGame->CurrentPlayer())
		{
			Step *pPath = pSelection->GetPath();

			if(pPath)
			{
				MFView_Push();

				int xTiles, yTiles;
				pMap->SetMapOrtho(&xTiles, &yTiles);

				float xStart, yStart;
				pMap->GetOffset(&xStart, &yStart);
				int xS = (int)xStart, yS = (int)yStart;

				xTiles += xS;
				yTiles += yS;

				// draw path
				while(pPath)
				{
					if(pPath->x >= xS && pPath->y >= yS && pPath->x < xTiles && pPath->y < yTiles)
						MFPrimitive_DrawUntexturedQuad((float)pPath->x + 0.4f, (float)pPath->y + 0.4f, 0.2f, 0.2f, MFVector::black);
					pPath = pPath->pNext;
				}

				MFView_Pop();
			}
		}

		// draw the group info
		int numUnits = pSelection->GetNumUnits();
		int tx = 42 + numUnits*32;
		int ty = 37 - (int)MFFont_GetFontHeight(pFont)/2;

		if(numUnits == 0)
		{
			Unit *pUnit = pSelection->GetVehicle();
			UnitDetails *pDetails = pUnit->GetDetails();
			pUnit->Draw(5.f + (pDetails->width-1)*22.f, 5.f + (pDetails->height-1)*30.f);
			tx += (pDetails->width-1)*76;
			ty += (pDetails->height-1)*18;
		}
		else
		{
			for(int a = numUnits-1; a >= 0; --a)
			{
				Unit *pUnit = pSelection->GetUnit(a);
				pUnit->Draw(5.f + (float)a*32.f, 5.f);
			}
		}
		Game::GetCurrent()->GetUnitDefs()->DrawUnits(64.f, MFRenderer_GetTexelCenterOffset());

		MFFont_BlitTextf(pFont, tx+1, ty+1, MakeVector(0,0,0,1), "Move: %g", (float)pSelection->GetMovement() * 0.5f);
		MFFont_BlitTextf(pFont, tx, ty, MakeVector(1,1,0,1), "Move: %g", (float)pSelection->GetMovement() * 0.5f);
	}

	pEndTurn->Draw();
	pMiniMap->Draw();

	// now draw any UI that might be on the screen.
	groupConfig.Draw();
	castleConfig.Draw();
}

void MapScreen::Deselect()
{

}

void MapScreen::EndTurn(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;

	pThis->DeselectGroup();

	pThis->pGame->EndTurn();
}

void MapScreen::ShowMiniMap(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;

}

void MapScreen::SelectGroup(Group *pGroup)
{
	if(pSelection == pGroup)
		return;

	if(pSelection)
		pSelection->bSelected = false;

	pSelection = pGroup;
	pSelection->bSelected = true;
}

void MapScreen::DeselectGroup()
{
	if(pSelection)
		pSelection->bSelected = false;
	pSelection = NULL;
}

Group *MapScreen::GetSelected()
{
	return pSelection;
}

static float gPositions[5][5][2] =
{
	{ {.5f, .5f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .5f},  {.67f, .5f},  {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .5f},  {.32f, .75f}, {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .25f}, {.32f, .75f}, {.67f, .75f}, {0.f, 0.f}  },
	{ {.16f, .25f}, {.84f, .25f}, {.5f, .5f},   {.16f, .75f}, {.84f, .75f} }
};

Inventory::Inventory()
{
	pFont = Game::GetCurrent()->GetTextFont();
	pItems = MFMaterial_Create("Items");

	bVisible = false;

	float margin = 5.f;
	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;
	AdjustRect_Margin(&window, margin*2.f);

	float x = window.x + window.width*0.5f - (32.f*4+8*3)*0.5f;
	for(int a=0; a<8; ++a)
	{
		MFRect pos = { x + (a & 0x3)*40.f, window.y + 64.f + (a >> 2)*40.f, 32.f, 32.f };
		pInventory[a] = Button::Create(pItems, &pos, &pos, MFVector::one, SelectItem, this, a);
	}
}

Inventory::~Inventory()
{
	MFMaterial_Destroy(pItems);
}

bool Inventory::Draw()
{
	if(!bVisible)
		return false;

	Game::GetCurrent()->DrawWindow(window);
	Game::GetCurrent()->DrawLine(window.x + 16, window.y + 160, window.x + window.width - 16, window.y + 160);

	float width = MFFont_GetStringWidth(pFont, "Inventory", MFFont_GetFontHeight(pFont));
	MFFont_BlitText(pFont, (int)(window.x + window.width*0.5f - width*0.5f), (int)window.y + 10, MFVector::yellow, "Inventory");

	for(int a=0; a<numItems; ++a)
		pInventory[a]->Draw();

	if(selected != -1)
	{
		Item *pItem = pUnit->GetItem(selected);
		width = MFFont_GetStringWidth(pFont, pItem->pName, MFFont_GetFontHeight(pFont));
		MFFont_BlitText(pFont, (int)(window.x + window.width*0.5f - width*0.5f), (int)window.y + 180, MFVector::white, pItem->pName);
	}

	return true;
}

bool Inventory::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Tap:
			break;
	}

	return true;
}

void Inventory::Show(Unit *_pUnit)
{
	pUnit = _pUnit;

	bVisible = true;
	selected = -1;

	pInputManager->PushReceiver(this);

	UnitDefinitions *pDefs = pUnit->GetDefs();
	float texelCenter = MFRenderer_GetTexelCenterOffset();

	numItems = pUnit->GetNumItems();
	for(int a=0; a<numItems; ++a)
	{
		Item *pItem = pUnit->GetItem(a);

		MFRect uvs;
		pDefs->GetItemUVs(pUnit->GetItemID(a), &uvs, texelCenter);
		pInventory[a]->SetImage(pDefs->GetItemMaterial(), &uvs);
		pInputManager->PushReceiver(pInventory[a]);
	}
}

void Inventory::Hide()
{
	bVisible = false;

	pInputManager->PopReceiver(this);
}

void Inventory::SelectItem(int button, void *pUserData, int buttonID)
{
	Inventory *pThis = (Inventory*)pUserData;
	pThis->selected = buttonID;
}

UnitConfig::UnitConfig()
{
	pFont = Game::GetCurrent()->GetTextFont();

	bVisible = false;

	float margin = 5.f;
	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;
	AdjustRect_Margin(&window, margin*2.f);

	DivideRect_Vert(window, 128+10, margin, &top, &bottom, true);

	pIcons = MFMaterial_Create("Icons");
	MFRect pos = { top.x + top.width - 64.f, top.y + top.height - 64.f, 64.f, 64.f };
	MFRect uvs = { 0.f + (.5f/256.f), 0.5f + (.5f/256.f), 0.25f, 0.25f };
	pInventory = Button::Create(pIcons, &pos, &uvs, MFVector::white, Inventory, this);

	float height = MFFont_GetFontHeight(pFont);
	MFRect cbRect = { bottom.x + 40, bottom.y + 10, 200.f, height };
	pStrategySelect[0] = CheckBox::Create(&cbRect, "Attack Strongest", MFVector::yellow, 1, SelectStrat, this, 0);
	cbRect.y += height;
	pStrategySelect[1] = CheckBox::Create(&cbRect, "Attack Weakest", MFVector::yellow, 0, SelectStrat, this, 1);
	cbRect.y += height + 10;
	pStrategySelect[2] = CheckBox::Create(&cbRect, "Attack First Available", MFVector::yellow, 1, SelectStrat, this, 2);
	cbRect.x = bottom.x + 260;
	cbRect.y = bottom.y + 15;
	pStrategySelect[3] = CheckBox::Create(&cbRect, "Attack Any", MFVector::yellow, 1, SelectStrat, this, 3);
	cbRect.y += height;
	pStrategySelect[4] = CheckBox::Create(&cbRect, "Attack Melee", MFVector::yellow, 0, SelectStrat, this, 4);
	cbRect.y += height;
	pStrategySelect[5] = CheckBox::Create(&cbRect, "Attack Ranged", MFVector::yellow, 0, SelectStrat, this, 5);
}

UnitConfig::~UnitConfig()
{
	MFMaterial_Destroy(pIcons);
}

bool UnitConfig::Draw()
{
	if(!bVisible)
		return false;

	if(inventory.Draw())
		return true;

	Game::GetCurrent()->DrawWindow(window);
	Game::GetCurrent()->DrawLine(window.x + 16, bottom.y - 5, window.x + window.width - 16, bottom.y - 5);

	UnitDefinitions *pDefs = pUnit->GetDefs();
	UnitDetails *pDetails = pUnit->GetDetails();

	pUnit->Draw(top.x + 32.f, top.y + 32.f);
	if(pDefs)
		pDefs->DrawUnits(64.f, MFRenderer_GetTexelCenterOffset(), false, true);

	// do we want to see this?
//	DrawHealthBar((int)(unit.x + 32.f), (int)(unit.y + 32.f), pDetails->life, pUnit->GetHealth());

	int height = (int)MFFont_GetFontHeight(pFont);
	float tWidth = MFFont_GetStringWidth(pFont, pUnit->GetName(), (float)height);
	MFFont_BlitText(pFont, (int)top.x + ((int)top.width / 2) - (int)(tWidth*0.5f), (int)top.y + 5, MFVector::yellow, pUnit->GetName());
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height, MFVector::white, "Type: %s", pDefs->GetArmourClassName(pDetails->defenceClass));
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*2, MFVector::white, "Atk: %d - %d (%s)", pDetails->attackMin, pDetails->attackMax, pDefs->GetWeaponClassName(pDetails->attackClass));
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*3, MFVector::white, "Mov: %g/%d%s", pUnit->GetMovement()*0.5f, pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pDefs->GetMovementClassName(pDetails->movementClass)) : "");
	MFFont_BlitTextf(pFont, (int)top.x + 133, (int)top.y + 5 + height*4, MFVector::white, "HP: %d/%d", (int)(pDetails->life * pUnit->GetHealth()), pDetails->life);

	MFFont_BlitTextf(pFont, (int)top.x + 320, (int)top.y + 5 + height, MFVector::white, "Victories: %d", pUnit->GetVictories());
	MFFont_BlitTextf(pFont, (int)top.x + 320, (int)top.y + 5 + height*2, MFVector::white, "Kills: %d", pUnit->GetKills());

	if(pUnit->IsHero() && pUnit->GetNumItems() > 0)
		pInventory->Draw();

	for(int a=0; a<6; ++a)
		pStrategySelect[a]->Draw();

	return true;
}

bool UnitConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Tap:
			break;
	}

	return true;
}

void UnitConfig::Show(Unit *_pUnit)
{
	pUnit = _pUnit;

	BattlePlan *pPlan = pUnit->GetBattlePlan();
	pStrategySelect[0]->SetValue(pPlan->strength == TS_Strongest ? 1 : 0);
	pStrategySelect[1]->SetValue(pPlan->strength == TS_Weakest ? 1 : 0);
	pStrategySelect[2]->SetValue(pPlan->bAttackAvailable ? 1 : 0);
	pStrategySelect[3]->SetValue(pPlan->type == TT_Any ? 1 : 0);
	pStrategySelect[4]->SetValue(pPlan->type == TT_Melee ? 1 : 0);
	pStrategySelect[5]->SetValue(pPlan->type == TT_Ranged ? 1 : 0);

	bVisible = true;

	pInputManager->PushReceiver(this);

	if(pUnit->IsHero() && pUnit->GetNumItems() > 0)
		pInputManager->PushReceiver(pInventory);

	for(int a=0; a<6; ++a)
		pInputManager->PushReceiver(pStrategySelect[a]);
}

void UnitConfig::Hide()
{
	bVisible = false;

	pInputManager->PopReceiver(this);
}

void UnitConfig::Inventory(int button, void *pUserData, int buttonID)
{
	UnitConfig *pThis = (UnitConfig*)pUserData;
	pThis->inventory.Show(pThis->pUnit);
}

void UnitConfig::SelectStrat(int value, void *pUserData, int buttonID)
{
	UnitConfig *pThis = (UnitConfig*)pUserData;

	BattlePlan *pPlan = pThis->pUnit->GetBattlePlan();

	switch(buttonID)
	{
		case 0:
		case 1:
			pPlan->strength = (TargetStrength)buttonID;

			if(value == 0)
				pThis->pStrategySelect[buttonID]->SetValue(1);
			else
				pThis->pStrategySelect[1 - buttonID]->SetValue(0);
			break;

		case 2:
			pPlan->bAttackAvailable = !!value;
			break;

		case 3:
		case 4:
		case 5:
			pPlan->type = (TargetType)(buttonID - 3);

			if(value == 0)
			{
				pThis->pStrategySelect[buttonID]->SetValue(1);
			}
			else
			{
				for(int a=3; a<6; ++a)
				{
					if(buttonID != a)
						pThis->pStrategySelect[a]->SetValue(0);
				}
			}
			break;
	}
}

GroupConfig::GroupConfig()
{
	pFont = Game::GetCurrent()->GetTextFont();

	bVisible = false;

	float margin = 5.f;
	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;
	AdjustRect_Margin(&window, margin*2.f);

	MFRect center, middle;
	DivideRect_Vert(window, window.height - 64 - margin*4, 0.f, &center, &lower, true);
	DivideRect_Vert(center, center.height - 64*2 - margin*8, 0.f, &top, &middle, true);
	DivideRect_Horiz(middle, 0.5f, 0.f, &rear, &front, false);
	AdjustRect_Margin(&top, margin);
	AdjustRect_Margin(&front, margin);
	AdjustRect_Margin(&rear, margin);
}

GroupConfig::~GroupConfig()
{
}

void GroupConfig::Draw()
{
	if(!bVisible)
		return;

	if(battleConfig.Draw())
		return;

//	Game::GetCurrent()->DrawWindow(window);
	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0, 0, 0, 0.8f));

	MFPrimitive_DrawUntexturedQuad(top.x, top.y, top.width, top.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(rear.x, rear.y, rear.width, rear.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(front.x, front.y, front.width, front.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(empty.x, empty.y, empty.width, empty.height, MakeVector(0, 0, 0, .8f));

	for(int a = 0; a < numExtraGroups; ++a)
		MFPrimitive_DrawUntexturedQuad(bottom[a].x, bottom[a].y, bottom[a].width, bottom[a].height, MakeVector(0, 0, 0, .8f));

	UnitDefinitions *pDefs = units[0].pUnit->GetDefs();

	for(int a = numUnits-1; a >= 0; --a)
	{
		GroupUnit &unit = units[a];
		unit.pUnit->Draw(unit.x - 32.f, unit.y - 32.f);
	}

	if(pDefs)
		pDefs->DrawUnits(64.f, MFRenderer_GetTexelCenterOffset(), false, true);

	for(int a = numUnits-1; a >= 0; --a)
	{
		GroupUnit &unit = units[a];

		if(unit.pGroup == units[0].pGroup && !unit.pUnit->IsVehicle())
			DrawHealthBar(unit.x - 32, unit.y - 32, unit.pUnit->GetDetails()->life, unit.pUnit->GetHealth());

		char move[8];
		sprintf(move, "%g", unit.pUnit->GetMovement() * 0.5f);

		MFFont *pFont = Game::GetCurrent()->GetSmallNumbersFont();
		float height = MFFont_GetFontHeight(pFont);
		float moveWidth = MFFont_GetStringWidth(pFont, move, height);

		MFFont_BlitText(pFont, unit.x + 30 - (int)moveWidth, unit.y + 30 - (int)height, MFVector::white, move);
	}
}

bool GroupConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Tap:
		{
			// see if we have selected a unit
			int unit = GetUnitFromPoint(info.tap.x, info.tap.y);
			if(unit > -1)
			{
				if(!units[unit].pUnit->IsVehicle())
					battleConfig.Show(units[unit].pUnit);
			}

			// see if we have selected another group
			int group = GetGroupFromPoint(info.tap.x, info.tap.y);
			if(group > -1)
			{
				pTile->BringGroupToFront(pExtraGroups[group]);
				PositionUnits();
			}
			break;
		}
		case IE_Drag:
		{
			if(dragUnit == -1 && pDragGroup == NULL)
			{
				// check for unit drag
				int unit = GetUnitFromPoint(info.drag.startX, info.drag.startY);
				if(unit > -1)
				{
					dragUnit = unit;
					break;
				}

				// check for group drag
				int group = GetGroupFromPoint(info.drag.startX, info.drag.startY);
				if(group > -1)
				{
					pDragGroup = pExtraGroups[group];
					break;
				}
			}
			else
			{
				if(dragUnit > -1)
				{
					units[dragUnit].x = (int)info.drag.x;
					units[dragUnit].y = (int)info.drag.y;
				}
				else if(pDragGroup)
				{
					for(int a=0; a<numUnits; ++a)
					{
						if(units[a].pGroup == pDragGroup)
						{
							units[a].x += (int)info.drag.deltaX;
							units[a].y += (int)info.drag.deltaY;
						}
					}
				}
			}
			break;
		}
		case IE_Up:
		{
			// find where we dropped it...
			if(dragUnit != -1)
			{
				bool bFound = false;

				if(units[dragUnit].rank != 2)
				{
					// check if we're swapping with another unit
					int unit = GetUnitFromPoint(info.up.x, info.up.y);
					if(unit > -1 && units[unit].pGroup == units[dragUnit].pGroup)
					{
						units[unit].pGroup->SwapUnits(units[unit].pUnit, units[dragUnit].pUnit);
						bFound = true;
					}

					// check dropping in the other file
					if(!bFound)
					{
						int dropped = GetFileFromPoint(info.up.x, info.up.y);
						if(dropped > -1)
						{
							if(dropped != units[dragUnit].rank)
							{
								Unit *pUnit = units[dragUnit].pUnit;
								Group *pGroup = units[dragUnit].pGroup;

								// check there is room in the target file
								if(dropped == 0)
								{
									if(numFront < 5)
									{
										pGroup->RemoveUnit(pUnit);
										pGroup->AddForwardUnit(pUnit);
									}
								}
								else
								{
									if(numRear < 5)
									{
										pGroup->RemoveUnit(pUnit);
										pGroup->AddRearUnit(pUnit);
									}
								}
							}

							bFound = true;
						}
					}
				}

				// check for dropping in other groups
				if(!bFound)
				{
					int group = GetGroupFromPoint(info.up.x, info.up.y);
					if(group > -1)
					{
						if(pExtraGroups[group]->GetNumUnits() < 10 || units[dragUnit].rank == 2)
						{
							Unit *pUnit = units[dragUnit].pUnit;
							Group *pGroup = units[dragUnit].pGroup;

							if(pExtraGroups[group]->AddUnit(pUnit))
								pGroup->RemoveUnit(pUnit);

							// check if we have just emptied the group
							if(pGroup->GetNumUnits() == 0 && pGroup->GetVehicle() == NULL)
							{
								pTile->RemoveGroup(pGroup);
								pGroup->Destroy();
							}
						}

						bFound = true;
					}
				}

				// check for dropping into the 'new' group
				if(!bFound)
				{
					if(MFTypes_PointInRect(info.up.x, info.up.y, &empty))
					{
						Unit *pUnit = units[dragUnit].pUnit;
						Group *pGroup = units[dragUnit].pGroup;

						Group *pNewGroup = Group::Create(pGroup->GetPlayer());
						pTile->AddGroupToBack(pNewGroup);

						if(pNewGroup->AddUnit(pUnit))
							pGroup->RemoveUnit(pUnit);

						// check if we have just emptied the group
						if(pGroup->GetNumUnits() == 0 && pGroup->GetVehicle() == NULL)
						{
							pTile->RemoveGroup(pGroup);
							pGroup->Destroy();
						}
					}
				}

				dragUnit = -1;
			}
			else if(pDragGroup)
			{
				// check if we dropped the group in one of the top files
				int dropped = GetFileFromPoint(info.up.x, info.up.y);
				if(dropped > -1)
				{
					// move as many into the chosen file as possible
					Group *pTarget = units[0].pGroup;

					Unit *pVehicle = pDragGroup->GetVehicle();
					if(pVehicle)
					{
						if(pTarget->AddUnit(pVehicle))
							pDragGroup->RemoveUnit(pVehicle);
					}

					while(pDragGroup->GetNumUnits() > 0 && pTarget->GetNumUnits() < 10)
					{
						Unit *pUnit = pDragGroup->GetUnit(0);
						pDragGroup->RemoveUnit(pUnit);
						if(dropped == 0)
						{
							if(!pTarget->AddForwardUnit(pUnit))
								pTarget->AddRearUnit(pUnit);
						}
						else
						{
							if(!pTarget->AddRearUnit(pUnit))
								pTarget->AddForwardUnit(pUnit);
						}
					}
				}
				else
				{
					// check if we dropped the group on another group
					int group = GetGroupFromPoint(info.up.x, info.up.y);

					if(group > -1 && pExtraGroups[group] != pDragGroup)
					{
						// move as many into the target group as possible
						Unit *pVehicle = pDragGroup->GetVehicle();
						if(pVehicle)
						{
							if(pExtraGroups[group]->AddUnit(pVehicle))
								pDragGroup->RemoveUnit(pVehicle);
						}

						while(pDragGroup->GetNumUnits() > 0 && pExtraGroups[group]->GetNumUnits() < 10)
						{
							Unit *pUnit = pDragGroup->GetUnit(0);
							if(pExtraGroups[group]->AddUnit(pUnit))
								pDragGroup->RemoveUnit(pUnit);
						}
					}
				}

				if(pDragGroup->GetNumUnits() == 0 && pDragGroup->GetVehicle() == NULL)
				{
					pTile->RemoveGroup(pDragGroup);
					pDragGroup->Destroy();
				}

				pDragGroup = NULL;
			}

			PositionUnits();
			break;
		}
	}

	return true;
}

int GroupConfig::GetUnitFromPoint(float x, float y)
{
	int unit = -1;

	Group *pFirstGroup = units[0].pGroup;
	for(int a=0; a<numUnits && units[a].pGroup == pFirstGroup; ++a)
	{
		if(x >= units[a].x - 32 && x < units[a].x + 32 &&
			y >= units[a].y - 32 && y < units[a].y + 32)
		{
			if(a != dragUnit)
			{
				unit = a;
				break;
			}
		}
	}

	return unit;
}

int GroupConfig::GetFileFromPoint(float x, float y)
{
	int file = -1;

	if(MFTypes_PointInRect(x, y, &front))
		file = 0;
	else if(MFTypes_PointInRect(x, y, &rear))
		file = 1;

	return file;
}

int GroupConfig::GetGroupFromPoint(float x, float y)
{
	int group = -1;

	for(int a=0; a<numExtraGroups; ++a)
	{
		if(MFTypes_PointInRect(x, y, &bottom[a]))
		{
			group = a;
			break;
		}
	}

	return group;
}

void GroupConfig::Show(MapTile *_pTile)
{
	bVisible = true;
	pTile = _pTile;

	dragUnit = -1;
	pDragGroup = NULL;

	pInputManager->PushReceiver(this);

	PositionUnits();
}

void GroupConfig::PositionUnits()
{
	int numGroups = MFMin(pTile->GetNumGroups(), 5);
	numUnits = 0;
	numFront = numRear = -1;

	for(int a=0; a<numGroups; ++a)
	{
		Group *pG = pTile->GetGroup(a);

		int front = pG->GetNumForwardUnits();
		int rear = pG->GetNumRearUnits();

		if(numFront == -1)
		{
			numFront = front;
			numRear = rear;
		}

		for(int b=0; b<front; ++b)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pG->GetForwardUnit(b);
			unit.rank = 0;
		}

		for(int b=0; b<rear; ++b)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pG->GetRearUnit(b);
			unit.rank = 1;
		}

		Unit *pVehicle = pG->GetVehicle();
		if(pVehicle)
		{
			GroupUnit &unit = units[numUnits++];
			unit.pGroup = pG;
			unit.pUnit = pVehicle;
			unit.rank = 2;
		}
	}

	// reposition the lower groups
	float bottomUsed = 0.f;
	const float margin = 5.f;

	numExtraGroups = -1;
	Group *pCurrentGroup = units[0].pGroup;

	for(int a=0; a<numUnits; ++a)
	{
		if(units[a].pGroup != pCurrentGroup)
		{
			++numExtraGroups;
			pCurrentGroup = units[a].pGroup;

			pExtraGroups[numExtraGroups] = pCurrentGroup;

			bottom[numExtraGroups].x = lower.x + bottomUsed;
			bottom[numExtraGroups].y = lower.y;
			bottom[numExtraGroups].width = 32.f + margin*4.f;
			bottom[numExtraGroups].height = lower.height;
			bottomUsed += bottom[numExtraGroups].width;

			AdjustRect_Margin(&bottom[numExtraGroups], margin, true);
		}

		if(numExtraGroups < 0)
		{
			// place the units
			if(units[a].rank == 0)
			{
				units[a].x = (int)(front.x + front.width*gPositions[numFront-1][a][0]);
				units[a].y = (int)(front.y + front.height*gPositions[numFront-1][a][1]) + 4;
			}
			else if(units[a].rank == 1)
			{
				units[a].x = (int)(rear.x + rear.width*gPositions[numRear-1][a - numFront][0]);
				units[a].y = (int)(rear.y + rear.height*gPositions[numRear-1][a - numFront][1]) + 4;
			}
			else
			{
				units[a].x = (int)((rear.x + rear.width + front.x)*0.5f - units[a].pUnit->GetDetails()->width*0.5f);
				units[a].y = (int)(front.y + front.height*0.5f - units[a].pUnit->GetDetails()->height*0.5f);
			}
		}
		else
		{
			bottom[numExtraGroups].width += 32.f;
			bottomUsed += 32.f;
			units[a].x = (int)bottom[numExtraGroups].x + (int)bottom[numExtraGroups].width - 32 - (int)margin;
			units[a].y = (int)bottom[numExtraGroups].y + 32 + (int)margin;
		}
	}

	++numExtraGroups;

	empty.x = lower.x + bottomUsed;
	empty.y = lower.y;
	empty.width = lower.width - bottomUsed;
	empty.height = lower.height;
	AdjustRect_Margin(&empty, margin, true);
}

void GroupConfig::Hide()
{
	pInputManager->PopReceiver(this);

	Game::GetCurrent()->GetMapScreen()->SelectGroup(pTile->GetGroup(0));

	bVisible = false;
}

CastleConfig::CastleConfig()
{
	pFont = Game::GetCurrent()->GetTextFont();

	bVisible = false;

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
}

CastleConfig::~CastleConfig()
{
}

void CastleConfig::Draw()
{
	if(!bVisible)
		return;

	Game *pGame = Game::GetCurrent();

	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0, 0, 0, 0.8f));

	MFPrimitive_DrawUntexturedQuad(title.x, title.y, title.width, title.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(units.x, units.y, units.width, units.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(right.x, right.y, right.width, right.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(lower.x, lower.y, lower.width, lower.height, MakeVector(0, 0, 0, .8f));

	MFFont_BlitTextf(pFont, (int)title.x + 5, (int)title.y, MFVector::yellow, pCastle->details.name);

	char gold[16];
	sprintf(gold, "Gold: %d", pGame->GetPlayerGold(pGame->CurrentPlayer()));
	float goldWidth = MFFont_GetStringWidth(pFont, gold, MFFont_GetFontHeight(pFont));
	MFFont_BlitText(pFont, (int)(title.x + title.width - goldWidth - 5), (int)title.y, MFVector::yellow, gold);

	int building = pCastle->GetBuildUnit();
	if(building > -1)
	{
		UnitDefinitions *pUnitDefs = pGame->GetUnitDefs();
		UnitDetails *pDetails = pUnitDefs->GetUnitDetails(building);

		int height = (int)MFFont_GetFontHeight(pFont);
		MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 5, MFVector::white, pDetails->pName);
		if(pDetails->type == UT_Vehicle)
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 9 + height, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 8 + height*2, MFVector::white, "Gold: %d", pCastle->details.buildUnits[pCastle->building].cost);
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 7 + height*3, MFVector::white, "Turns: %d", pCastle->buildTime);
		}
		else
		{
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 9 + height, MFVector::white, "Type: %s", pUnitDefs->GetArmourClassName(pDetails->defenceClass));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 8 + height*2, MFVector::white, "Atk: %d - %d %s", pDetails->attackMin, pDetails->attackMax, pUnitDefs->GetWeaponClassName(pDetails->attackClass));
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 7 + height*3, MFVector::white, "Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : "");
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 6 + height*4, MFVector::white, "Gold: %d", pCastle->details.buildUnits[pCastle->building].cost);
			MFFont_BlitTextf(pFont, (int)right.x + 5, (int)right.y + 5 + height*5, MFVector::white, "Turns: %d", pCastle->buildTime);
		}
	}

	MFFont_BlitTextf(pFont, (int)lower.x + 5, (int)lower.y + 5, MFVector::white, "Income: %d", pCastle->details.income);

	for(int a=0; a<numBuildUnits; ++a)
		pBuildUnits[a]->Draw();
}

bool CastleConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	return true;
}

void CastleConfig::Show(Castle *pCastle)
{
	this->pCastle = pCastle;
	bVisible = true;

	pInputManager->PushReceiver(this);

	UnitDefinitions *pUnitDefs = pCastle->pUnitDefs;
	Game *pGame = pUnitDefs->GetGame();

	MFMaterial *pUnitMat = pUnitDefs->GetUnitMaterial();
	float texelCenter = MFRenderer_GetTexelCenterOffset();

	numBuildUnits = pCastle->details.numBuildUnits;

	int a = 0;
	for(; a<numBuildUnits; ++a)
	{
		MFRect uvs;
		int unit = pCastle->details.buildUnits[a].unit;
		pUnitDefs->GetUnitUVs(pCastle->details.buildUnits[a].unit, false, &uvs, texelCenter);
		pBuildUnits[a]->SetImage(pUnitMat, &uvs, pGame->GetPlayerColour(pCastle->player));

		if(a == 2)
		{
			MFRect pos;
			pos.x = units.x + 5.f;
			pos.y = units.y + units.height - 64.f - 5.f;
			pos.width = pos.height = 64.f;

			if(numBuildUnits == 3)
			{
				float ratio = uvs.width/uvs.height;
				float width = 64.f * ratio;
				pos.x = MFFloor(units.x + units.width*0.5f - width*0.5f);
				pos.width = MFFloor(pos.width * ratio);
			}

			pBuildUnits[a]->SetPos(&pos);
		}

		pBuildUnits[a]->SetOutline(true, pCastle->building == a ? MFVector::blue : MFVector::white);

		pInputManager->PushReceiver(pBuildUnits[a]);
	}

	Unit *pHero = pGame->GetPlayerHero(pCastle->GetPlayer());
	if(a < 4 && pHero->IsDead())
	{
		int unit = Game::GetCurrent()->GetPlayerRace(pCastle->GetPlayer())-1;
		MFRect uvs;
		pUnitDefs->GetUnitUVs(unit, false, &uvs, texelCenter);
		pBuildUnits[a]->SetImage(pUnitMat, &uvs, pGame->GetPlayerColour(pCastle->player));

		UnitDetails *pDetails = pHero->GetDetails();
		pCastle->details.buildUnits[a].unit = unit;
		pCastle->details.buildUnits[a].cost = pDetails->cost;
		pCastle->details.buildUnits[a].buildTime = pDetails->buildTime;

		if(a == 2)
		{
			float ratio = uvs.width/uvs.height;
			float width = 64.f * ratio;

			MFRect pos;
			pos.x = MFFloor(units.x + units.width*0.5f - width*0.5f);
			pos.y = units.y + units.height - 64.f - 5.f;
			pos.width = pos.height = 64.f;

			pBuildUnits[a]->SetPos(&pos);
		}

		pBuildUnits[a]->SetOutline(true, pCastle->building == a ? MFVector::blue : MFVector::white);

		pInputManager->PushReceiver(pBuildUnits[a]);

		++numBuildUnits;
	}
}

void CastleConfig::Hide()
{
	pInputManager->PopReceiver(this);

	bVisible = false;
}

void CastleConfig::SelectUnit(int button, void *pUserData, int buttonID)
{
	CastleConfig *pThis = (CastleConfig*)pUserData;
	Castle *pCastle = pThis->pCastle;

	for(int a=0; a<pThis->numBuildUnits; ++a)
		pThis->pBuildUnits[a]->SetOutline(true, buttonID == a ? MFVector::blue : MFVector::white);

	pCastle->SetBuildUnit(buttonID);
}
