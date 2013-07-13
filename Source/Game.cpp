#include "Warlords.h"
#include "Game.h"

#include "Menu/Game/GameUI.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "Fuji/MFInput.h"
#include "Fuji/MFSystem.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFFont.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFTexture.h"
#include "Fuji/MFRenderer.h"
#include "Fuji/MFView.h"

#include <stdarg.h>

Game *Game::pCurrent = NULL;

Game::Game(GameParams *pParams)
: history(this)
{
	bOnline = pParams->bOnline;
	gameID = pParams->gameID;

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
			MFZeroMemory(&players[a], sizeof(players[a]));

			players[a].playerID = pParams->players[a].id;
			players[a].race = pParams->players[a].race;
			players[a].colourID = pParams->players[a].colour;
			players[a].colour = pUnitDefs->GetRaceColour(pParams->players[a].colour);
			players[a].startingHero = pParams->players[a].hero;
		}

		numPlayers = pParams->numPlayers;

		// construct the map
		pMap->ConstructMap();
	}

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(this, &Game::ReceivePeerMessage));
}

Game::Game(GameState *pState)
: history(this)
{
	bOnline = true;
	gameID = pState->id;

	Init(pState->map, false);

	// TODO: we should try loading game state and pending actions from disk...
	currentPlayer = 0;
	currentTurn = 0;

	// setup players
	for(int a=0; a<pState->numPlayers; ++a)
	{
		MFZeroMemory(&players[a], sizeof(players[a]));

		players[a].playerID = pState->players[a].id;
		players[a].race = pState->players[a].race;
		players[a].colourID = pState->players[a].colour;
		players[a].colour = pUnitDefs->GetRaceColour(pState->players[a].colour);
		players[a].startingHero = pState->players[a].hero;
	}

	numPlayers = pState->numPlayers;

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
	unitPool.Init(sizeof(Unit), 256, 256);
	groupPool.Init(sizeof(Group), 128, 64);
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
			history.UpdateMoveAction(pSelection);

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
												history.PushCaptureCastle(pSelection, pCastle);

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
										// TODO: need to choose which hero...

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

											history.PushSearch(pHero, pPlace);

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
						history.PushMoveAction(pSelection);

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
	// produce BeginGame action
	MFArray<Action::Castle> castles;
	MFArray<Action::Ruin> ruins;

	int numCastles = pMap->GetNumCastles();
	for(int a=0; a<numCastles; ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		int player = pCastle->GetPlayer();
		if(player != -1)
		{
			Action::Castle &c = castles.push();
			c.castle = a;
			c.player = player;
		}
	}

	int numPlaces = pMap->GetNumPlaces();
	for(int a=0; a<numPlaces; ++a)
	{
		Place *pPlace = pMap->GetPlace(a);
		if(pPlace->GetType() == Special::ST_Searchable)
		{
			Action::Ruin &r = ruins.push();
			r.place = a;
			r.item = pPlace->ruin.item;
		}
	}

	history.PushBeginGame(pMap->GetFileName(), players, numPlayers, castles.getCopy(), castles.size(), ruins.getCopy(), ruins.size());

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
	history.PushBeginTurn(0);

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

	// count castles, and update build times
	int numCastles = 0;
	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			++numCastles;

			if(pCastle->building != -1)
				--pCastle->buildTime;

			// show the starting castle build screen
			if(currentTurn == 0 && IsMyTurn())
				pGameUI->ShowCastleMenu(pCastle);
		}
	}
	if(numCastles == 0)
	{
		// TODO: show warning to player!
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
					pGroup = CreateUnit(pHero->GetType(), pCastle, NULL, true);

					if(pGroup && !bOnline || IsMyTurn())
						pGameUI->ShowCastleMenu(pCastle);
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

					if(!bOnline || IsMyTurn())
						pGroup = CreateUnit(buildUnit.unit, pCastle, NULL, true);
					if(pGroup)
						pCastle->buildTime = pCastle->GetUnitBuildTime();
				}
			}
		}
	}

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
	pMap->CenterView((float)players[player].cursorX, (float)players[player].cursorY);
}

