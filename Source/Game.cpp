#include "Warlords.h"
#include "Game.h"
#include "Profile.h"
#include "Lobby.h"
#include "Group.h"

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
#include "Fuji/MFDocumentJSON.h"

#include <stdarg.h>

Game::Game(GameState &state)
: pendingPool(sizeof(PendingAction), 1024, 1024)
, state(state)
, mapView(this, state.Map())
, groupConfig(this)
{
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
	MFTexture_Release(pTemp);

	// and allocate the game screens
	pMapScreen = new MapScreen(this);
	pBattle = new Battle(this);
}

Game *Game::CreateEditor(MFString map)
{
	MFDebug_Assert(false, "Not done!");

	MapTemplate *pMapTemplate = MapTemplate::Create(map);
	MFDebug_Assert(pMapTemplate, "!");

	GameState *pState = new GameState(*pMapTemplate);

	// construct the map
	pState->map.ConstructMap(0);

	// setup players in edit positions
	int numRaces = pState->map.UnitDefs()->GetNumRaces() - 1;
	for(int a=0; a<numRaces; ++a)
	{
		::Player &p = pState->players.push();
		p.race = a + 1;
		p.colour = pState->map.UnitDefs()->GetRaceColour(a + 1);
	}

	Game *pGame = new Game(*pState);

	return pGame;
}

Game::~Game()
{
	if(pBattle)
		delete pBattle;
	if(pMapScreen)
		delete pMapScreen;

	MFMaterial_Release(pIcons);

	MFFont_Destroy(pText);
	MFFont_Destroy(pBattleNumbersFont);
	MFFont_Destroy(pSmallNumbersFont);

	delete pGameUI;
}

void Game::Show()
{
	UnitButton::SetUnitDefs(Map().UnitDefs());

	Screen::SetNext(pMapScreen);
	pGameUI->Show();
}

