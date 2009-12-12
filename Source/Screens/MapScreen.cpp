#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"
#include "Display.h"

#include "Path.h"

MapScreen::MapScreen(Game *_pGame)
{
	pGame = _pGame;
	pSelection = NULL;

	bMoving = false;

	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("Font/Charlemagne");

	// buttons
	Tileset *pTiles = pGame->GetMap()->GetTileset();
	UnitDefinitions *pUnits = pGame->GetUnitDefs();

	MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };

	// end turn button
	pos.x = (float)(gDefaults.display.displayWidth - (16 + tileWidth));
	pos.y = (float)(gDefaults.display.displayHeight - (16 + tileHeight));
	uvs.x = 0.25f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	uvs.width = 0.25f; uvs.height = 0.25f;
	pEndTurn = Button::Create(pIcons, &pos, &uvs, EndTurn, this, 0, false);

	// minimap button
	pos.y = 16.f;
	uvs.x = 0.75f + (1.f/256.f); uvs.y = 0.f + (1.f/256.f);
	pMiniMap = Button::Create(pIcons, &pos, &uvs, ShowMiniMap, this, 0, false);
}

MapScreen::~MapScreen()
{
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
		if(info.buttonID == 1 && ev == IE_Tap)
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
						// the path is only a single item long, it may be an attack command
						MapTile *pTile = pMap->GetTile(cursorX, cursorY);

						if(pTile->IsEnemyTile(pSelection->GetPlayer()))
						{
							// we have an attack command!

							// check the castle is occuppied
							if(pTile->GetNumGroups() == 0)
							{
								// search castle squares for units
								Castle *pCastle = pTile->GetCastle();

								for(int a=0; a<4; ++a)
								{
									MapTile *pCastleTile = pCastle->GetTile(a);
									if(pCastleTile->GetNumGroups() != 0)
									{
										pTile = pCastleTile;
										break;
									}
								}
							}

							if(pTile->GetNumUnits() == 0)
							{
								Castle *pCastle = pTile->GetCastle();

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
								// begin the battle!
								pGame->BeginBattle(pSelection, pTile);
								break;
							}
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
				pSelection->pPath = pMap->FindPath(pSelection->GetPlayer(), pTile->GetX(), pTile->GetY(), cursorX, cursorY);
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
					if(pCastle)
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
		for(int a = numUnits-1; a >= 0; --a)
		{
			Unit *pUnit = pSelection->GetUnit(a);
			pUnit->Draw(5.f + (float)a*32.f, 5.f);
		}
		pSelection->GetUnit(0)->GetDefs()->DrawUnits(64.f);

		int tx = 42 + numUnits*32;
		int ty = 37 - (int)MFFont_GetFontHeight(pFont)/2;
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

GroupConfig::GroupConfig()
{
	pFont = MFFont_Create("Font/Charlemagne");

	bVisible = false;

	float margin = 5.f;
	MFDisplay_GetDisplayRect(&window);
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
	MFFont_Destroy(pFont);
}

void GroupConfig::Draw()
{
	if(!bVisible)
		return;

	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0, 0, 0, 0.8f));

	MFPrimitive_DrawUntexturedQuad(top.x, top.y, top.width, top.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(rear.x, rear.y, rear.width, rear.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(front.x, front.y, front.width, front.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(empty.x, empty.y, empty.width, empty.height, MakeVector(0, 0, 0, .8f));

	for(int a = 0; a < numExtraGroups; ++a)
		MFPrimitive_DrawUntexturedQuad(bottom[a].x, bottom[a].y, bottom[a].width, bottom[a].height, MakeVector(0, 0, 0, .8f));

	UnitDefinitions *pDefs = NULL;

	for(int a = numUnits-1; a >= 0; --a)
	{
		GroupUnit &unit = units[a];
		unit.pUnit->Draw(unit.x - 32.f, unit.y - 32.f);

		pDefs = unit.pUnit->GetDefs();
	}

	if(pDefs)
		pDefs->DrawUnits(64.f);
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
				// select a unit, print the stats i guess..
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
				// check if we're swapping with another unit
				bool bFound = false;

				// check dropping on another unit in the same group
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

				// check for dropping in other groups
				if(!bFound)
				{
					int group = GetGroupFromPoint(info.up.x, info.up.y);
					if(group > -1)
					{
						if(pExtraGroups[group]->GetNumUnits() < 10)
						{
							Unit *pUnit = units[dragUnit].pUnit;
							Group *pGroup = units[dragUnit].pGroup;

							pGroup->RemoveUnit(pUnit);
							pExtraGroups[group]->AddUnit(pUnit);

							// check if we have just emptied the group
							if(pGroup->GetNumUnits() == 0)
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

						pGroup->RemoveUnit(pUnit);
						pNewGroup->AddUnit(pUnit);

						// check if we have just emptied the group
						if(pGroup->GetNumUnits() == 0)
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
						while(pDragGroup->GetNumUnits() > 0 && pExtraGroups[group]->GetNumUnits() < 10)
						{
							Unit *pUnit = pDragGroup->GetUnit(0);
							pDragGroup->RemoveUnit(pUnit);
							pExtraGroups[group]->AddUnit(pUnit);
						}
					}
				}

				if(pDragGroup->GetNumUnits() == 0)
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
				units[a].y = (int)(front.y + front.height*gPositions[numFront-1][a][1]);
			}
			else
			{
				units[a].x = (int)(rear.x + rear.width*gPositions[numRear-1][a - numFront][0]);
				units[a].y = (int)(rear.y + rear.height*gPositions[numRear-1][a - numFront][1]);
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
	pFont = MFFont_Create("Font/Charlemagne");

	bVisible = false;

	float margin = 5.f;
	MFDisplay_GetDisplayRect(&window);
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
	pBuildUnits[0] = Button::Create(NULL, &button, &uvs, SelectUnit, this, 0);
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + 5.f;
	pBuildUnits[1] = Button::Create(NULL, &button, &uvs, SelectUnit, this, 1);
	button.x = units.x + 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[2] = Button::Create(NULL, &button, &uvs, SelectUnit, this, 2);
	button.x = units.x + units.width - 64.f - 5.f; button.y = units.y + units.height - 64.f - 5.f;
	pBuildUnits[3] = Button::Create(NULL, &button, &uvs, SelectUnit, this, 3);
}

CastleConfig::~CastleConfig()
{
	MFFont_Destroy(pFont);
}

void CastleConfig::Draw()
{
	if(!bVisible)
		return;

	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0, 0, 0, 0.8f));

	MFPrimitive_DrawUntexturedQuad(title.x, title.y, title.width, title.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(units.x, units.y, units.width, units.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(right.x, right.y, right.width, right.height, MakeVector(0, 0, 0, .8f));
	MFPrimitive_DrawUntexturedQuad(lower.x, lower.y, lower.width, lower.height, MakeVector(0, 0, 0, .8f));

	MFFont_BlitTextf(pFont, (int)title.x, (int)title.y, MFVector::yellow, pCastle->details.pName);

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

	MFMaterial *pDetailMap = pUnitDefs->GetUnitDetailMap();
	MFMaterial *pColourMap = pUnitDefs->GetUnitColourMap();

	for(int a=0; a<pCastle->details.numBuildUnits; ++a)
	{
		MFRect uvs;
		int unit = pCastle->details.buildUnits[a].unit;
		pUnitDefs->GetUnitUVs(pCastle->details.buildUnits[a].unit, false, &uvs);
		pBuildUnits[a]->SetImage(pDetailMap, &uvs);

		if(a == 2)
		{
			MFRect pos;
			pos.x = units.x;
			pos.y = units.y + units.height - 64.f - 5.f;
			pos.width = pos.height = 64.f;

			if(pCastle->details.numBuildUnits == 3)
			{
				float ratio = uvs.width/uvs.height;
				float width = 64.f * ratio;
				pos.x = MFFloor(units.x + units.width*0.5f - width*0.5f);
				pos.width = MFFloor(pos.width * ratio);
			}

			pBuildUnits[a]->SetPos(&pos);
		}

		pBuildUnits[a]->SetOutline(true, pCastle->building == a ? MFVector::blue : MFVector::white);
		pBuildUnits[a]->SetOverlay(pColourMap, pGame->GetPlayerColour(pCastle->player));

		pInputManager->PushReceiver(pBuildUnits[a]);
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

	for(int a=0; a<pCastle->details.numBuildUnits; ++a)
		pThis->pBuildUnits[a]->SetOutline(true, buttonID == a ? MFVector::blue : MFVector::white);

	pCastle->SetBuildUnit(buttonID);
}
