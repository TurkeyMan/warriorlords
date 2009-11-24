#include "Warlords.h"
#include "MapScreen.h"
#include "Unit.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"

#include "Path.h"

MapScreen::MapScreen(Game *_pGame)
{
	pGame = _pGame;
	pSelection = NULL;

	bMoving = false;

	pIcons = MFMaterial_Create("Icons");

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
		return false;

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
					// show group config screen
					//...

					// for the time being, deselect the group
					DeselectGroup(pSelection);
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
									Group *pGroup = Group::Create(-1);
									int numUnits = MFRand()%3 + 1;
									for(int a=0; a<numUnits; ++a)
										pGroup->AddUnit(pGame->GetUnitDefs()->CreateUnit(24 + (MFRand()&1), -1));

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

				// see if there's a castle on the square
				Castle *pCastle = pTile->GetCastle();
				if(pCastle)
				{
					// enter the castle config menu
					//...
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

			// validate we can move to the new square
			MapTile *pNewTile = pMap->GetTile(x, y);
			if(!pNewTile->CanMove(pSelection))
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
	}

	pEndTurn->Draw();
	pMiniMap->Draw();

	// now draw any UI that might be on the screen.

}

void MapScreen::Deselect()
{

}

void MapScreen::EndTurn(int button, void *pUserData, int buttonID)
{
	MapScreen *pThis = (MapScreen*)pUserData;
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

void MapScreen::DeselectGroup(Group *pGroup)
{
	if(pSelection)
		pSelection->bSelected = false;
	pSelection = NULL;
}

Group *MapScreen::GetSelected()
{
	return pSelection;
}
