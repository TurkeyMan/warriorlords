#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"
#include "Display.h"

#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFView.h"
#include "MFRenderer.h"

MapScreen::MapScreen(Game *_pGame)
{
	pGame = _pGame;
	pMap = pGame->GetMap();
	pSelection = NULL;
	pShowCastle = NULL;

	bMoving = false;
	bUndoVisible = false;

	pIcons = MFMaterial_Create("Icons");
	pFont = pGame->GetTextFont();
	pSmallNumbers = pGame->GetSmallNumbersFont();

	// buttons
	Tileset *pTiles = pMap->GetTileset();
	UnitDefinitions *pUnits = pGame->GetUnitDefs();

	MFMaterial *pTileMat = pTiles->GetTileMaterial();
	MFMaterial *pCastleMat = pUnits->GetCastleMaterial();
	MFMaterial *pRoadMat = pTiles->GetRoadMaterial();

	int tileWidth, tileHeight;
	pTiles->GetTileSize(&tileWidth, &tileHeight);

	MFRect uvs, pos = { 0, 0, (float)tileWidth, (float)tileHeight };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	// end turn button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = display.width - (16.f + (float)tileWidth);
	pos.y = display.height - (16.f + (float)tileHeight);
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pEndTurn = Button::Create(pIcons, &pos, &uvs, MFVector::one, EndTurn, this, 0, false);

	// minimap button
	pos.x = display.width - (16.f + (float)tileWidth);
	pos.y = 16.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pMiniMap = Button::Create(pIcons, &pos, &uvs, MFVector::one, ShowMiniMap, this, 0, false);

	// undo button
	pos.x = 16.f;
	pos.y = display.height - (16.f + (float)tileHeight);
	uvs.x = 0.f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pUndo = Button::Create(pIcons, &pos, &uvs, MFVector::one, UndoMove, this, 0, false);
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

	pCurrent = this;
	ShowUndoButton();

	lastUpdateTime = 0.f;
}

bool MapScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// only handle left clicks
	if(info.buttonID != 0)
	{
		if(info.buttonID == 1 && ev == IE_Tap && !bMoving)
			SelectGroup(NULL);
		return false;
	}

	switch(ev)
	{
		case IE_Tap:
		{
			// if a unit is moving, disable interaction
			if(bMoving)
				return true;

			// calculate the cursor position
			int cursorX, cursorY;
			float cx, cy;
			pMap->GetCursor(info.tap.x, info.tap.y, &cx, &cy);
			cursorX = (int)cx;
			cursorY = (int)cy;

			// get the tile
			MapTile *pTile = pMap->GetTile(cursorX, cursorY);

			if(pSelection && pGame->IsCurrentPlayer(pSelection->GetPlayer()))
			{
				// if we've already selected a group on this tile
				if(pTile == pSelection->pTile)
				{
					SelectGroup(NULL);

					// show group config screen
					groupConfig.Show(pTile);
					return true;
				}

				// get the selected groups tile
				pTile = pSelection->GetTile();

				// if the selected group has a path already planned
				if(pSelection->pPath)
				{
					Step *pStep = pSelection->pPath->GetLast();

					// first check for an attack command
					if(cursorX == pStep->x && cursorY == pStep->y)
					{
						if(pSelection->pPath->IsEnd() && pStep->CanMove())
						{
							const char *pMessage = NULL;
							bool bCommitActions = false;

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

								if(pSelection->GetNumUnits() > 0)
								{
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
												pGame->PushCaptureCastle(pSelection, pCastle);

												bCommitActions = true;
											}
										}
										else
										{
											// there must be empty enemy vehicles on the tile, we'll capture the empty vehicles
											for(int a=0; a<pTile->GetNumGroups(); ++a)
											{
												Group *pUnits = pTile->GetGroup(a);
												pGame->PushCaptureUnits(pSelection, pUnits);
												pUnits->SetPlayer(pSelection->GetPlayer());
											}

											bCommitActions = true;
										}
									}
									else
									{
										// begin the battle!
										pGame->BeginBattle(pSelection, pTile);
										break;
									}
								}
								else
								{
									// can't attack with an empty vehicle!
									// TODO: play sound?
									break;
								}
							}
							else if(pTile->GetType() == OT_Special)
							{
								// search command
								Unit *pHero = pSelection->GetHero();
								if(!pHero)
									break;

								// TODO: random encounter?

								// get an item
								Ruin *pRuin = pTile->GetRuin();
								if(!pRuin->bHasSearched)
								{
									pHero->AddItem(pRuin->item);
									pRuin->bHasSearched = true;

									pGame->PushSearch(pSelection, pRuin);

									Item *pItem = Game::GetCurrent()->GetUnitDefs()->GetItem(pRuin->item);
									pMessage = MFStr("You search the ruin and find\n%s", pItem->pName);
								}
								else
								{
									pMessage = "You search the ruin,\nbut it is empty!";
								}

								bCommitActions = true;
							}

							if(bCommitActions)
							{
								// move group to the square
								if(pGame->MoveGroupToTile(pSelection, pTile))
								{
									pSelection->pPath->Destroy();
									pSelection->pPath = NULL;
								}

								// commit the actions
								pGame->CommitActions(pSelection);

								if(pMessage)
									Game::GetCurrent()->ShowRequest(pMessage, NULL, true);
								break;
							}
						}

						// move to destination...
						bMoving = true;
						countdown = 0.f;

						// push the move to the action list
						pGame->PushMoveAction(pSelection);

						// if the undo wasn't previously visible, push it now.
						ShowUndoButton();
						break;
					}
				}

				// plot a path to the new location
				pSelection->pathX = cursorX;
				pSelection->pathY = cursorY;
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
					if(pCastle && pGame->IsCurrentPlayer(pCastle->GetPlayer()))
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

