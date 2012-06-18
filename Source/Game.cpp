#include "Warlords.h"
#include "Game.h"

#include "Menu/Game/GameUI.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFRenderer.h"
#include "MFView.h"

#include <stdarg.h>

Game *Game::pCurrent = NULL;

Game::Game(GameParams *pParams)
{
	bOnline = pParams->bOnline;
	gameID = pParams->gameID;

	pActionList = bOnline ? new ActionList(gameID) : NULL;

	Init(pParams->pMap, pParams->bEditMap);

	lastAction = 0;

	currentPlayer = 0;
	currentTurn = 0;

	if(pParams->bEditMap)
	{
		// init the players
		int numRaces = pUnitDefs->GetNumRaces() - 1;
		for(int a=0; a<numRaces; ++a)
		{
			players[a].race = a + 1;
			players[a].colour = pUnitDefs->GetRaceColour(a + 1);
		}

		// construct the map
		pMap->ConstructMap(0);
	}
	else
	{
		// setup players
		for(int a=0; a<pParams->numPlayers; ++a)
		{
			players[a].playerID = pParams->players[a].id;
			players[a].race = pParams->players[a].race;
			players[a].colour = pUnitDefs->GetRaceColour(pParams->players[a].colour);
			players[a].startingHero = pParams->players[a].hero;
			players[a].cursorX = 0;
			players[a].cursorY = 0;
			players[a].numHeroes = 0;
			for(int i=0; i<4; ++i)
			{
				players[a].pHero[i] = NULL;
				players[a].pHeroReviveLocation[i] = NULL;
			}
		}

		// construct the map
		pMap->ConstructMap();
	}

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(this, &Game::ReceivePeerMessage));
}

Game::Game(GameState *pState)
{
	bOnline = true;
	gameID = pState->id;

	pActionList = new ActionList(pState->id);

	Init(pState->map, false);

	// TODO: we should try loading game state and pending actions from disk...
	currentPlayer = 0;
	currentTurn = 0;

	// setup players
	for(int a=0; a<pState->numPlayers; ++a)
	{
		players[a].playerID = pState->players[a].id;

		players[a].race = pState->players[a].race;
		players[a].colour = pUnitDefs->GetRaceColour(pState->players[a].colour);
		players[a].startingHero = pState->players[a].hero;

		// TODO: we need to fetch these from the server!
		players[a].cursorX = 0;
		players[a].cursorY = 0;
		players[a].numHeroes = 0;
		for(int i=0; i<4; ++i)
		{
			players[a].pHero[i] = NULL;
			players[a].pHeroReviveLocation[i] = NULL;
		}

	}

	// construct the map
	pMap->ConstructMap();

	// update the game state
	lastAction = 0;

	// resume the game
	Screen::SetNext(pMapScreen);
	pGameUI->Show();

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(this, &Game::ReceivePeerMessage));
}

void Game::Init(const char *pMapName, bool bEdit)
{
	// init fields
	pSelection = NULL;
	bMoving = false;
	countdown = 0.f;

	// create the GameUI
	pGameUI = new GameUI(this);

	pText = MFFont_Create("FranklinGothic");
	pBattleNumbersFont = MFFont_Create("Battle");
	pSmallNumbersFont = MFFont_Create("SmallNumbers");

	pIcons = MFMaterial_Create("Icons");

	// ** REMOVE ME ***
	MFTexture *pTemp = MFTexture_Create("Horiz-Pirates", false);
	pWindow = MFMaterial_Create("Window-Pirates");
	pHorizLine = MFMaterial_Create("Horiz-Pirates");
	MFTexture_Destroy(pTemp);

	SetCurrent(this);

	// create the map and screens
	pMap = Map::Create(this, pMapName, bEdit);
	pUnitDefs = pMap->GetUnitDefinitions();

	pMapScreen = new MapScreen(this);
	pBattle = new Battle(this);

	// allocate runtime data
	const int numSizes = 4;
	const int numItems[numSizes] = { 1024, 256, 64, 4 };
	const size_t sizes[numSizes] = { sizeof(Action*)*1, sizeof(Action*)*6, sizeof(Action*)*MapTile::MaxUnitsOnTile * 2, sizeof(Action*)*256 };
	actionList.Init(numSizes, numItems, sizes);

	actionCache.Init(1024, sizeof(Action), 256);

	units.Init(256, sizeof(Unit), 256);
	groups.Init(128, sizeof(Group), 64);

	numUnitsAllocated = 256;
	ppUnits = (Unit**)MFHeap_Alloc(sizeof(Unit*) * numUnitsAllocated);
	numUnits = 0;

	numGroupsAllocated = 256;
	ppGroups = (Group**)MFHeap_Alloc(sizeof(Group*) * numGroupsAllocated);
	numGroups = 0;

	ppActionHistory = NULL;
	numTopActions = 0;

	bUpdating = false;
}

Game::~Game()
{
	if(pBattle)
		delete pBattle;
	if(pMapScreen)
		delete pMapScreen;

	if(pMap)
		pMap->Destroy();

	MFMaterial_Destroy(pIcons);

	MFFont_Destroy(pText);
	MFFont_Destroy(pBattleNumbersFont);
	MFFont_Destroy(pSmallNumbersFont);

	actionCache.Destroy();
	actionList.Destroy();

	if(pActionList)
		delete pActionList;

	delete pGameUI;
}

void Game::Update()
{
	// update UI
	pGameUI->Update();

	// update map
	pMap->Update();

	// are we moving things around?
	if(bMoving)
	{
		countdown -= MFSystem_TimeDelta();

		while(countdown <= 0.f)
		{
			Step *pStep = pSelection->pPath->GetPath();
			if(pStep->InvalidMove())
			{
				bMoving = false;
				break;
			}

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
			UpdateMoveAction(pSelection);

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

	// update game state
	if(pActionList && !IsMyTurn())
		pActionList->Update();

	if(NumPendingActions() > 0)
		ReplayActions();
//		ReplayActions(303);
}

void Game::Draw()
{
	pMap->Draw();

	if(pSelection)
	{
		// render the path
		if(IsCurrentPlayer(pSelection->GetPlayer()))
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
					if(pStep[p].InvalidMove())
						break;

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
					if(pStep[p].InvalidMove())
						break;

					if(pStep[p].cost > 2 && pStep[p].CanMove())
						MFFont_DrawText2f(pSmallNumbersFont, pStep[p].x + 0.61f, pStep[p].y + 0.61f, 0.27f, MFVector::yellow, "%g", (float)pStep[p].cost * 0.5f);
				}

				MFView_Pop();
			}
		}

		// draw the group info
		int numUnits = pSelection->GetNumUnits();
		int tx = 42 + numUnits*32;
		int ty = 37 - (int)MFFont_GetFontHeight(pText)/2;

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

		float movement = pSelection->MoveRemaining();
		MFFont_BlitTextf(pText, tx+1, ty+1, MakeVector(0,0,0,1), "Move: %g", movement);
		MFFont_BlitTextf(pText, tx, ty, MakeVector(1,1,0,1), "Move: %g", movement);
	}

	// now draw any UI that might be on the screen.
	groupConfig.Draw();

	if(NumPendingActions() > 0)
		MFFont_BlitTextf(pText, 100, 50, MakeVector(1,1,1,1), GetNextActionDesc());
}