void Game::EndTurn()
{
	// deselect the group
	SelectGroup(NULL);

	// commit all outstanding actions
	CommitAllActions();

	// change build units
	int numCastles = pMap->GetNumCastles();
	int numPossessed = 0;
	for(int a=0; a<numCastles; ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
			++numPossessed;
		if(pCastle->nextBuild != pCastle->building)
		{
			pCastle->buildTime = pCastle->BuildTimeRemaining();
			pCastle->building = pCastle->nextBuild;

			history.PushSetBuild(pCastle->id, pCastle->building, pCastle->buildTime);
		}
	}

	// if player has no castles, they are eliminated
	if(numPossessed == 0)
	{
		players[currentPlayer].bEliminated = true;

		history.PushPlayerEliminated(currentPlayer);

		// TODO: show player eliminated
		//...

	}

	// get the next player
	int nextPlayer = NextPlayer();

	// if all opponents are dead
	int playersAlive = 0;
	for(int a=0; a<numPlayers; ++a)
	{
		if(!players[a].bEliminated)
			++playersAlive;
	}

	if(playersAlive > 1)
	{
		history.PushBeginTurn(nextPlayer);
		BeginTurn(nextPlayer);
	}
	else
	{
		history.PushVictory(nextPlayer);

		// TODO: show victory for nextPlayer
		//...
	}
}

int Game::NextPlayer()
{
	for(int a=0; a<numPlayers; ++a)
	{
		int player = (currentPlayer + 1) % numPlayers;
		if(!players[player].bEliminated)
			return player;
	}

	MFDebug_Assert(false, "How can all players be dead?");
	return -1;
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

	MFArray<Action::Unit> units;

	for(int a=0; a<pGroup->GetNumUnits(); ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);

		auto &unit = units.push();
		unit.unit = pUnit->GetID();
		unit.health = pUnit->GetHP();
		unit.kills = pUnit->GetKills();

		if(pUnit->IsDead())
		{
			pGroup->RemoveUnit(pUnit);
			if(!pUnit->IsHero())
				pUnit->Destroy();
			--a;
		}
	}

	// if all units in the group were destroyed
	if(pGroup->GetNumUnits() == 0)
	{
		pGroup->GetTile()->RemoveGroup(pGroup);
		pGroup->Destroy();
		pGroup = NULL;
	}

	// remove units from all groups on the target tile
	for(int a=0; a<pTarget->GetNumGroups();)
	{
		Group *pG = pTarget->GetGroup(a);
		bool bIsMercGroup = pG->GetPlayer() == -1;

		for(int b=0; b<pG->GetNumUnits(); ++b)
		{
			Unit *pUnit = pG->GetUnit(b);

			if(!bIsMercGroup)
			{
				auto &unit = units.push();
				unit.unit = pUnit->GetID();
				unit.health = pUnit->GetHP();
				unit.kills = pUnit->GetKills();
			}

			if(pUnit->IsDead() || bIsMercGroup)
			{
				pG->RemoveUnit(pUnit);
				if(!pUnit->IsHero())
					pUnit->Destroy();
				--b;
			}
		}

		if(pG->GetNumUnits() == 0)
		{
			pTarget->RemoveGroup(pG);
			pG->Destroy();
		}
		else
			++a;
	}

	// commit the battle results
	history.PushBattle(units.getCopy(), units.size());

	// if the attacker won...
	if(pGroup)
	{
		// select the victorious group
		SelectGroup(pGroup);

		// if the target is an empty castle, capture it
		Castle *pCastle = pTarget->GetCastle();
		if(pCastle && pCastle->IsEmpty())
		{
			pCastle->Capture(pGroup);
			history.PushCaptureCastle(pGroup, pCastle);

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

	Screen::SetNext(pMapScreen);
	pGameUI->Show();
}

Unit *Game::AllocUnit()
{
	return (Unit*)unitPool.Alloc();
}

void Game::DestroyUnit(Unit *pUnit)
{
	unitPool.Free(pUnit);
}

Group *Game::AllocGroup()
{
	return (Group*)groupPool.Alloc();
}

void Game::DestroyGroup(Group *pGroup)
{
	groupPool.Free(pGroup);
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

Group *Game::CreateUnit(int unit, Castle *pCastle, Place *pPlace, bool bCommitUnit)
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

					history.PushResurrect(pUnit);
					break;
				}
			}
		}

		if(!pUnit)
		{
			pUnit = pUnitDefs->CreateUnit(unit, player);
			if(pUnit->IsHero())
				players[player].pHero[players[player].numHeroes++] = pUnit;

			history.PushCreateUnit(pUnit);
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
				pGroup->AddUnit(pUnit2);

				history.PushCreateUnit(pUnit2);
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

						history.PushCreateUnit(pUnit2);

						// create a group for the unit, and add it to the tile
						Group *pGroup2 = Group::Create(player);
						pGroup2->AddUnit(pUnit2);
						pT->AddGroup(pGroup2);

						history.PushCreateGroup(pGroup2);
						break;
					}
				}
			}
		}

		history.PushCreateGroup(pGroup);

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

	history.PushMoveAction(pGroup);
	MapTile *pCurrent = pGroup->GetTile();
	if(pCurrent)
		pCurrent->RemoveGroup(pGroup);
	pTile->AddGroup(pGroup);

	// subtract movement costs?
	//...

	history.UpdateMoveAction(pGroup);

	// claim nearby flags
	pMap->ClaimFlags(pTile->GetX(), pTile->GetY(), pGroup->GetPlayer());

	return true;
}