int MapScreen::Update()
{
	if(pShowCastle)
	{
		castleConfig.Show(pShowCastle);
		pShowCastle = NULL;
	}

	pGame->GetMap()->Update();

	if(bMoving)
	{
		countdown -= MFSystem_TimeDelta();

		while(countdown <= 0.f)
		{
			Step *pStep = pSelection->pPath->GetPath();
			int x = pStep->x;
			int y = pStep->y;

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

			// update the move action
			pGame->UpdateMoveAction(pSelection);

			// center the map on the guy moving
			pMap->CenterView((float)x, (float)y);

			countdown += 0.15f;

			// strip the step from the path
			pStep = pSelection->pPath->StripStep(pStep);

			// if we have reached our destination
			if(!pStep)
			{
				pSelection->pPath = NULL;
				bMoving = false;
				break;
			}
		}
	}

	if(pGame->IsOnline() && !pGame->IsMyTurn() && pGame->NumPendingActions() == 0)
	{
		lastUpdateTime += MFSystem_TimeDelta();
		if(lastUpdateTime > 10.f)
		{
			pGame->UpdateGameState();
			pGame->ReplayActions();
			lastUpdateTime -= 10.f;
		}
	}

#if defined(_DEBUG)
	int w, h;
	pMap->GetMapSize(&w, &h);
	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			MapTile *pTile = pMap->GetTile(x, y);

			int numGroups = pTile->GetNumGroups();
			for(int g=0; g<numGroups; ++g)
			{
				Group *pGroup = pTile->GetGroup(g);
				MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");
			}
		}
	}
#endif

	return 0;
}