void Game::SetInputSource(HKWidget *pWidget)
{
	pWidget->RegisterInputEventHook(MakeDelegate(this, &Game::HandleInputEvent));
}

bool Game::HandleInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev)
{
	if(ev.pSource->device == IDD_Mouse && ev.pSource->deviceID != 0)
		return false;

	// only handle left clicks
	if(ev.buttonID != 0)
	{
		if(ev.buttonID == 1 && ev.ev == HKInputManager::IE_Tap && !bMoving)
			SelectGroup(NULL);
		return false;
	}

	switch(ev.ev)
	{
		case HKInputManager::IE_Tap:
		{
			// if a unit is moving, disable interaction
			if(bMoving)
				return true;

			// calculate the cursor position
			int cursorX, cursorY;
			float cx, cy;
			pMap->GetCursor(ev.tap.x, ev.tap.y, &cx, &cy);
			cursorX = (int)cx;
			cursorY = (int)cy;

			// get the tile
			MapTile *pTile = pMap->GetTile(cursorX, cursorY);

			if(pSelection && IsCurrentPlayer(pSelection->GetPlayer()))
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

					// if we commit to move
					if(cursorX == pStep->x && cursorY == pStep->y)
					{
						// check we can move at all
						Step *pFirst = pSelection->pPath->GetPath();
						if(!pFirst->CanMove() || pFirst->InvalidMove())
							break;

						// first check for an attack command
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
												BeginBattle(pSelection, pTile);
												break;
											}
											else
											{
												// the castle is empty! claim that shit!
												pCastle->Capture(pSelection);
												PushCaptureCastle(pSelection, pCastle);

												// show the catle config window
												pGameUI->ShowCastleMenu(pCastle);

												bCommitActions = true;
											}
										}
										else
										{
											// there must be empty enemy vehicles on the tile, we'll capture the empty vehicles
											for(int a=0; a<pTile->GetNumGroups(); ++a)
											{
												Group *pUnits = pTile->GetGroup(a);
												PushCaptureUnits(pSelection, pUnits);
												pUnits->SetPlayer(pSelection->GetPlayer());
											}

											bCommitActions = true;
										}
									}
									else
									{
										// begin the battle!
										BeginBattle(pSelection, pTile);
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
							else if(pTile->GetType() == OT_Place)
							{
								Place *pPlace = pTile->GetPlace();
								Special::Type type = pPlace->GetType();

								switch(type)
								{
									case Special::ST_Searchable:
									{
										// search command
										Unit *pHero = pSelection->GetHero();
										if(!pHero)
											break;

										// TODO: random encounter?

										// get an item
										if(!pPlace->ruin.bHasSearched)
										{
											pHero->AddItem(pPlace->ruin.item);
											pPlace->ruin.bHasSearched = true;

											PushSearch(pSelection, pPlace);

											Item *pItem = Game::GetCurrent()->GetUnitDefs()->GetItem(pPlace->ruin.item);
											pMessage = MFStr("You search the ruin and find\n%s", pItem->pName);
										}
										else
										{
											pMessage = "You search the ruin,\nbut it is empty!";
										}

										bCommitActions = true;
										break;
									}
									case Special::ST_Recruit:
									{
										// recruit hero
										Unit *pHero = pSelection->GetHero();
										if(!pHero)
											break;

										pGameUI->ShowRecruitMenu(pPlace, pHero);

										bCommitActions = true;
										break;
									}
								}
							}

							if(bCommitActions)
							{
								// move group to the square
								if(MoveGroupToTile(pSelection, pTile))
								{
									pSelection->pPath->Destroy();
									pSelection->pPath = NULL;
								}

								// commit the actions
								CommitActions(pSelection);

								if(pMessage)
									Game::GetCurrent()->ShowRequest(pMessage, NULL, true);
								break;
							}
						}

						// move to destination...
						bMoving = true;
						countdown = 0.f;

						// push the move to the action list
						PushMoveAction(pSelection);

						// if the undo wasn't previously visible, push it now.
						UpdateUndoButton();
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
					if(pCastle && IsCurrentPlayer(pCastle->GetPlayer()))
					{
						// enter the castle config menu
						pGameUI->ShowCastleMenu(pCastle);
					}
				}
			}
			break;
		}
	}

	return pMap->ReceiveInputEvent(manager, ev);
}

bool Game::IsCurrentPlayer(int player)
{
	if(player == -1)
		return false;

	if(!bOnline)
		return player == currentPlayer;

	Session *pSession = Session::Get();
	if(!pSession || !pSession->IsLoggedIn())
		return false;

	return player == currentPlayer && players[player].playerID == pSession->GetUserID();
}

void Game::BeginGame()
{
	// inform the server of the starting state...
	if(bOnline)
	{
		int numCastles = pMap->GetNumCastles();
		if(numCastles)
		{
			GameAction *pAction = SubmitAction(GA_ADDCASTLES, numCastles);

			for(int a=0; a<numCastles; ++a)
			{
				Castle *pCastle = pMap->GetCastle(a);
				pAction->pArgs[a] = pCastle->GetPlayer();
			}
		}

		int numPlaces = pMap->GetNumPlaces();
		if(numPlaces)
		{
			GameAction *pAction = SubmitAction(GA_ADDRUINS, numPlaces);

			for(int a=0; a<numPlaces; ++a)
			{
				Place *pPlace = pMap->GetPlace(a);
				pAction->pArgs[a] = pPlace->GetType() == Special::ST_Searchable ? pPlace->ruin.item : 0;
			}
		}
	}

	// create starting units
	for(int a=0; a<pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player != -1)
		{
			// set each players map focus on starting castle
			players[pCastle->player].cursorX = pCastle->details.x;
			players[pCastle->player].cursorY = pCastle->details.y;

			// produce starting hero
			int hero = players[pCastle->player].startingHero;
			int heroID = -1;

			int numUnits = pUnitDefs->GetNumUnitTypes();
			for(int u=0; u<numUnits; ++u)
			{
				UnitDetails *pUnit = pUnitDefs->GetUnitDetails(u);
				if(pUnit->type == UT_Hero && (pUnit->race == players[pCastle->player].race || pUnit->race == 0))
				{
					if(hero-- == 0)
					{
						heroID = u;
						break;
					}
				}
			}

			MFDebug_Assert(heroID != -1, "Couldn't find hero!");

			Group *pGroup = CreateUnit(heroID, pCastle, NULL, true);
			players[pCastle->player].pHero[players[pCastle->player].numHeroes++] = pGroup->GetUnit(0);

			// pirates also start with a galleon
			int pirateID = pUnitDefs->FindRace("Pirates");
			int galleonID = pUnitDefs->FindUnit("Galleon");
			if(players[pCastle->player].race == pirateID && galleonID >= 0)
				CreateUnit(galleonID, pCastle, NULL, true);
		}
	}

	// begin the first turn
	BeginTurn(0);

	// send the actions to the server
	ApplyActions();

	Screen::SetNext(pMapScreen);
	pGameUI->Show();
}

