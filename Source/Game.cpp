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

Game::Game(History *pHistory, Map *pMap)
: pendingPool(sizeof(PendingAction), 1024, 1024)
, history(*pHistory)
, pMap(pMap)
{
}

Game::Game(GameParams *pParams)
: pendingPool(sizeof(PendingAction), 1024, 1024)
, history(*(new History()))
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
		int races[16];
		for(int a=0; a<pParams->numPlayers; ++a)
			races[a] = pParams->players[a].race;
		pMap->ConstructMap(races);
	}

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(this, &Game::ReceivePeerMessage));
}

Game::Game(GameState *pState)
: pendingPool(sizeof(PendingAction), 1024, 1024)
, history(*(new History()))
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
	int races[16];
	for(int a=0; a<pState->numPlayers; ++a)
		races[a] = pState->players[a].race;
	pMap->ConstructMap(races);

	// update the game state
	lastAction = 0;

	// resume the game
	Screen::SetNext(pMapScreen);
	pGameUI->Show();

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(this, &Game::ReceivePeerMessage));
}

Game *Game::NewGame(GameParams *pParams)
{
	History *pHistory = new History();

	// create map...
	Map *pMap = Map::Create(NULL, pParams->pMap, false);
	UnitDefinitions *pUnitDefs = pMap->GetUnitDefinitions();

	int races[16];
	for(int a=0; a<pParams->numPlayers; ++a)
		races[a] = pParams->players[a].race;
	pMap->ConstructMap(races);

	// populate map with starting players
	MFArray<Action::Castle> castles;
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

	// populate the ruins with items
	MFArray<Action::Ruin> ruins;
	int numPlaces = pMap->GetNumPlaces();
	for(int a=0; a<numPlaces; ++a)
	{
		Place *pPlace = pMap->GetPlace(a);
		pPlace->ruin.item = MFRand() % pUnitDefs->GetNumItems();
		if(pPlace->GetType() == Special::ST_Searchable)
		{
			Action::Ruin &r = ruins.push();
			r.place = a;
			r.item = pPlace->ruin.item;
		}
	}

	// produce the BeginGame action
	Action::Player players[16];
	for(int a=0; a<pParams->numPlayers; ++a)
	{
		players[a].id = pParams->players[a].id;
		players[a].race = pParams->players[a].race;
		players[a].colour = pParams->players[a].colour;
	}

	pHistory->PushBeginGame(pParams->pMap, players, pParams->numPlayers, castles.getCopy(), castles.size(), ruins.getCopy(), ruins.size());

	Game *pGame = new Game(pHistory, pMap);

	pGame->bOnline = pParams->bOnline;
	pGame->gameID = pParams->gameID;

	pGame->Init(NULL, false);

	pGame->lastAction = 0;
	pGame->currentPlayer = 0;
	pGame->currentTurn = 0;

	// setup players
	for(int a=0; a<pParams->numPlayers; ++a)
	{
		MFZeroMemory(&pGame->players[a], sizeof(pGame->players[a]));

		pGame->players[a].playerID = pParams->players[a].id;
		pGame->players[a].race = pParams->players[a].race;
		pGame->players[a].colourID = pParams->players[a].colour;
		pGame->players[a].colour = pGame->pUnitDefs->GetRaceColour(pParams->players[a].colour);
		pGame->players[a].startingHero = pParams->players[a].hero;
	}

	pGame->numPlayers = pParams->numPlayers;

	// TODO: eliminate this...
	Game *pOld = Game::SetCurrent(pGame);

	// create starting units
	for(int a=0; a<pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player != -1)
		{
			// set each players map focus on starting castle
			pGame->players[pCastle->player].cursorX = pCastle->details.x;
			pGame->players[pCastle->player].cursorY = pCastle->details.y;

			// produce starting hero
			int hero = pGame->players[pCastle->player].startingHero;
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

			Group *pGroup = pGame->CreateUnit(heroID, pCastle, NULL, true);
			pGame->players[pCastle->player].pHero[pGame->players[pCastle->player].numHeroes++] = pGroup->GetUnit(0);

			// pirates also start with a galleon
			int pirateID = pUnitDefs->FindRace("Pirates");
			int galleonID = pUnitDefs->FindUnit("Galleon");
			if(players[pCastle->player].race == pirateID && galleonID >= 0)
				pGame->CreateUnit(galleonID, pCastle, NULL, true);
		}
	}

	// begin the first turn
	pGame->BeginTurn(0);
	pGame->history.PushBeginTurn(0);

	Game::SetCurrent(pOld);

	// TODO: move this?
	Screen::SetNext(pGame->pMapScreen);
	pGame->pGameUI->Show();

	// hook up message receiver
	Session *pSession = Session::Get();
	pSession->SetMessageCallback(MakeDelegate(pGame, &Game::ReceivePeerMessage));

	return pGame;
}