void MapScreen::Draw()
{
	pMap->Draw();

	if(pSelection)
	{
		// render the path
		if(pGame->IsCurrentPlayer(pSelection->GetPlayer()))
		{
			Path *pPath = pSelection->GetPath();
			if(pPath && pPath->GetPathLength())
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
				MFMaterial_SetMaterial(pIcons);

				int len = pPath->GetPathLength();
				Step *pStep = pPath->GetPath();

				for(int p = 0; p < len; ++p)
				{
					float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;
					if(pStep[p].x >= xS && pStep[p].y >= yS && pStep[p].x < xTiles && pStep[p].y < yTiles)
					{
						float tx = (float)(pStep[p].icon & 7) / 8.f + texelCenterOffset;
						float ty = (float)(4 + (pStep[p].icon >> 3)) / 8.f + texelCenterOffset;
						MFPrimitive_DrawQuad((float)pStep[p].x + 0.25f, (float)pStep[p].y + 0.25f, 0.5f, 0.5f, MFVector::one, tx, ty, tx + 0.125f, ty+0.125f);
					}
				}

				for(int p = 0; p < len; ++p)
				{
					if(pStep[p].cost > 2 && pStep[p].CanMove())
						MFFont_DrawTextf(pSmallNumbers, pStep[p].x + 0.61f, pStep[p].y + 0.61f, 0.27f, MFVector::yellow, "%g", (float)pStep[p].cost * 0.5f);
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

		if(bUndoVisible)
			pUndo->Draw();
	}

	pEndTurn->Draw();
	pMiniMap->Draw();

	// now draw any UI that might be on the screen.
	groupConfig.Draw();
	castleConfig.Draw();
	miniMap.Draw();

	pGame->DrawRequest();

	if(pGame->NumPendingActions() > 0)
		MFFont_BlitTextf(pFont, 100, 50, MakeVector(1,1,1,1), pGame->GetNextActionDesc());
}

void MapScreen::Deselect()
{
	pInputManager->PopReceiver(this);
	bUndoVisible = false;
}

void MapScreen::EndTurn(int button, void *pUserData, int buttonID)
{
	Game *pGame = Game::GetCurrent();

	if(pGame->NumPendingActions() > 0)
		pGame->ReplayNextAction();
	else
		pGame->ShowRequest("End Turn?", FinishTurn, false, pUserData);
}

void MapScreen::FinishTurn(int selection, void *pUserData)
{
	if(selection == 0)
	{
		MapScreen *pMapScreen = (MapScreen*)pUserData;
		pMapScreen->SelectGroup(NULL);
		pMapScreen->pGame->EndTurn();
	}
}

void MapScreen::ShowMiniMap(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;
	pThis->miniMap.Show(Game::GetCurrent()->GetMap());
}

void MapScreen::UndoMove(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;

	Group *pGroup = pThis->pGame->RevertAction(pThis->pSelection);
	if(pGroup)
	{
		MapTile *pTile = pGroup->GetTile();
		pThis->pMap->CenterView((float)pTile->GetX(), (float)pTile->GetY());

		pThis->SelectGroup(pGroup);
	}
}

void MapScreen::SelectGroup(Group *pGroup)
{
	if(pSelection == pGroup)
		return;

	if(pGroup)
	{
		pSelection = pGroup;
		pSelection->bSelected = true;
		pSelection->pPath = NULL;

		if(pGroup->pathX != -1 && pGroup->pathY != -1)
			pSelection->pPath = pMap->FindPath(pGroup, pGroup->pathX, pGroup->pathY);
	}
	else
	{
		pSelection->bSelected = false;

		if(pSelection->pPath)
		{
			pSelection->pPath->Destroy();
			pSelection->pPath = NULL;
		}

		pSelection = NULL;
	}

	ShowUndoButton();
}

Group *MapScreen::GetSelected()
{
	return pSelection;
}

void MapScreen::ShowUndoButton()
{
	if(Screen::GetCurrent() != this)
		return;

	if(pSelection && pSelection->GetLastAction())
	{
		if(!bUndoVisible)
		{
			pInputManager->PushReceiver(pUndo);
			bUndoVisible = true;
		}
	}
	else
	{
		if(bUndoVisible)
		{
			pInputManager->PopReceiver(pUndo);
			bUndoVisible = false;
		}
	}
}