void Game::BeginTurn(int player)
{
	if(player < currentPlayer)
		++currentTurn;

	currentPlayer = player;

	// heal units and restore movement
	int width, height;
	pMap->GetMapSize(&width, &height);

	for(int y=0; y<height; ++y)
	{
		for(int x=0; x<width; ++x)
		{
			MapTile *pTile = pMap->GetTile(x, y);

			int numGroups = pTile->GetNumGroups();
			if(numGroups && pTile->IsFriendlyTile(player))
			{
				for(int a=0; a<numGroups; ++a)
				{
					Group *pGroup = pTile->GetGroup(a);

					Unit *pVehicle = pGroup->GetVehicle();
					if(pVehicle)
						pVehicle->Restore();

					int numUnits = pGroup->GetNumUnits();
					for(int b=0; b<numUnits; ++b)
					{
						Unit *pUnit = pGroup->GetUnit(b);
						pUnit->Restore();
					}
				}
			}
		}
	}

	// count castles, tax the land
	int numCastles = 0;
	int numOpponentCastles = 0;

	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			// show the starting castle build screen
			if(currentTurn == 0 && IsMyTurn())
				pGameUI->ShowCastleMenu(pCastle);

			++numCastles;

			if(pCastle->building != -1)
				--pCastle->buildTime;
		}
		else if(pCastle->player != -1)
		{
			++numOpponentCastles;
		}
	}

	// resurrect heroes
	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			if(pCastle->building != -1 && pCastle->buildTime <= 0)
			{
				// if a hero is building here
				if(pCastle->building >= 4)
				{
					Group *pGroup = NULL;

					Unit *pHero = players[currentPlayer].pHero[pCastle->building - 4];
					if(!bOnline || (IsMyTurn() && NumPendingActions() <= 1))
					{
						pGroup = CreateUnit(pHero->GetType(), pCastle, NULL, true);
						if(pGroup)
							pGameUI->ShowCastleMenu(pCastle);
					}
					else
					{
						pHero->Revive();
						pCastle->ClearBuildUnit();
					}
				}
			}
		}
	}

	// recruit heroes
	for(int a = 0; a < pMap->GetNumPlaces(); ++a)
	{
		Place *pPlace = pMap->GetPlace(a);

		if(pPlace->GetType() != Special::ST_Recruit || pPlace->recruit.recruiting < 0)
			continue;

		if(pPlace->recruit.pRecruitHero->GetPlayer() != currentPlayer)
			continue;

		if(pPlace->recruit.pRecruitHero->IsDead())
		{
			pPlace->recruit.pRecruitHero = NULL;
			pPlace->recruit.recruiting = -1;
			continue;
		}

		MapTile *pTile = pPlace->recruit.pRecruitHero->GetGroup()->GetTile();
		if(pTile->GetPlace() == pPlace)
		{
			--pPlace->recruit.turnsRemaining;

			if(pPlace->recruit.turnsRemaining == 0)
			{
				// build hero
				CreateUnit(pPlace->recruit.recruiting, NULL, pPlace, true);

				pPlace->recruit.pRecruitHero = NULL;
				pPlace->recruit.recruiting = -1;
			}
		}
	}

	// build units
	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			if(pCastle->building != -1 && pCastle->buildTime <= 0)
			{
				if(pCastle->building < 4)
				{
					BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

					Group *pGroup = NULL;

					if(!bOnline || (IsMyTurn() && NumPendingActions() <= 1))
						pGroup = CreateUnit(buildUnit.unit, pCastle, NULL, true);

					if(pGroup || bOnline)
					{
						pCastle->buildTime = pCastle->GetUnitBuildTime();
					}
				}
			}
		}
	}

	// if the player has no castles left, go to next player
	if(numCastles == 0)
	{
		int numPlayers = 6;
		BeginTurn((currentPlayer + 1) % numPlayers);
		return;
	}
	else if(numOpponentCastles == 0)
	{
		if(bOnline)
		{
			// victory!!
			SubmitActionArgs(GA_VICTORY, 1, currentPlayer);
		}

		// show victory prompt
		//...
	}

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
	pMap->CenterView((float)players[player].cursorX, (float)players[player].cursorY);

	// commit all outstanding actions
	CommitAllActions();
}

void Game::EndTurn()
{
	// deselect the group
	SelectGroup(NULL);

	// commit all outstanding actions
	CommitAllActions();

	// change build units
	int numCastles = pMap->GetNumCastles();
	for(int a=0; a<numCastles; ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->nextBuild != pCastle->building)
		{
			pCastle->buildTime = pCastle->BuildTimeRemaining();
			pCastle->building = pCastle->nextBuild;

			if(bOnline && !bUpdating && IsMyTurn())
			{
				SubmitActionArgs(GA_SETBUILDING, 3, pCastle->id, pCastle->building, pCastle->buildTime);
			}
		}
	}

	// end the turn
	if(bOnline && !bUpdating && IsMyTurn())
	{
		SubmitActionArgs(GA_ENDTURN, 1, currentPlayer);
	}

	// begin the next players turn
	int numPlayers = 6;
	BeginTurn((currentPlayer + 1) % numPlayers);
}

void Game::BeginBattle(Group *pGroup, MapTile *pTarget)
{
	CommitActions(pGroup);

	// enter the battle
	SelectGroup(NULL);
	pBattle->Begin(pGroup, pTarget);
}