void Game::Update()
{
	// update UI
	pGameUI->Update();

	// update map
	mapView.Update();

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
			MapTile *pNewTile = Map().GetTile(x, y);
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
			Map().ClaimFlags(x, y, pSelection->GetPlayer());

			// update the move action
			UpdateMoveAction(pSelection);

			// center the map on the guy moving
			mapView.CenterView((float)x, (float)y);

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
	mapView.Draw();

	if(pSelection)
	{
		// render the path
		if(state.IsCurrentPlayer(pSelection->GetPlayer()))
		{
			Path *pPath = pSelection->GetPath();
			if(pPath && pPath->GetPathLength())
			{
				MFView_Push();

				int xTiles, yTiles;
				mapView.SetMapOrtho(&xTiles, &yTiles);

				float xStart, yStart;
				mapView.GetOffset(&xStart, &yStart);
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
		renderUnits.clear();

		int numUnits = pSelection->GetNumUnits();
		int tx = 42 + numUnits*32;
		int ty = 37 - (int)MFFont_GetFontHeight(pText)/2;

		if(numUnits == 0)
		{
			Unit *pUnit = pSelection->GetVehicle();
			const UnitDetails &details = pUnit->GetDetails();
			renderUnits.push() = pUnit->Render(5.f + (details.width-1)*22.f, 5.f + (details.height-1)*30.f);
			tx += (details.width-1)*76;
			ty += (details.height-1)*18;
		}
		else
		{
			for(int a = numUnits-1; a >= 0; --a)
			{
				Unit *pUnit = pSelection->GetUnit(a);
				renderUnits.push() = pUnit->Render(5.f + (float)a*32.f, 5.f);
			}
		}
		DrawUnits(renderUnits, 64.f, MFRenderer_GetTexelCenterOffset());

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
			mapView.GetCursor(ev.tap.x, ev.tap.y, &cx, &cy);
			cursorX = (int)cx;
			cursorY = (int)cy;

			// get the tile
			MapTile *pTile = Map().GetTile(cursorX, cursorY);

			if(pSelection && state.IsCurrentPlayer(pSelection->GetPlayer()))
			{
				// if we've already selected a group on this tile
				if(pTile == pSelection->pTile)
				{
					SelectGroup(NULL);

					// show group config screen
					groupConfig.Show(this, pTile);
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
							MapTile *pTile = Map().GetTile(cursorX, cursorY);

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
												CommitPending(pSelection);
												state.History().PushCaptureCastle(pSelection, pCastle);

												pCastle->Capture(pSelection);

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

												CommitPending(pSelection);
												state.History().PushCaptureUnits(pUnits);

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

											CommitPending(pSelection);
											state.History().PushSearch(pHero, pPlace);

											const Item &item = Map().UnitDefs()->GetItem(pPlace->ruin.item);
											pMessage = MFStr("You search the ruin and find\n%s", item.name.CStr());
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
								CommitPending(pSelection);

								if(pMessage)
									ShowRequest(pMessage, NULL, true);
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
				pSelection->pPath = Map().FindPath(pSelection, cursorX, cursorY);
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
					if(pCastle && state.IsCurrentPlayer(pCastle->GetPlayer()))
					{
						// enter the castle config menu
						pGameUI->ShowCastleMenu(pCastle);
					}
				}
			}
			break;
		}
	}

	return mapView.ReceiveInputEvent(manager, ev);
}

void Game::BeginTurn(int player)
{
	state.BeginTurn(player);

	int currentPlayer = state.CurrentPlayer();
	int currentRound = state.CurrentRound();
	::Map &map = Map();

	// heal units and restore movement
	int width, height;
	map.GetMapSize(&width, &height);

	for(int y=0; y<height; ++y)
	{
		for(int x=0; x<width; ++x)
		{
			MapTile *pTile = map.GetTile(x, y);

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
	for(int a = 0; a < map.GetNumCastles(); ++a)
	{
		Castle *pCastle = map.GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			++numCastles;

			if(pCastle->building != -1)
				--pCastle->buildTime;

			// show the starting castle build screen
			if(currentRound == 1 && IsMyTurn())
				pGameUI->ShowCastleMenu(pCastle);
		}
	}
	if(numCastles == 0)
	{
		// TODO: show warning to player!
	}

	// resurrect heroes
	for(int a = 0; a < map.GetNumCastles(); ++a)
	{
		Castle *pCastle = map.GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			if(pCastle->building != -1 && pCastle->buildTime <= 0)
			{
				// if a hero is building here
				if(pCastle->building >= 4)
				{
					Group *pGroup = NULL;

					Unit *pHero = Player(currentPlayer).pHero[pCastle->building - 4];
					pGroup = state.CreateUnit(pHero->GetType(), pCastle, NULL);

					if(pGroup && !state.IsOnline() || state.IsMyTurn())
						pGameUI->ShowCastleMenu(pCastle);
				}
			}
		}
	}

	// recruit heroes
	for(int a = 0; a < map.GetNumPlaces(); ++a)
	{
		Place *pPlace = map.GetPlace(a);

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
				state.CreateUnit(pPlace->recruit.recruiting, NULL, pPlace);

				pPlace->recruit.pRecruitHero = NULL;
				pPlace->recruit.recruiting = -1;
			}
		}
	}

	// build units
	for(int a = 0; a < map.GetNumCastles(); ++a)
	{
		Castle *pCastle = map.GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			if(pCastle->building != -1 && pCastle->buildTime <= 0)
			{
				if(pCastle->building < 4)
				{
					BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

					Group *pGroup = NULL;

					if(!state.IsOnline() || state.IsMyTurn())
						pGroup = state.CreateUnit(buildUnit.unit, pCastle, NULL);
					if(pGroup)
						pCastle->buildTime = pCastle->GetUnitBuildTime();
				}
			}
		}
	}

	// focus map on starting castle, oooor maybe not (focus on last focused position?)
	mapView.CenterView((float)Player(player).cursorX, (float)Player(player).cursorY);
}

void Game::EndTurn()
{
	// deselect the group
	SelectGroup(NULL);

	// commit all outstanding actions
	CommitAllActions();

	// change build units
	int currentPlayer = state.CurrentPlayer();
	int numCastles = Map().GetNumCastles();
	int numPossessed = 0;
	for(int a=0; a<numCastles; ++a)
	{
		Castle *pCastle = Map().GetCastle(a);
		if(pCastle->player == currentPlayer)
			++numPossessed;
		if(pCastle->nextBuild != pCastle->building)
		{
			pCastle->buildTime = pCastle->BuildTimeRemaining();
			pCastle->building = pCastle->nextBuild;

			state.History().PushSetBuild(pCastle->id, pCastle->building, pCastle->buildTime);
		}
	}

	// if player has no castles, they are eliminated
	if(numPossessed == 0)
	{
		Player(currentPlayer).bEliminated = true;

		state.History().PushPlayerEliminated(currentPlayer);

		// TODO: show player eliminated
		//...

	}

	// get the next player
	int nextPlayer = NextPlayer();

	// if all opponents are dead
	int playersAlive = 0;
	for(int a=0; a<NumPlayers(); ++a)
	{
		if(!Player(a).bEliminated)
			++playersAlive;
	}

	if(playersAlive > 1)
	{
		BeginTurn(nextPlayer);
	}
	else
	{
		state.History().PushVictory(nextPlayer);

		// TODO: show victory for nextPlayer
		//...
	}
}