Game *Game::ResumeGame(const char *pGameName, bool bOnline)
{
	char *pGameHistory = MFFileSystem_Load(pGameName);
	History *pHistory = new History();
	pHistory->Read(pGameHistory);

	Game *pGame = new Game(pHistory);

	MFHeap_Free(pGameHistory);

	// ...

	return pGame;
}

Game *Game::CreateEditor(const char *pMap)
{
	Game *pGame = new Game((History*)NULL);

	pGame->bOnline = false;
	pGame->gameID = 0;

	pGame->Init(pMap, true);

	pGame->lastAction = 0;
	pGame->currentPlayer = 0;
	pGame->currentTurn = 0;

	// init the players
	int numRaces = pGame->pUnitDefs->GetNumRaces() - 1;
	for(int a=0; a<numRaces; ++a)
	{
		pGame->players[a].race = a + 1;
		pGame->players[a].colour = pGame->pUnitDefs->GetRaceColour(a + 1);
	}

	// construct the map
	pGame->pMap->ConstructMap(0);

	return pGame;
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

	// create the map and screens
	if(!pMap)
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
}

void Game::Draw()
{
	pMap->Draw(this);

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
												CommitPending(pSelection);
												history.PushCaptureCastle(pSelection, pCastle);

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
												history.PushCaptureUnits(pSelection, pUnits);

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
								CommitPending(pSelection);

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
			CommitPending(pGroup);
			history.PushCaptureCastle(pGroup, pCastle);

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

Unit *Game::CreateUnit(int unit, int player)
{
	Unit *pUnit = AllocUnit();

	pUnit->type = unit;
	pUnit->player = player;

	pUnit->pUnitDefs = pUnitDefs;
	pUnit->details = *pUnitDefs->GetUnitDetails(unit);
	pUnit->pName = pUnit->details.pName;
	pUnit->pGroup = NULL;

	pUnit->pItems = NULL;
	pUnit->numItems = 0;

	if(pUnit->details.type == UT_Hero)
	{
		MFDebug_Assert(pUnit->details.numItems <= Unit::MaxItems, "Too many items!");

		pUnit->pItems = (int*)MFHeap_Alloc(sizeof(int)*Unit::MaxItems);

		pUnit->numItems = pUnit->details.numItems;
		for(int a=0; a<pUnit->details.numItems; ++a)
			pUnit->pItems[a] = pUnit->details.items[a];
	}

	pUnit->kills = pUnit->victories = 0;
	pUnit->UpdateStats();

	pUnit->life = pUnit->GetMaxHP();
	pUnit->movement = pUnit->GetMaxMovement() * 2;

	pUnit->plan.type = TT_Ranged;
	pUnit->plan.strength = TS_Weakest;
	pUnit->plan.bAttackAvailable = true;

	return pUnit;
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
			pUnit = CreateUnit(unit, player);

			if(pUnit->IsHero())
				players[player].pHero[players[player].numHeroes++] = pUnit;

			pUnit->id = AddUnit(pUnit);
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
				Unit *pUnit2 = CreateUnit(unit, player);
				pGroup->AddUnit(pUnit2);

				pUnit2->id = AddUnit(pUnit2);
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
						Unit *pUnit2 = CreateUnit(unit, player);

						pUnit2->id = AddUnit(pUnit2);
						history.PushCreateUnit(pUnit2);

						// create a group for the unit, and add it to the tile
						Group *pGroup2 = Group::Create(player);
						pGroup2->AddUnit(pUnit2);
						pT->AddGroup(pGroup2);

						pGroup2->id = AddGroup(pGroup2);
						history.PushCreateGroup(pGroup2);
						history.PushRestack(pT);
						break;
					}
				}
			}
		}

		pGroup->id = AddGroup(pGroup);
		history.PushCreateGroup(pGroup);
		history.PushRestack(pTile);

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

			history.PushMove(pAction);

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
			a.restack.y = regroup.x;

			// create new groups
			for(int i=0; i<regroup.numAfter; ++i)
			{
				PendingAction::Regroup::After &newGroup = regroup.pAfter[i];
				int sub = newGroup.sub;

				// check if the group wasn't changed
				if(regroup.pSubRegroups[sub].ppBefore[0] != regroup.pSubRegroups[sub].ppAfter[0])
				{
					// if it was, create the new group
					newGroup.pGroup->id = AddGroup(newGroup.pGroup);
					history.PushCreateGroup(newGroup.pGroup);
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
			history.Push(a);

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
	PopPending(pAction);

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