void Game::EndBattle(Group *pGroup, MapTile *pTarget)
{
	pBattle->End();

	// destroy dead units
	int battleArgs[32];
	int numArgs = 0;

	battleArgs[numArgs++] = pGroup->GetID();

	for(int a=0; a<pGroup->GetNumUnits(); ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);

		if(bOnline)
		{
			battleArgs[numArgs++] = pUnit->GetHP();
			battleArgs[numArgs++] = pUnit->GetKills();
		}

		if(pUnit->IsDead())
		{
			pGroup->RemoveUnit(pUnit);
			if(!pUnit->IsHero())
				pUnit->Destroy();
			--a;
		}
	}

	if(bOnline)
	{
		GameAction *pNew = SubmitAction(GA_BATTLE, numArgs);
		for(int a=0; a<numArgs; ++a)
			pNew->pArgs[a] = battleArgs[a];
	}

	// if all units in the group were destroyed
	MapTile *pCurrentTile = pGroup->GetTile();
	if(pGroup->GetNumUnits() == 0)
	{
		pCurrentTile->RemoveGroup(pGroup);
		pGroup->Destroy();
		pGroup = NULL;
	}

	// remove units from all groups on the target tile
	for(int a=0; a<pTarget->GetNumGroups();)
	{
		Group *pG = pTarget->GetGroup(a);
		bool bIsMercGroup = pG->GetPlayer() == -1;

		numArgs = 0;
		battleArgs[numArgs++] = pG->GetID();

		for(int b=0; b<pG->GetNumUnits(); ++b)
		{
			Unit *pUnit = pG->GetUnit(b);

			if(bOnline && !bIsMercGroup)
			{
				battleArgs[numArgs++] = pUnit->GetHP();
				battleArgs[numArgs++] = pUnit->GetKills();
			}

			if(pUnit->IsDead() || bIsMercGroup)
			{
				pG->RemoveUnit(pUnit);
				if(!pUnit->IsHero())
					pUnit->Destroy();
				--b;
			}
		}

		if(bOnline && !bIsMercGroup)
		{
			GameAction *pNew = SubmitAction(GA_BATTLE, numArgs);
			for(int a=0; a<numArgs; ++a)
				pNew->pArgs[a] = battleArgs[a];
		}

		if(pG->GetNumUnits() == 0)
		{
			pTarget->RemoveGroup(pG);
			pG->Destroy();
		}
		else
			++a;
	}

	if(pGroup)
	{
		// select the victorious group
		SelectGroup(pGroup);

		// if the target is an empty castle, capture it
		Castle *pCastle = pTarget->GetCastle();
		if(pCastle && pCastle->IsEmpty())
		{
			pCastle->Capture(pGroup);
			PushCaptureCastle(pGroup, pCastle);

			pGameUI->ShowCastleMenu(pCastle);
		}

		// check if the target is clear, and move there
		if(pTarget->CanMove(pGroup))
			MoveGroupToTile(pGroup, pTarget);

		pGroup->pPath->Destroy();
		pGroup->pPath = NULL;

		// commit all battle actions
		CommitActions(pGroup);
	}
	else
	{
		// since the group is dead, explicitly apply the battle actions
		ApplyActions();
	}

	Screen::SetNext(pMapScreen);
	pGameUI->Show();
}

Unit *Game::AllocUnit()
{
	return (Unit*)units.Alloc();
}

void Game::DestroyUnit(Unit *pUnit)
{
	units.Delete(pUnit);
}

Group *Game::AllocGroup()
{
	return (Group*)groups.Alloc();
}

void Game::DestroyGroup(Group *pGroup)
{
	groups.Delete(pGroup);
}

bool Game::PlayerHasHero(int player, int hero)
{
	for(int a=0; a<players[player].numHeroes; ++a)
	{
		if(players[player].pHero[a]->GetType() == hero)
			return true;
	}
	return false;
}

bool Game::CanCastleBuildHero(Castle *pCastle, int hero)
{
	Player &p = players[pCastle->player];
	if(!p.pHero[hero] || !p.pHero[hero]->IsDead())
		return false;
	return p.pHeroReviveLocation[hero] == NULL || p.pHeroReviveLocation[hero] == pCastle;
}

void Game::SetHeroRebuildCastle(int player, int hero, Castle *pCastle)
{
	players[player].pHeroReviveLocation[hero] = pCastle;
}

Group *Game::CreateUnit(int unit, Castle *pCastle, Place *pPlace,  bool bCommitUnit)
{
	// find space in the castle for the unit
	MapTile *pTile = NULL;

	int x, y;
	int player;

	if(pCastle)
	{
		x = pCastle->details.x;
		y = pCastle->details.y;
		player = pCastle->player;
	}
	if(pPlace)
	{
		x = pPlace->pTile->GetX();
		y = pPlace->pTile->GetY();
		player = pPlace->recruit.pRecruitHero->GetPlayer();
	}

	UnitDetails *pDetails = pUnitDefs->GetUnitDetails(unit);
	uint32 castleTerrain = pMap->GetTerrainAt(x, y);

	int movePenalty = pUnitDefs->GetMovementPenalty(pDetails->movementClass, castleTerrain & 0xFF);
	if(movePenalty == 0 || movePenalty >= 100)
	{
		// the unit can't go on the castle, it must be a boat or something
		// find a patch of terrain around the castle where it can begin
		const int searchTable[12] = { 4, 8, 7, 11, 13, 14, 1, 2, 12, 15, 0, 3 };

		int width, height;
		pMap->GetMapSize(&width, &height);

		for(int i = 0; i < 12; ++i)
		{
			int tx = x - 1 + (searchTable[i] & 3);
			int ty = y - 1 + (searchTable[i] >> 2);

			if(tx < 0 || ty < 0 || tx >= width || ty >= height)
				continue;

			MapTile *pT = pMap->GetTile(tx, ty);

			if(pT->IsEnemyTile(player))
				continue;

			uint32 terrain = pT->GetTerrain();
			for(int j=0; j<4; ++j)
			{
				if(pUnitDefs->GetMovementPenalty(pDetails->movementClass, terrain & 0xFF) != 0)
				{
					if((pDetails->type == UT_Vehicle && pT->GetNumGroups() < 10) || pT->GetNumUnits() < 10)
					{
						if(!pTile || pT->GetType() == OT_Road)
							pTile = pT;
						break;
					}
				}
				terrain >>= 8;
			}
		}
	}
	else
	{
		for(int i = 0; i < 4; ++i)
		{
			MapTile *pT = pMap->GetTile(x + (i & 1), y + (i >> 1));
			int numUnits = pT->GetNumUnits();
			if(numUnits < 10)
			{
				pTile = pT;
				break;
			}
		}
	}

	if(pTile)
	{
		// create the unit
		Unit *pUnit = NULL;
		if(pDetails->type == UT_Hero)
		{
			for(int a=0; a<players[player].numHeroes; ++a)
			{
				if(players[player].pHero[a]->GetType() == unit)
				{
					pUnit = players[player].pHero[a];
					pUnit->Revive();

					pCastle->ClearBuildUnit();
					break;
				}
			}
		}

		if(!pUnit)
		{
			pUnit = pUnitDefs->CreateUnit(unit, player);
			AddUnit(pUnit, bCommitUnit);

			if(pUnit->IsHero())
				players[player].pHero[players[player].numHeroes++] = pUnit;
		}

		// create a group for the unit, and add it to the tile
		Group *pGroup = Group::Create(player);
		pGroup->AddUnit(pUnit);
		pTile->AddGroup(pGroup);

		// HACK: Skeletons build 2 at a time!
		int skeletonID = pUnitDefs->FindUnit("Skeleton");
		if(unit == skeletonID)
		{
			if(pGroup->GetTile()->GetNumUnits() < 10)
			{
				// if there's space on the same tile, add the second one to the same group
				Unit *pUnit2 = pUnitDefs->CreateUnit(unit, player);
				AddUnit(pUnit2, bCommitUnit);
				pGroup->AddUnit(pUnit2);
			}
			else
			{
				// find space on a new tile
				for(int i = 0; i < 4; ++i)
				{
					MapTile *pT = pMap->GetTile(x + (i & 1), y + (i >> 1));
					int numUnits = pT->GetNumUnits();
					if(numUnits < 10)
					{
						// create the second on the new tile
						Unit *pUnit2 = pUnitDefs->CreateUnit(unit, player);
						AddUnit(pUnit2, bCommitUnit);

						// create a group for the unit, and add it to the tile
						Group *pGroup2 = Group::Create(player);
						pGroup2->AddUnit(pUnit2);
						pT->AddGroup(pGroup2);
						AddGroup(pGroup2, bCommitUnit);
						break;
					}
				}
			}
		}

		// add the group to the manager
		AddGroup(pGroup, bCommitUnit);

		return pGroup;
	}

	return NULL;
}