int Game::NextPlayer()
{
	int numPlayers = NumPlayers();
	for(int a=0; a<numPlayers; ++a)
	{
		int player = (state.CurrentPlayer() + 1) % numPlayers;
		if(!Player(player).bEliminated)
			return player;
	}

	MFDebug_Assert(false, "How can all players be dead?");
	return -1;
}

void Game::BeginBattle(Group *pGroup, MapTile *pTarget)
{
	CommitPending(pGroup);

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
				delete pUnit;
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
					delete pUnit;
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
	state.History().PushBattle(units.getCopy(), units.size());

	// if the attacker won...
	if(pGroup)
	{
		// select the victorious group
		SelectGroup(pGroup);

		// if the target is an empty castle, capture it
		Castle *pCastle = pTarget->GetCastle();
		if(pCastle && pCastle->IsEmpty())
		{
			CommitPending(pGroup);
			state.History().PushCaptureCastle(pGroup, pCastle);

			pCastle->Capture(pGroup);

			pGameUI->ShowCastleMenu(pCastle);
		}

		// check if the target is clear, and move there
		if(pTarget->CanMove(pGroup))
			MoveGroupToTile(pGroup, pTarget);

		pGroup->pPath->Destroy();
		pGroup->pPath = NULL;

		// commit all battle actions
		CommitPending(pGroup);
	}

	Screen::SetNext(pMapScreen);
	pGameUI->Show();
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
	Map().ClaimFlags(pTile->GetX(), pTile->GetY(), pGroup->GetPlayer());

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
			pSelection->pPath = Map().FindPath(pGroup, pGroup->pathX, pGroup->pathY);
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

void Game::PushMoveAction(Group *pGroup)
{
	PendingAction *pA = (PendingAction*)pendingPool.AllocAndZero();
	pA->type = PAT_Move;
	pA->move.pGroup = pGroup;
	pA->move.startX = pGroup->x;
	pA->move.startY = pGroup->y;
	pA->move.destX = pGroup->x;
	pA->move.destY = pGroup->y;

	// set movement
	int numUnits = pGroup->GetNumUnits();
	for(int a=0; a<numUnits; ++a)
		pA->move.endMovement[a] = pA->move.startMovement[a] = pGroup->GetUnit(a)->GetMovement();
	if(pGroup->pVehicle)
	{
		pA->move.endMovement[numUnits] = pA->move.startMovement[numUnits] = pGroup->pVehicle->GetMovement();
		++numUnits;
	}
	for(int a=numUnits; a<11; ++a)
		pA->move.endMovement[a] = pA->move.startMovement[a] = -1;

	// couple the action to the previous action
	pA->move.pPreviousAction = pGroup->pLastAction;
	if(pGroup->pLastAction)
	{
		PendingAction *pPrev = pGroup->pLastAction;
		switch(pPrev->type)
		{
			case PAT_Move:
				pPrev->move.pNextAction = pA;
				break;

			case PAT_Regroup:
				for(int a=0; a<pPrev->regroup.numAfter; ++a)
				{
					if(pPrev->regroup.pAfter[a].pGroup == pGroup)
						pPrev->regroup.pAfter[a].pNextAction = pA;
				}
				break;
		}
	}

	pGroup->pLastAction = pA;
}

void Game::UpdateMoveAction(Group *pGroup)
{
	MapTile *pTile = pGroup->GetTile();
	PendingAction *pAction = pGroup->pLastAction;
	MFDebug_Assert(pAction && pAction->type == PAT_Move, "!?");

	pAction->move.destX = pTile->GetX();
	pAction->move.destY = pTile->GetY();
	for(int a=0; a<11; ++a)
	{
		if(a < 5)
			pAction->move.endMovement[a] = a < pGroup->numForwardUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else if(a < 10)
			pAction->move.endMovement[a] = a - 5 < pGroup->numRearUnits ? pGroup->pUnits[a]->GetMovement() : -1;
		else
			pAction->move.endMovement[a] = pGroup->pUnits[a] ? pGroup->pUnits[a]->GetMovement() : -1;
	}
}

void Game::PushRegroup(PendingAction::Regroup *pRegroup)
{
	PendingAction *pA = (PendingAction*)pendingPool.AllocAndZero();
	pA->type = PAT_Regroup;

	pA->regroup = *pRegroup;

	// couple the action to each before group's previous action
	for(int a=0; a<pA->regroup.numBefore; ++a)
	{
		PendingAction::Regroup::Before &before = pA->regroup.pBefore[a];

		before.pPreviousAction = before.pGroup->pLastAction;
		if(before.pPreviousAction)
		{
			// couple the action to the previous action
			PendingAction *pPrev = before.pPreviousAction;
			switch(pPrev->type)
			{
				case PAT_Move:
					pPrev->move.pNextAction = pA;
					break;

				case PAT_Regroup:
					for(int a=0; a<pPrev->regroup.numAfter; ++a)
					{
						if(pPrev->regroup.pAfter[a].pGroup == before.pGroup)
							pPrev->regroup.pAfter[a].pNextAction = pA;
					}
					break;
			}
		}
	}
	for(int a=0; a<pRegroup->numAfter; ++a)
	{
		pRegroup->pAfter[a].pGroup->pLastAction = pA;
		pRegroup->pAfter[a].pNextAction = NULL;
	}
}

void Game::CommitPending(Group *pGroup)
{
	if(pGroup->pLastAction)
	{
		CommitAction(pGroup->pLastAction);
		pGroup->pLastAction = NULL;
	}

	UpdateUndoButton();
}

void Game::CommitAllActions()
{
	int w, h;
	Map().GetMapSize(&w, &h);
	for(int y=0; y<h; ++y)
	{
		for(int x=0; x<w; ++x)
		{
			MapTile *pTile = Map().GetTile(x, y);
			for(int g=0; g<pTile->GetNumGroups(); ++g)
			{
				Group *pGroup = pTile->GetGroup(g);
				if(pGroup->GetPlayer() == CurrentPlayer())
					CommitPending(pGroup);
			}
		}
	}
}

void Game::CommitAction(PendingAction *pAction)
{
	switch(pAction->type)
	{
		case PAT_Move:
		{
			PendingAction::Move &move = pAction->move;

			if(move.pPreviousAction)
				CommitAction(move.pPreviousAction);

			state.History().PushMove(pAction);

			// disconnect from action tree
			if(pAction->move.pNextAction)
			{
				PendingAction *pNext = pAction->move.pNextAction;

				switch(pNext->type)
				{
					case PAT_Move:
						pNext->move.pPreviousAction = NULL;
						break;

					case PAT_Regroup:
						for(int a=0; a<pNext->regroup.numBefore; ++a)
						{
							if(pNext->regroup.pBefore[a].pPreviousAction == pAction)
								pNext->regroup.pBefore[a].pPreviousAction = NULL;
						}
						break;
				}
			}
			else
			{
				MFDebug_Assert(pAction->move.pGroup->pLastAction == pAction, "Something's gone wonky?");
				pAction->move.pGroup->pLastAction = NULL;
			}

			break;
		}

		case PAT_Regroup:
		{
			PendingAction::Regroup &regroup = pAction->regroup;

			// commit previous actions for each source group
			for(int i=0; i<regroup.numBefore; ++i)
			{
				if(regroup.pBefore[i].pPreviousAction)
					CommitAction(regroup.pBefore[i].pPreviousAction);
			}

			Action a;
			a.type = AT_Restack;
			a.restack.numGroups = regroup.numAfter;
			a.restack.x = regroup.x;
			a.restack.y = regroup.y;

			// create new groups
			for(int i=0; i<regroup.numAfter; ++i)
			{
				PendingAction::Regroup::After &newGroup = regroup.pAfter[i];
				int sub = newGroup.sub;

				// check if the group wasn't changed
				if(regroup.pSubRegroups[sub].ppBefore[0] != regroup.pSubRegroups[sub].ppAfter[0])
				{
					// if it was, create the new group
					newGroup.pGroup->id = state.AddGroup(newGroup.pGroup);
					state.History().PushCreateGroup(newGroup.pGroup);
				}

				// add group to stack
				a.restack.groupStack[i] = newGroup.pGroup->GetID();

				// disconnect from action tree
				if(newGroup.pNextAction)
				{
					PendingAction *pNext = newGroup.pNextAction;

					switch(pNext->type)
					{
						case PAT_Move:
							pNext->move.pPreviousAction = NULL;
							break;

						case PAT_Regroup:
							for(int a=0; a<pNext->regroup.numBefore; ++a)
							{
								if(pNext->regroup.pBefore[a].pPreviousAction == pAction)
									pNext->regroup.pBefore[a].pPreviousAction = NULL;
							}
							break;
					}
				}
				else
				{
					MFDebug_Assert(newGroup.pGroup->pLastAction == pAction, "Something's gone wonky?");
					newGroup.pGroup->pLastAction = NULL;
				}
			}

			// push the restack action
			state.History().Push(a);

			MFHeap_Free(pAction->regroup.pMem);
			break;
		}
	}

	pendingPool.Free(pAction);
}

void Game::PopPending(PendingAction *pAction)
{
	switch(pAction->type)
	{
		case PAT_Move:
			DisconnectAction(pAction, pAction->move.pPreviousAction);
			break;

		case PAT_Regroup:
			for(int i=0; i<pAction->regroup.numBefore; ++i)
				DisconnectAction(pAction, pAction->regroup.pBefore[i].pPreviousAction);
			MFHeap_Free(pAction->regroup.pMem);
			break;
	}

	pendingPool.Free(pAction);
}

void Game::DisconnectAction(PendingAction *pAction, PendingAction *pFrom)
{
	if(pFrom)
	{
		switch(pFrom->type)
		{
			case PAT_Move:
				MFDebug_Assert(pFrom->move.pNextAction == pAction, "Expected; == pAction!");
				pFrom->move.pNextAction = NULL;
				break;
			case PAT_Regroup:
				for(int i=0; i<pFrom->regroup.numAfter; ++i)
				{
					if(pFrom->regroup.pAfter[i].pNextAction == pAction)
						pFrom->regroup.pAfter[i].pNextAction = NULL;
				}
				break;
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
			MapTile *pOldTile = Map().GetTile(pAction->move.startX, pAction->move.startY);
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
	PopPending(pAction);

	return pGroup;
}
/*
Player* Game::GetPlayer(uint32 user, int *pPlayer)
{
	if(!state.IsOnline())
		return NULL;

	for(int a=0; a<numPlayers; ++a)
	{
		if(players[a].pUser->ID() == user)
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
*/
void Game::UpdateUndoButton()
{
	pGameUI->ShowUndoButton(pSelection && pSelection->GetLastAction());
}

void Game::ShowMiniMap()
{
	pGameUI->ShowMiniMap();
}

void Game::DrawUnits(const MFArray<UnitRender> &units, float scale, float texelOffset, bool bHead, bool bRank)
{
	if(units.size() == 0)
		return;

	UnitDefinitions *pUnitDefs = Map().UnitDefs();

	int numRanked = 0;

	MFMaterial_SetMaterial(bHead ? pUnitDefs->GetUnitHeadsMaterial() : pUnitDefs->GetUnitMaterial());

	MFPrimitive(PT_TriList);
	MFBegin(6*units.size());

	for(size_t u=0; u<units.size(); ++u)
	{
		const UnitRender &unit = units[u];
		const UnitDetails &def = pUnitDefs->GetUnitDetails(unit.unit);

		if(unit.rank > 0)
			++numRanked;

		MFSetColourV(MakeVector(GetPlayerColour(unit.player), unit.alpha));

		MFRect uvs = pUnitDefs->GetUnitUVs(unit.unit, unit.bFlip, texelOffset);

		float depth = 0.f;//bHead ? 0.f : (1000.f - unit.y) / 1000.f;

		float xOffset = -(def.width - 1) / 2.f * scale;
		float yOffset = -(def.height - 1) / 2.f * scale;
		MFSetTexCoord1(uvs.x, uvs.y);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset+def.height*scale, depth);

		MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x+uvs.width, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset+def.height*scale, depth);
		MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset+def.height*scale, depth);
	}

	MFEnd();

	if(bRank && numRanked != 0)
	{
		MFMaterial_SetMaterial(pUnitDefs->GetRanksMaterial());

		MFPrimitive(PT_TriList);
		MFBegin(6*numRanked);

		for(size_t u=0; u<units.size(); ++u)
		{
			const UnitRender &unit = units[u];
			if(unit.rank == 0)
				continue;

			const UnitDetails &def = pUnitDefs->GetUnitDetails(unit.unit);

			MFSetColour(1.f, 1.f, 1.f, unit.alpha);

			MFRect uvs = pUnitDefs->GetBadgeUVs(unit.rank, texelOffset);

			float depth = 0.f;//bHead ? 0.f : (1000.f - unit.y) / 1000.f;

			float xOffset = -(def.width - 1) / 2.f * scale + (unit.bFlip ? def.width*scale - 0.25f*scale : 0.f);
			float yOffset = -(def.height - 1) / 2.f * scale;
			MFSetTexCoord1(uvs.x, uvs.y);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset+0.25f*scale, depth);

			MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x+uvs.width, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset+0.25f*scale, depth);
			MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset+0.25f*scale, depth);
		}

		MFEnd();
	}
}

int Game::DrawCastle(int race)
{
	return 0;
}

int Game::DrawFlag(int race)
{
	return 0;
}

int Game::DrawSpecial(int index)
{
	return 0;
}