void Game::SelectGroup(Group *pGroup)
{
	if(pSelection == pGroup)
		return;

	if(pSelection)
		pSelection->bSelected = false;

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
		if(pSelection->pPath)
		{
			pSelection->pPath->Destroy();
			pSelection->pPath = NULL;
		}

		pSelection = NULL;
	}

	UpdateUndoButton();
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

void Game::CommitActions(Group *pGroup)
{
	history.CommitPending(pGroup);

	UpdateUndoButton();
}

void Game::CommitAllActions()
{
	int w, h;
	pMap->GetMapSize(&w, &h);
	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			MapTile *pTile = pMap->GetTile(x, y);
			for(int g=0; g<pTile->GetNumGroups(); ++g)
			{
				Group *pGroup = pTile->GetGroup(g);
				if(pGroup->GetPlayer() == currentPlayer)
					history.CommitPending(pGroup);
			}
		}
	}
}

Group *Game::RevertAction(Group *pGroup)
{
	PendingAction *pAction = pGroup->pLastAction;
	if(!pAction)
		return pGroup;

	// perform undo
	switch(pAction->type)
	{
		case PAT_Move:
		{
			// revert to old tile
			MapTile *pTile = pGroup->GetTile();
			pTile->RemoveGroup(pGroup);
			MapTile *pOldTile = pMap->GetTile(pAction->move.startX, pAction->move.startY);
			pOldTile->AddGroup(pGroup);

			int numUnits = pGroup->GetNumUnits();
			for(int a=0; a<numUnits; ++a)
			{
				// restore unit movement
				Unit *pUnit = pGroup->GetUnit(a);
				pUnit->SetMovement(pAction->move.startMovement[a]);
			}
			if(pGroup->pVehicle)
			{
				// restore vehicle movement
				pGroup->pVehicle->SetMovement(pAction->move.startMovement[numUnits]);
			}

			// set the groups path target to the position we undid
			pGroup->FindPath(pAction->move.destX, pAction->move.destY);

			// pop action
			pGroup->pLastAction = pAction->move.pPreviousAction;
			break;
		}

		case PAT_Regroup:
		{
			// find if any groups are missing...
			PendingAction *pMostRecent = pAction;
			while(1)
			{
				if(pMostRecent->type == PAT_Move)
				{
					if(!pMostRecent->move.pNextAction)
						break;
					pMostRecent = pMostRecent->move.pNextAction;
				}
				else if(pMostRecent->type == PAT_Regroup)
				{
					int a=0;
					for(; a<pMostRecent->regroup.numAfter; ++a)
					{
						if(pMostRecent->regroup.pAfter[a].pNextAction)
						{
							pMostRecent = pMostRecent->regroup.pAfter[a].pNextAction;
							break;
						}
					}
					if(a == pMostRecent->regroup.numAfter)
						break;
				}
			}
			if(pMostRecent != pAction)
			{
				// at least one group is still missing, let's forward the undo action to that group
				if(pMostRecent->type == PAT_Move)
					return RevertAction(pMostRecent->move.pGroup);
				else
					return RevertAction(pMostRecent->regroup.pAfter[0].pGroup);
			}

			// get map tile
			MapTile *pTile = pAction->regroup.pAfter[0].pGroup->GetTile();

			// remove 'after' groups
			for(int a=0; a<pAction->regroup.numAfter; ++a)
			{
				// destroy the target groups
				PendingAction::Regroup::After &after = pAction->regroup.pAfter[a];
				pTile->RemoveGroup(after.pGroup);

				// only destroy if it was created...
				PendingAction::Regroup::SubRegroup &sub = pAction->regroup.pSubRegroups[after.sub];
				if(sub.ppBefore[0] != sub.ppAfter[0])
					after.pGroup->Destroy();
			}

			// revert to 'before' groups
			for(int a=pAction->regroup.numBefore-1; a>=0; --a)
			{
				PendingAction::Regroup::Before &before = pAction->regroup.pBefore[a];
				pGroup = before.pGroup;

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

				// and point at the previous action
				pGroup->pLastAction = before.pPreviousAction;
			}
		}
	}

	// disconnect action
	history.PopPending(pAction);

	return pGroup;
}

void Game::ReplayAction(Action *pAction)
{
//	bUpdating = true;
/*
	switch(pAction->type)
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
*/
//	bUpdating = false;
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