bool Game::MoveGroupToTile(Group *pGroup, MapTile *pTile)
{
	// check movement costs
//	if(not enough movement)
//		return false;

	// Check there is room on the target square
	if(pGroup->GetNumUnits() > pTile->GetAvailableUnitSpace())
		return false;

	PushMoveAction(pGroup);
	MapTile *pCurrent = pGroup->GetTile();
	if(pCurrent)
		pCurrent->RemoveGroup(pGroup);
	pTile->AddGroup(pGroup);

	// subtract movement costs?
	//...

	UpdateMoveAction(pGroup);

	// claim nearby flags
	pMap->ClaimFlags(pTile->GetX(), pTile->GetY(), pGroup->GetPlayer());

	return true;
}

void Game::SelectGroup(Group *pGroup)
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

	UpdateUndoButton();
}

void Game::AddUnit(Unit *pUnit, bool bCommitUnit)
{
	if(numUnits >= numUnitsAllocated)
	{
		numUnitsAllocated *= 2;
		ppUnits = (Unit**)MFHeap_Realloc(ppUnits, sizeof(Unit*) * numUnitsAllocated);
	}

	pUnit->SetID(numUnits);
	ppUnits[numUnits++] = pUnit;

	// notify the server of the new unit
	if(bOnline && bCommitUnit)
	{
		SubmitActionArgs(GA_CREATEUNIT, 4, pUnit->GetPlayer(), pUnit->GetType(), pUnit->GetMaxMovement(), pUnit->GetMaxHP());
	}
}

void Game::AddGroup(Group *pGroup, bool bCommitGroup)
{
	if(numGroups >= numGroupsAllocated)
	{
		numGroupsAllocated *= 2;
		ppGroups = (Group**)MFHeap_Realloc(ppGroups, sizeof(Group*) * numGroupsAllocated);
	}

	pGroup->SetID(numGroups);
	ppGroups[numGroups++] = pGroup;

	// notify the server of the new group
	if(bOnline && (!bUpdating || bCommitGroup))
	{
		GameAction *pNew = SubmitAction(GA_CREATEGROUP, 14);
		pNew->pArgs[0] = pGroup->GetPlayer();
		pNew->pArgs[1] = pGroup->x;
		pNew->pArgs[2] = pGroup->y;

		// add front units
		int forwardUnits = pGroup->GetNumForwardUnits();
		for(int a=0; a<5; ++a)
		{
			Unit *pUnit = NULL;
			if(a < forwardUnits)
				pUnit = pGroup->GetForwardUnit(a);
			pNew->pArgs[3 + a] = pUnit ? pUnit->GetID() : -1;
		}

		// add rear units
		int rearUnits = pGroup->GetNumRearUnits();
		for(int a=0; a<5; ++a)
		{
			Unit *pUnit = NULL;
			if(a < rearUnits)
				pUnit = pGroup->GetRearUnit(a);
			pNew->pArgs[8 + a] = pUnit ? pUnit->GetID() : -1;
		}

		// add vehicle
		Unit *pVehicle = pGroup->GetVehicle();
		pNew->pArgs[13] = pVehicle ? pVehicle->GetID() : -1;
	}
}

void Game::DrawWindow(const MFRect &rect)
{
	MFMaterial_SetMaterial(pWindow);

	float texelCenterX = MFRenderer_GetTexelCenterOffset() * (1.f/480.f);
	float texelCenterY = MFRenderer_GetTexelCenterOffset() * (1.f/320.f);

	MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height, MFVector::one, texelCenterX, texelCenterY, 1.f+texelCenterX, 1.f+texelCenterX);
}

void Game::DrawLine(float sx, float sy, float dx, float dy)
{
	MFMaterial_SetMaterial(pHorizLine);

	float texelCenter = MFRenderer_GetTexelCenterOffset() * (1.f/16.f);

	if(sx == dx)
	{
		MFPrimitive(PT_TriStrip | PT_Prelit);
		MFBegin(4);

		MFSetTexCoord1(0, 1.f+texelCenter);
		MFSetPosition(sx-8.f, sy, 0);

		MFSetTexCoord1(0, texelCenter);
		MFSetPosition(sx+8.f, sy, 0);

		MFSetTexCoord1(1, 1.f+texelCenter);
		MFSetPosition(sx-8.f, dy, 0);

		MFSetTexCoord1(1, texelCenter);
		MFSetPosition(sx+8.f, dy, 0);

		MFEnd();
	}
	else if(sy == dy)
		MFPrimitive_DrawQuad(sx, sy-8.f, dx-sx, 16.f, MFVector::one, 0.f, texelCenter, 1.f, 1.f + (texelCenter));
}

void Game::ShowRequest(const char *pMessage, GameUI::MsgBoxDelegate callback, bool bNotification)
{
	pGameUI->ShowMessageBox(pMessage, callback, bNotification);
}

void Game::PushMoveAction(Group *pGroup)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Move;
	pAction->pGroup = pGroup;

	MapTile *pTile = pGroup->GetTile();
	pAction->prop.move.startX = pGroup->x;
	pAction->prop.move.startY = pGroup->y;
	pAction->prop.move.destX = pAction->prop.move.startX;
	pAction->prop.move.destY = pAction->prop.move.startY;

	int numUnits = pGroup->GetNumUnits();
	for(int a=0; a<numUnits; ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		pAction->prop.move.startMove[a] = pUnit->GetMovement();
		pAction->prop.move.destMove[a] = pAction->prop.move.startMove[a];
	}

	Unit *pUnit = pGroup->GetVehicle();
	if(pUnit)
	{
		pAction->prop.move.startMove[numUnits] = pUnit->GetMovement();
		pAction->prop.move.destMove[numUnits] = pAction->prop.move.startMove[numUnits];
	}

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushRearrange(Group *pGroup, Unit **ppNewOrder)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Rearrange;
	pAction->pGroup = pGroup;

	pAction->prop.rearrange.beforeForward = pGroup->numForwardUnits;
	pAction->prop.rearrange.beforeRear = pGroup->numRearUnits;

	for(int a=0; a<pGroup->numForwardUnits; ++a)
		pAction->prop.rearrange.pBefore[a] = pGroup->pForwardUnits[a];
	for(int a=0; a<pGroup->numRearUnits; ++a)
		pAction->prop.rearrange.pBefore[a + 5] = pGroup->pRearUnits[a];

	for(int a=0; a<10; ++a)
	{
		pAction->prop.rearrange.pAfter[a] = ppNewOrder[a];
		if(ppNewOrder[a])
		{
			if(a < 5)
				++pAction->prop.rearrange.afterForward;
			else
				++pAction->prop.rearrange.afterRear;
		}
	}
	pGroup->numForwardUnits = pAction->prop.rearrange.afterForward;
	pGroup->numRearUnits = pAction->prop.rearrange.afterRear;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushRegroup(Group **ppBefore, int numBefore, Group **ppAfter, int numAfter)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Regroup;
	pAction->pGroup = NULL;

	bool bHasParent = false;
	pAction->prop.regroup.numBefore = numBefore;
	for(int a=0; a<numBefore; ++a)
	{
		pAction->prop.regroup.pBefore[a] = ppBefore[a];
		if(ppBefore[a]->pLastAction)
		{
			AddActions(pAction, ppBefore[a]->pLastAction);
			bHasParent = true;
		}
	}

	pAction->prop.regroup.numAfter = numAfter;
	for(int a=0; a<numAfter; ++a)
	{
		pAction->prop.regroup.pAfter[a] = ppAfter[a];
		ppAfter[a]->pLastAction = pAction;
	}

	if(!bHasParent)
		AddActions(pAction, NULL);

	pAction->pParent = NULL; // regroup actions do not have a single parent
}

void Game::PushSearch(Group *pGroup, Place *pRuin)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Search;
	pAction->pGroup = pGroup;
	pAction->prop.search.pUnit = pGroup->GetHero();
	pAction->prop.search.pRuin = pRuin;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushCaptureCastle(Group *pGroup, Castle *pCastle)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_CaptureCastle;
	pAction->pGroup = pGroup;
	pAction->prop.captureCastle.pCastle = pCastle;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushCaptureUnits(Group *pGroup, Group *pUnits)
{
	if(bUpdating)
		return;

	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_CaptureUnits;
	pAction->pGroup = pGroup;
	pAction->prop.captureUnits.pUnits = pUnits;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::UpdateMoveAction(Group *pGroup)
{
	if(bUpdating)
		return;

	MapTile *pTile = pGroup->GetTile();
	Action *pAction = pGroup->pLastAction;
	MFDebug_Assert(pAction && pAction->type == Action::AT_Move, "!?");

	pAction->prop.move.destX = pTile->GetX();
	pAction->prop.move.destY = pTile->GetY();

	int numUnits = pGroup->GetNumUnits();
	for(int a=0; a<numUnits; ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		pAction->prop.move.destMove[a] = pUnit->GetMovement();
	}

	Unit *pUnit = pGroup->GetVehicle();
	if(pUnit)
		pAction->prop.move.destMove[numUnits] = pUnit->GetMovement();
}

void Game::AddActions(Action *pAction, Action *pParent)
{
	// connet to new siblings
	if(pParent)
	{
		pParent->ppChildren = (Action**)actionList.Realloc(pParent->ppChildren, sizeof(Action*)*(pParent->numChildren + 1));
		pParent->ppChildren[pParent->numChildren++] = pAction;
	}
	else
	{
		ppActionHistory = (Action**)actionList.Realloc(ppActionHistory, sizeof(Action*)*(numTopActions + 1));
		ppActionHistory[numTopActions++] = pAction;
	}

	// set the parents
	pAction->pParent = pParent;
}

void Game::CommitActions(Group *pGroup)
{
	CommitAction(pGroup->pLastAction);
	pGroup->pLastAction = NULL;

	ApplyActions();

	UpdateUndoButton();
}

void Game::CommitAction(Action *pAction)
{
	if(!pAction)
		return;

	// commit parent action
	if(pAction->pParent)
		CommitAction(pAction->pParent);

	// commit the action
	switch(pAction->type)
	{
		case Action::AT_Move:
			if(pActionList)
			{
				int numUnits = pAction->pGroup->GetNumUnits() + (pAction->pGroup->pVehicle ? 1 : 0);

				GameAction *pNew = SubmitAction(GA_MOVEGROUP, 3 + numUnits);
				pNew->pArgs[0] = pAction->pGroup->GetID();
				pNew->pArgs[1] = pAction->prop.move.destX;
				pNew->pArgs[2] = pAction->prop.move.destY;

				for(int a=0; a<numUnits; ++a)
					pNew->pArgs[3 + a] = pAction->prop.move.destMove[a];
			}
			break;

		case Action::AT_Rearrange:
			if(pActionList)
			{
				GameAction *pNew = SubmitAction(GA_REARRANGEGROUP, 11);
				pNew->pArgs[0] = pAction->pGroup->GetID();
				for(int a=0; a<10; ++a)
				{
					Unit *pUnit = pAction->prop.rearrange.pAfter[a];
					pNew->pArgs[1 + a] = pUnit ? pUnit->GetID() : -1;
				}
			}
			break;

		case Action::AT_Regroup:
			for(int a=0; a<pAction->prop.regroup.numBefore; ++a)
			{
				Group *pBefore = pAction->prop.regroup.pBefore[a];

				if(pBefore->pLastAction)
				{
					// disconnect parent action
					for(int b=0; b<pBefore->pLastAction->numChildren; ++b)
					{
						if(pBefore->pLastAction->ppChildren[b] == pAction)
						{
							--pBefore->pLastAction->numChildren;
							for(; b<pBefore->pLastAction->numChildren; ++b)
								ppActionHistory[b] = ppActionHistory[b+1];
							break;
						}
					}

					CommitAction(pBefore->pLastAction);
				}

				SubmitActionArgs(GA_DESTROYGROUP, 1, pBefore->GetID());

				pBefore->Destroy();
			}

			for(int a=0; a<pAction->prop.regroup.numAfter; ++a)
			{
				Group *pAfter = pAction->prop.regroup.pAfter[a];

				if(pAfter->pLastAction == pAction)
					pAfter->pLastAction = NULL;

				AddGroup(pAfter);
			}
			break;
		case Action::AT_Search:
			SubmitActionArgs(GA_SEARCH, 2, pAction->prop.search.pUnit->GetID(), pAction->prop.search.pRuin->GetID());
			break;
		case Action::AT_CaptureCastle:
			SubmitActionArgs(GA_CLAIMCASTLE, 2, pAction->prop.captureCastle.pCastle->id, pAction->pGroup->GetPlayer());
			break;
		case Action::AT_CaptureUnits:
			SubmitActionArgs(GA_CAPTUREUNITS, 2, pAction->prop.captureUnits.pUnits->GetID(), pAction->pGroup->GetPlayer());
			break;
	}

	// disconnect action
//	int numInstances = 0;
	for(int a=0; a<numTopActions; ++a)
	{
		if(ppActionHistory[a] == pAction)
		{
			--numTopActions;
			for(int b=a; b<numTopActions; ++b)
				ppActionHistory[b] = ppActionHistory[b+1];
//			break;
			--a;
//			++numInstances;
		}
	}
//	MFDebug_Assert(numInstances < 2, "!!");

	// make child actions top level
	for(int a=0; a<pAction->numChildren; ++a)
		AddActions(pAction->ppChildren[a], NULL);

	// remove action from group
	if(pAction->pGroup && pAction->pGroup->pLastAction == pAction)
		pAction->pGroup->pLastAction = NULL;

	// destroy the action
	DestroyAction(pAction);
}

void Game::CommitAllActions()
{
	while(numTopActions)
		CommitAction(ppActionHistory[numTopActions-1]);

	ApplyActions();
}

void Game::DestroyAction(Action *pAction)
{
	if(pAction->ppChildren)
		actionList.Delete(pAction->ppChildren);
	actionCache.Delete(pAction);
}

Group *Game::RevertAction(Group *pGroup)
{
	Action *pAction = pGroup->pLastAction;
	if(!pAction || (pAction->numChildren && pAction->type != Action::AT_Regroup))
		return pGroup;

	// perform undo
	switch(pAction->type)
	{
		case Action::AT_Move:
		{
			// revert to old tile
			MapTile *pTile = pGroup->GetTile();
			pTile->RemoveGroup(pGroup);
			MapTile *pOldTile = pMap->GetTile(pAction->prop.move.startX, pAction->prop.move.startY);
			pOldTile->AddGroup(pGroup);

			int numUnits = pGroup->GetNumUnits();
			for(int a=0; a<numUnits; ++a)
			{
				// restore unit movement
				Unit *pUnit = pGroup->GetUnit(a);
				pUnit->SetMovement(pAction->prop.move.startMove[a]);
			}

			if(pGroup->pVehicle)
			{
				// restore vehicle movement
				pGroup->pVehicle->SetMovement(pAction->prop.move.startMove[numUnits]);
			}

			// set the groups path target to the position we undid
			pGroup->FindPath(pAction->prop.move.destX, pAction->prop.move.destY);

			// pop action
			pGroup->pLastAction = pAction->pParent;
			break;
		}

		case Action::AT_Rearrange:
		{
			for(int a=0; a<10; ++a)
				pGroup->pForwardUnits[a] = pAction->prop.rearrange.pBefore[a];
			pGroup->numForwardUnits = pAction->prop.rearrange.beforeForward;
			pGroup->numRearUnits = pAction->prop.rearrange.beforeRear;

			// pop action
			pGroup->pLastAction = pAction->pParent;
			break;
		}

		case Action::AT_Regroup:
		{
			// check if all groups are present
			for(int a=0; a<pAction->prop.regroup.numAfter; ++a)
			{
				Group *pAfter = pAction->prop.regroup.pAfter[a];
				if(pAfter->pLastAction != pAction)
				{
					// one group is still missing, lets undo that units most recent action
					Action *pUndo = FindFirstDependency(pAfter->pLastAction);

					Group *pDeepestChild = pUndo->pGroup ? pUndo->pGroup : pUndo->prop.regroup.pAfter[0];
					return RevertAction(pDeepestChild);
				}
			}

			// get map tile
			MapTile *pTile = pAction->prop.regroup.pAfter[0]->GetTile();

			for(int a=0; a<pAction->prop.regroup.numAfter; ++a)
			{
				// destroy the target groups
				Group *pAfter = pAction->prop.regroup.pAfter[a];
				pTile->RemoveGroup(pAfter);
				pAfter->Destroy();
			}

			for(int a=pAction->prop.regroup.numBefore-1; a>=0; --a)
			{
				pGroup = pAction->prop.regroup.pBefore[a];

				// assign the units back to the old group
				for(int b=0; b<pGroup->GetNumUnits(); ++b)
				{
					Unit *pUnit = pGroup->GetUnit(b);
					pUnit->SetGroup(pGroup);
				}

				if(pGroup->pVehicle)
					pGroup->pVehicle->SetGroup(pGroup);

				// add old group to tile
				pTile->AddGroup(pGroup);

				// clear child action from old group
				if(pGroup->pLastAction)
				{
					for(int b=0; b<pGroup->pLastAction->numChildren; ++b)
					{
						if(pGroup->pLastAction->ppChildren[a] == pAction)
						{
							--pGroup->pLastAction->numChildren;
							for(; a<pGroup->pLastAction->numChildren; ++a)
								pGroup->pLastAction->ppChildren[a] = pGroup->pLastAction->ppChildren[a+1];
							break;
						}
					}
				}
			}
		}
	}

	// disconnect action
	if(pAction->pParent)
	{
		for(int a=0; a<pAction->pParent->numChildren; ++a)
		{
			if(pAction->pParent->ppChildren[a] == pAction)
			{
				--pAction->pParent->numChildren;
				for(; a<pAction->pParent->numChildren; ++a)
					pAction->pParent->ppChildren[a] = pAction->pParent->ppChildren[a+1];
				break;
			}
		}
	}
	else if(pAction->type == Action::AT_Regroup)
	{
		for(int p=0; p<pAction->prop.regroup.numBefore; ++p)
		{
			Action *pParent = pAction->prop.regroup.pBefore[p]->pLastAction;
			if(!pParent)
				continue;

			for(int a=0; a<pParent->numChildren; ++a)
			{
				if(pParent->ppChildren[a] == pAction)
				{
					--pParent->numChildren;
					for(; a<pParent->numChildren; ++a)
						pParent->ppChildren[a] = pParent->ppChildren[a+1];
					break;
				}
			}
		}
	}
	else
	{
		for(int a=0; a<numTopActions; ++a)
		{
			if(ppActionHistory[a] == pAction)
			{
				--numTopActions;
				for(; a<numTopActions; ++a)
					ppActionHistory[a] = ppActionHistory[a+1];
				break;
			}
		}
	}

	// destroy the action
	DestroyAction(pAction);

	return pGroup;
}

Action *Game::FindFirstDependency(Action *pAction)
{
	while(pAction->numChildren)
		pAction = pAction->ppChildren[0];
	return pAction;
}

void Game::ApplyActions()
{
	if(pActionList)
		pActionList->Sync();
}

void Game::ReplayActions(int stopAction)
{
	if(!pActionList)
		return;

	uint32 stop = (uint32)stopAction;
	int numActions = pActionList->GetNumActions();

	while((uint32)lastAction < stop && lastAction < numActions)
		ReplayNextAction();
}

void Game::ReplayNextAction()
{
	if(!pActionList)
		return;

	GameAction *pAction = pActionList->GetAction(lastAction++);
	ReplayAction(pAction);
}

int Game::NumPendingActions()
{
	return pActionList ? pActionList->GetNumActions() - lastAction : 0;
}


const char *Game::GetNextActionDesc()
{
	if(!pActionList || lastAction >= pActionList->GetNumActions())
		return "";

	GameAction *pAction = pActionList->GetAction(lastAction);

	return MFStr("%d - %s", lastAction, pAction->GetString());
}

GameAction *Game::SubmitAction(GameActions action, int numArgs)
{
	if(pActionList)
	{
		++lastAction;
		return pActionList->SubmitAction(action, numArgs);
	}
	return NULL;
}

GameAction *Game::SubmitActionArgs(GameActions action, int numArgs, ...)
{
	GameAction *pReturn = NULL;
	if(pActionList)
	{
		++lastAction;
		va_list args;
		va_start(args, numArgs);
		pReturn = pActionList->SubmitActionArgs(action, numArgs, args);
		va_end(args);
	}
	return pReturn;
}

void Game::ReplayAction(GameAction *pAction)
{
	bUpdating = true;

	switch(pAction->action)
	{
		case GA_ADDCASTLES:
			//...
			break;
		case GA_ADDRUINS:
		{
			for(int b=0; b<pAction->numArgs; ++b)
			{
				Place *pPlace = pMap->GetPlace(b);
				if(pPlace->GetType() == Special::ST_Searchable)
					pPlace->ruin.item = pAction->pArgs[b];
			}
			break;
		}
		case GA_CREATEUNIT:
		{
			int player = pAction->pArgs[0];
			int unit = pAction->pArgs[1];
			Unit *pUnit = pUnitDefs->CreateUnit(unit, player);
			if(pUnit->IsHero())
				players[player].pHero[players[player].numHeroes++] = pUnit;
			AddUnit(pUnit);
			break;
		}
		case GA_CREATEGROUP:
		{
			Group *pGroup = Group::Create(pAction->pArgs[0]);
			AddGroup(pGroup);

			for(int b=0; b<11; ++b)
			{
				int unit = pAction->pArgs[3 + b];

				if(unit > -1)
				{
					if(b < 5)
						pGroup->AddForwardUnit(ppUnits[unit]);
					else if (b < 10)
						pGroup->AddRearUnit(ppUnits[unit]);
					else
						pGroup->AddUnit(ppUnits[unit]);
				}
			}

			MapTile *pTile = pMap->GetTile(pAction->pArgs[1], pAction->pArgs[2]);
			pTile->AddGroup(pGroup);

			MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");
			break;
		}
		case GA_DESTROYGROUP:
		{
			Group *pGroup = ppGroups[pAction->pArgs[0]];

			MapTile *pTile = pGroup->GetTile();
			pTile->RemoveGroup(pGroup);

			pGroup->Destroy();
			ppGroups[pAction->pArgs[0]] = NULL;
			break;
		}
		case GA_MOVEGROUP:
		{
			Group *pGroup = ppGroups[pAction->pArgs[0]];
			MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");

			int x = pAction->pArgs[1];
			int y = pAction->pArgs[2];

			MoveGroupToTile(pGroup, pMap->GetTile(x, y));

			int numUnits = pGroup->GetNumUnits();
			for(int b=0; b<numUnits; ++b)
			{
				Unit *pUnit = pGroup->GetUnit(b);
				pUnit->SetMovement(pAction->pArgs[3 + b]);
			}
			if(pGroup->pVehicle)
				pGroup->pVehicle->SetMovement(pAction->pArgs[3 + numUnits]);

			pMap->CenterView((float)x, (float)y);
			break;
		}
		case GA_REARRANGEGROUP:
		{
			Group *pGroup = ppGroups[pAction->pArgs[0]];
			MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");

			pGroup->numForwardUnits = 0;
			pGroup->numRearUnits = 0;
			for(int b=0; b<10; ++b)
			{
				int unit = pAction->pArgs[b+1];
				Unit *pUnit = unit >= 0 ? ppUnits[unit] : NULL;
				pGroup->pUnits[b] = pUnit;
				if(pUnit)
				{
					if(b < 5)
						++pGroup->numForwardUnits;
					else
						++pGroup->numRearUnits;
				}
			}

			MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");
			break;
		}
		case GA_CLAIMCASTLE:
		{
			Castle *pCastle = pMap->GetCastle(pAction->pArgs[0]);
			pCastle->player = pAction->pArgs[1];
			break;
		}
		case GA_CAPTUREUNITS:
		{
			Group *pGroup = ppGroups[pAction->pArgs[0]];
			pGroup->SetPlayer(pAction->pArgs[1]);
			break;
		}
		case GA_SETBUILDING:
		{
			Castle *pCastle = pMap->GetCastle(pAction->pArgs[0]);
			pCastle->building = pCastle->nextBuild = pAction->pArgs[1];
			pCastle->buildTime = pAction->pArgs[2];
			break;
		}
		case GA_SETBATTLEPLAN:
			//...
			break;
		case GA_SEARCH:
		{
			Unit *pUnit = ppUnits[pAction->pArgs[0]];
			Place *pRuin = pMap->GetPlace(pAction->pArgs[1]);
			pUnit->AddItem(pRuin->ruin.item);
			pRuin->ruin.bHasSearched = true;
			break;
		}
		case GA_BATTLE:
		{
			Group *pGroup = ppGroups[pAction->pArgs[0]];
			for(int u=0, a=0; u<pGroup->GetNumUnits(); ++a)
			{
				Unit *pUnit = pGroup->GetUnit(u);
				pUnit->SetHP(pAction->pArgs[1 + a*2]);
				pUnit->SetKills(pAction->pArgs[2 + a*2]);

				if(pUnit->IsDead())
				{
					// destroy the unit
					pGroup->RemoveUnit(pUnit);
					if(!pUnit->IsHero())
						pUnit->Destroy();
				}
				else
				{
					// move to next unit
					++u;
				}
			}

			if(pGroup->GetNumUnits() == 0)
			{
				MapTile *pTile = pGroup->GetTile();
				pTile->RemoveGroup(pGroup);
				pGroup->Destroy();
				pGroup = NULL;
			}
			break;
		}
		case GA_ENDTURN:
			EndTurn();
			break;
		case GA_VICTORY:
			//...
			break;
	}

	bUpdating = false;
}

Player* Game::GetPlayer(uint32 user, int *pPlayer)
{
	if(!bOnline)
		return NULL;

	for(int a=0; a<numPlayers; ++a)
	{
		if(players[a].playerID == user)
		{
			if(pPlayer)
				*pPlayer = a;
			return &players[a];
		}
	}

	return NULL;
}

void Game::ReceivePeerMessage(uint32 user, const char *pMessage)
{
	int player;
	Player *pPlayer = GetPlayer(user, &player);

	do
	{
		char *pNextLine = MFString_Chr(pMessage, '\n');
		if(pNextLine)
			*pNextLine++ = 0;

		// parse args
		char *pArgs[32];
		int numArgs = 0;

		char *pArg = (char*)pMessage;
		for(int a=0; a<32; ++a)
		{
			pArg = MFString_Chr(pArg, ':');
			if(pArg)
			{
				*pArg++ = 0;
				pArgs[a] = pArg;

				++numArgs;
			}
			else
			{
				break;
			}
		}

		if(!MFString_CaseCmp(pMessage, "JOIN"))
		{
		}

		pMessage = pNextLine;
	}
	while(pMessage);
}

void Game::UpdateUndoButton()
{
	pGameUI->ShowUndoButton(pSelection && pSelection->GetLastAction());
}

void Game::ShowMiniMap()
{
	pGameUI->ShowMiniMap();
}
