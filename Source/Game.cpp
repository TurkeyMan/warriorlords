#include "Warlords.h"
#include "Game.h"

#include "Screens/MapScreen.h"
#include "Screens/Battle.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFRenderer.h"

Game *Game::pCurrent = NULL;

Game::Game(GameParams *pParams)
{
	update.SetCompleteDelegate(MakeDelegate(this, &Game::Update));
	commit.SetCompleteDelegate(MakeDelegate(this, &Game::Commit));

	Init(pParams->pMap, pParams->bEditMap);

	bOnline = pParams->bOnline;
	gameID = pParams->gameID;

	lastCommit = lastCommitArg = 0;
	nextAction = nextArg = 0;
	attemptCommit = 0;

	lastAction = 0;
	firstServerAction = numServerActions = serverActionCount = 0;

	currentPlayer = 0;
	currentTurn = 0;

	if(pParams->bEditMap)
	{
		// init the players
		for(int a=0; a<8; ++a)
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
			players[a].gold = 0;
			players[a].cursorX = 0;
			players[a].cursorY = 0;
			players[a].pHero = NULL;
		}

		// construct the map
		pMap->ConstructMap();
	}
}

Game::Game(GameState *pState)
{
	update.SetCompleteDelegate(MakeDelegate(this, &Game::Update));
	commit.SetCompleteDelegate(MakeDelegate(this, &Game::Commit));

	Init(pState->map, false);

	bOnline = true;
	gameID = pState->id;

	// TODO: we should try loading game state and pending actions from disk...
	lastCommit = lastCommitArg = 0;
	nextAction = nextArg = 0;
	attemptCommit = 0;

	currentPlayer = 0;
	currentTurn = 0;

	// setup players
	for(int a=0; a<pState->numPlayers; ++a)
	{
		players[a].playerID = pState->players[a].id;

		players[a].race = pState->players[a].race;
		players[a].colour = pUnitDefs->GetRaceColour(pState->players[a].colour);

		// TODO: we need to fetch these from the server!
		players[a].gold = 0;
		players[a].cursorX = 0;
		players[a].cursorY = 0;
		players[a].pHero = NULL;
	}

	// construct the map
	pMap->ConstructMap();

	// update the game state
	lastAction = 0;
	firstServerAction = numServerActions = serverActionCount = 0;

	UpdateGameState();

	// resume the game
	Screen::SetNext(pMapScreen);
}

void Game::Init(const char *pMapName, bool bEdit)
{
	pText = MFFont_Create("FranklinGothic");
	pBattleNumbersFont = MFFont_Create("Battle");
	pSmallNumbersFont = MFFont_Create("SmallNumbers");

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

	pRequestBox = new RequestBox;

	// allocate runtime data
	const int numSizes = 4;
	const int numItems[numSizes] = { 1024, 256, 64, 4 };
	const int sizes[numSizes] = { sizeof(Action*)*1, sizeof(Action*)*6, sizeof(Action*)*MapTile::MaxUnitsOnTile * 2, sizeof(Action*)*256 };
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
	if(pRequestBox)
		delete pRequestBox;

	if(pMap)
		pMap->Destroy();

	MFFont_Destroy(pText);
	MFFont_Destroy(pBattleNumbersFont);
	MFFont_Destroy(pSmallNumbersFont);

	actionCache.Destroy();
	actionList.Destroy();
}

bool Game::IsCurrentPlayer(int player)
{
	if(player == -1)
		return false;

	if(!bOnline)
		return player == currentPlayer;

	Session *pSession = Session::GetCurrent();
	if(!pSession || pSession->IsOffline())
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
			PushAction(GA_ADDCASTLES);

			for(int a=0; a<numCastles; ++a)
			{
				Castle *pCastle = pMap->GetCastle(a);
				PushActionArg(pCastle->GetPlayer());
			}
		}

		int numRuins = pMap->GetNumRuins();
		if(numRuins)
		{
			PushAction(GA_ADDRUINS);

			for(int a=0; a<numRuins; ++a)
			{
				Ruin *pRuin = pMap->GetRuin(a);
				PushActionArg(pRuin->item);
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

			// produce starting hero (or flag that hero wants to join on the first turn)
			Group *pGroup = CreateUnit(players[pCastle->player].race - 1, pCastle, true);
			players[pCastle->player].pHero = pGroup->GetUnit(0);

			// pirates also start with a galleon
			if(players[pCastle->player].race == 3)
				CreateUnit(38, pCastle, true);
		}
	}

	// begin the first turn
	BeginTurn(0);

	// send the actions to the server
	ApplyActions();

	Screen::SetNext(pMapScreen);
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
				pMapScreen->ShowCastleConfig(pCastle);

			++numCastles;
			players[currentPlayer].gold += pCastle->details.income;

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
				BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

				// if a hero is building here
				if(pUnitDefs->GetUnitType(buildUnit.unit) == UT_Hero && buildUnit.cost <= players[currentPlayer].gold)
				{
					Group *pGroup = NULL;

					if(!bOnline || (IsMyTurn() && NumPendingActions() <= 1))
					{
						pGroup = CreateUnit(buildUnit.unit, pCastle, true);
						if(pGroup)
							pMapScreen->ShowCastleConfig(pCastle);
					}
					else
						players[currentPlayer].pHero->Revive();

					if(pGroup || bOnline)
					{
						players[currentPlayer].gold -= buildUnit.cost;

						// clear the hero from building, and show the build dialog
						pCastle->building = pCastle->nextBuild = -1;
						pCastle->buildTime = 0;
					}
				}
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
				BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

				if(pUnitDefs->GetUnitType(buildUnit.unit) != UT_Hero && buildUnit.cost <= players[currentPlayer].gold)
				{
					Group *pGroup = NULL;

					if(!bOnline || (IsMyTurn() && NumPendingActions() <= 1))
						pGroup = CreateUnit(buildUnit.unit, pCastle, true);

					if(pGroup || bOnline)
					{
						players[currentPlayer].gold -= buildUnit.cost;
						pCastle->buildTime = pCastle->details.buildUnits[pCastle->building].buildTime;
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
			PushAction(GA_VICTORY);
			PushActionArg(currentPlayer);
			ApplyActions();
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
	// commit all outstanding actions
	CommitAllActions();

	// change build units
	int numCastles = pMap->GetNumCastles();
	for(int a=0; a<numCastles; ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->nextBuild != pCastle->building)
		{
			pCastle->building = pCastle->nextBuild;
			pCastle->buildTime = pCastle->nextBuild > -1 ? pCastle->details.buildUnits[pCastle->nextBuild].buildTime : 0;

			if(!bUpdating && IsMyTurn())
			{
				PushAction(GA_SETBUILDING);
				PushActionArg(pCastle->id);
				PushActionArg(pCastle->building);
				PushActionArg(pCastle->buildTime);
			}
		}
	}

	// end the turn
	if(!bUpdating && IsMyTurn())
	{
		PushAction(GA_ENDTURN);
		PushActionArg(currentPlayer);
		ApplyActions();
	}

	// begin the next players turn
	int numPlayers = 6;
	BeginTurn((currentPlayer + 1) % numPlayers);
}

void Game::BeginBattle(Group *pGroup, MapTile *pTarget)
{
	CommitActions(pGroup);

	// enter the battle
	pMapScreen->SelectGroup(NULL);
	pBattle->Begin(pGroup, pTarget);
}

void Game::EndBattle(Group *pGroup, MapTile *pTarget)
{
	pBattle->End();

	// destroy dead units
	if(bOnline)
	{
		PushAction(GA_BATTLE);
		PushActionArg(pGroup->GetID());
	}

	for(int a=0; a<pGroup->GetNumUnits(); ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);

		if(bOnline)
		{
			PushActionArg(pUnit->GetHP());
			PushActionArg(pUnit->GetKills());
		}

		if(pUnit->IsDead())
		{
			pGroup->RemoveUnit(pUnit);
			if(!pUnit->IsHero())
				pUnit->Destroy();
			--a;
		}
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

		if(bOnline && !bIsMercGroup)
		{
			PushAction(GA_BATTLE);
			PushActionArg(pG->GetID());
		}

		for(int b=0; b<pG->GetNumUnits(); ++b)
		{
			Unit *pUnit = pG->GetUnit(b);

			if(bOnline && !bIsMercGroup)
			{
				PushActionArg(pUnit->GetHP());
				PushActionArg(pUnit->GetKills());
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

	if(pGroup)
	{
		// select the victorious group
		pMapScreen->SelectGroup(pGroup);

		// if the target is an empty castle, capture it
		Castle *pCastle = pTarget->GetCastle();
		if(pCastle && pCastle->IsEmpty())
		{
			pCastle->Capture(pGroup);
			PushCaptureCastle(pGroup, pCastle);

			pMapScreen->ShowCastleConfig(pCastle);
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

Group *Game::CreateUnit(int unit, Castle *pCastle, bool bCommitUnit)
{
	// find space in the castle for the unit
	MapTile *pTile = NULL;

	int x = pCastle->details.x;
	int y = pCastle->details.y;

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

			if(pT->IsEnemyTile(pCastle->player))
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
		Unit *pUnit;
		if(players[pCastle->player].pHero && pDetails->type == UT_Hero)
		{
			pUnit = players[pCastle->player].pHero;
			pUnit->Revive();

			pCastle->building = -1;
			pCastle->nextBuild = -1;
		}
		else
		{
			pUnit = pUnitDefs->CreateUnit(unit, pCastle->player);
			AddUnit(pUnit, bCommitUnit);
		}

		// create a group for the unit, and add it to the tile
		Group *pGroup = Group::Create(pCastle->player);
		pGroup->AddUnit(pUnit);
		pTile->AddGroup(pGroup);

		// HACK: Skeletons build 2 at a time!
		if(unit == 14)
		{
			if(pGroup->GetTile()->GetNumUnits() < 10)
			{
				// if there's space on the same tile, add the second one to the same group
				Unit *pUnit2 = pUnitDefs->CreateUnit(unit, pCastle->player);
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
						Unit *pUnit2 = pUnitDefs->CreateUnit(unit, pCastle->player);
						AddUnit(pUnit2, bCommitUnit);

						// create a group for the unit, and add it to the tile
						Group *pGroup2 = Group::Create(pCastle->player);
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

void Game::AddUnit(Unit *pUnit, bool bCommitUnit)
{
	pUnit->SetID(numUnits);
	ppUnits[numUnits++] = pUnit;

	// notify the server of the new unit
	if(bOnline && bCommitUnit)
	{
		PushAction(GA_CREATEUNIT);
		PushActionArg(pUnit->GetPlayer());
		PushActionArg(pUnit->GetType());
		PushActionArg(pUnit->GetMaxMovement());
		PushActionArg(pUnit->GetMaxHP());
	}
}

void Game::AddGroup(Group *pGroup, bool bCommitGroup)
{
	pGroup->SetID(numGroups);
	ppGroups[numGroups++] = pGroup;

	// notify the server of the new group
	if(bOnline && (!bUpdating || bCommitGroup))
	{
		PushAction(GA_CREATEGROUP);
		PushActionArg(pGroup->GetPlayer());

		PushActionArg(pGroup->x);
		PushActionArg(pGroup->y);

		// add front units
		int forwardUnits = pGroup->GetNumForwardUnits();
		for(int a=0; a<5; ++a)
		{
			Unit *pUnit = NULL;
			if(a < forwardUnits)
				pUnit = pGroup->GetForwardUnit(a);
			PushActionArg(pUnit ? pUnit->GetID() : -1);
		}

		// add rear units
		int rearUnits = pGroup->GetNumRearUnits();
		for(int a=0; a<5; ++a)
		{
			Unit *pUnit = NULL;
			if(a < rearUnits)
				pUnit = pGroup->GetRearUnit(a);
			PushActionArg(pUnit ? pUnit->GetID() : -1);
		}

		// add vehicle
		Unit *pVehicle = pGroup->GetVehicle();
		PushActionArg(pVehicle ? pVehicle->GetID() : -1);
	}
}

void Game::DrawWindow(const MFRect &rect)
{
	MFMaterial_SetMaterial(pWindow);
	MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height);
}

void Game::DrawLine(float sx, float sy, float dx, float dy)
{
	MFMaterial_SetMaterial(pHorizLine);

	float texelCenter = MFRenderer_GetTexelCenterOffset() / 16.f;

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

void Game::ShowRequest(const char *pMessage, RequestBox::SelectCallback callback, bool bNotification)
{
	pRequestBox->Show(pMessage, callback, bNotification);
}

bool Game::DrawRequest()
{
	return pRequestBox->Draw();
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

void Game::PushSearch(Group *pGroup, Ruin *pRuin)
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

	pMapScreen->ShowUndoButton();
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
		{
			PushAction(GA_MOVEGROUP);
			PushActionArg(pAction->pGroup->GetID());
			PushActionArg(pAction->prop.move.destX);
			PushActionArg(pAction->prop.move.destY);
			int numUnits = pAction->pGroup->GetNumUnits() + (pAction->pGroup->pVehicle ? 1 : 0);
			for(int a=0; a<numUnits; ++a)
				PushActionArg(pAction->prop.move.destMove[a]);
			break;
		}
		case Action::AT_Rearrange:
			PushAction(GA_REARRANGEGROUP);
			PushActionArg(pAction->pGroup->GetID());
			for(int a=0; a<10; ++a)
			{
				Unit *pUnit = pAction->prop.rearrange.pAfter[a];
				PushActionArg(pUnit ? pUnit->GetID() : -1);
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

				PushAction(GA_DESTROYGROUP);
				PushActionArg(pBefore->GetID());

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
			PushAction(GA_SEARCH);
			PushActionArg(pAction->prop.search.pUnit->GetID());
			PushActionArg(pAction->prop.search.pRuin->id);
			break;
		case Action::AT_CaptureCastle:
			PushAction(GA_CLAIMCASTLE);
			PushActionArg(pAction->prop.captureCastle.pCastle->id);
			PushActionArg(pAction->pGroup->GetPlayer());
			break;
		case Action::AT_CaptureUnits:
			PushAction(GA_CAPTUREUNITS);
			PushActionArg(pAction->prop.captureUnits.pUnits->GetID());
			PushActionArg(pAction->pGroup->GetPlayer());
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

int Game::GetNumRemainingActions()
{
	if(nextAction <= lastCommit)
		return MAX_PENDING_ACTIONS - lastCommit + nextAction;
	return MAX_PENDING_ACTIONS - nextAction - lastCommit;
}

int Game::GetNumRemainingActionArgs()
{
	if(nextArg <= lastCommitArg)
		return MAX_PENDING_ACTION_ARGS - lastCommitArg + nextArg;
	return MAX_PENDING_ACTION_ARGS - nextArg - lastCommitArg;
}

int Game::GetNumPendingActions()
{
	if(attemptCommit < lastCommit)
		return MAX_PENDING_ACTIONS - lastCommit + attemptCommit;
	return attemptCommit - lastCommit;
}

int Game::GetPrevAction()
{
	return nextAction == 0 ? MAX_PENDING_ACTIONS - 1 : nextAction - 1;
}

void Game::PushAction(GameActions action)
{
	if(!bOnline)
		return;

	MFDebug_Assert(GetNumRemainingActions() > 1, "Not enough commit actions!");

	pendingActions[nextAction].action = action;
	pendingActions[nextAction].pArgs = actionArgs + nextArg;
	pendingActions[nextAction].numArgs = 0;
	nextAction = (nextAction + 1) % MAX_PENDING_ACTIONS;
}

void Game::PushActionArg(int arg)
{
	if(!bOnline)
		return;

	MFDebug_Assert(arg >= -1, "!");
	MFDebug_Assert(GetNumRemainingActionArgs() > 1, "Not enough commit args!");

	GameAction &action = pendingActions[GetPrevAction()];

	if(nextArg == 0 && action.numArgs)
	{
		MFDebug_Assert(lastCommitArg > action.numArgs + 1, "Not enough commit args!");

		// copy the existing args to the beginning of the circular queue
		for(int a=0; a<action.numArgs; ++a)
			actionArgs[a] = action.pArgs[a];

		action.pArgs = actionArgs;
		nextArg = action.numArgs;
	}

	actionArgs[nextArg++] = arg;
	++action.numArgs;

	nextArg = nextArg % MAX_PENDING_ACTION_ARGS;
}

void Game::PushActionArgs(int *pArgs, int numArgs)
{
	if(!bOnline)
		return;

	MFDebug_Assert(GetNumRemainingActionArgs() > numArgs + 1, "Not enough commit args!");

	GameAction &action = pendingActions[GetPrevAction()];

	if(nextArg > MAX_PENDING_ACTION_ARGS - numArgs || nextArg < action.numArgs)
	{
		MFDebug_Assert(lastCommitArg > action.numArgs + numArgs, "Not enough commit args!");

		// copy the existing args to the beginning of the circular queue
		for(int a=0; a<action.numArgs; ++a)
			actionArgs[a] = action.pArgs[a];

		action.pArgs = actionArgs;
		nextArg = action.numArgs;
	}

	for(int a=0; a<numArgs; ++a)
		actionArgs[nextArg++] = pArgs[a];
	action.numArgs += numArgs;

	nextArg = nextArg % MAX_PENDING_ACTION_ARGS;
}

void Game::ApplyActions()
{
	attemptCommit = nextAction;

	CommitPending();
}

void Game::CommitPending()
{
	if(!bOnline || lastCommit == attemptCommit || commit.RequestPending())
		return;

	if(attemptCommit < lastCommit)
		pendingCommit = MAX_PENDING_ACTIONS - lastCommit;
	else
		pendingCommit = attemptCommit - lastCommit;

	pendingCommit = WLServ_ApplyActions(commit, gameID, pendingActions + lastCommit, pendingCommit);
}

void Game::Commit(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetError(commit);
	if(err != SE_NO_ERROR)
		return;

	numServerActions += pendingCommit;
	serverActionCount += pendingCommit;
	lastAction += pendingCommit;

	lastCommit = (lastCommit + pendingCommit) % MAX_PENDING_ACTIONS;
	lastCommitArg = lastCommit == nextAction ? 0 : (int)(pendingActions[lastCommit].pArgs - actionArgs);

	if(lastCommit != attemptCommit)
		CommitPending();
}

void Game::UpdateGameState()
{
	if(!update.RequestPending())
		WLServ_UpdateState(update, gameID, lastAction);
}

void Game::Update(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetActions(update, &pServerActions, &numServerActions, &serverActionCount);
	if(err != SE_NO_ERROR)
		return;

	firstServerAction = lastAction;

	ReplayActions();

	// recurse if we don't have all actions yet.
	if(lastAction < serverActionCount)
		UpdateGameState();
}

void Game::ReplayActions(int stopAction)
{
	if(lastAction < firstServerAction)
		return;

	uint32 stop = (uint32)stopAction;

	while((uint32)lastAction < stop && lastAction - firstServerAction < numServerActions)
		ReplayNextAction();
}

void Game::ReplayNextAction()
{
	int serverAction = lastAction - firstServerAction;
	if(serverAction < 0 || serverAction >= numServerActions)
		return;

	ReplayAction(&pServerActions[serverAction]);
	++lastAction;
}

int Game::NumPendingActions()
{
	int action = lastAction - firstServerAction;
	return numServerActions - action;
}

const char *Game::GetNextActionDesc()
{
	if(!pServerActions || numServerActions == 0)
		return "";

	int a = lastAction - firstServerAction;
	const char *pDesc = MFStr("%d - %s: ", lastAction, WLServ_GetActionName(pServerActions[a].action));

	for(int b=0; b<pServerActions[a].numArgs; ++b)
		pDesc = MFStr(b > 0 ? "%s, %d" : "%s%d", pDesc, pServerActions[a].pArgs[b]);
	return pDesc;
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
				Ruin *pRuin = pMap->GetRuin(b);
				pRuin->item = pAction->pArgs[b];
			}
			break;
		}
		case GA_CREATEUNIT:
		{
			int player = pAction->pArgs[0];
			int unit = pAction->pArgs[1];
			Unit *pUnit = pUnitDefs->CreateUnit(unit, player);
			if(pUnit->IsHero())
				players[player].pHero = pUnit;
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
			Ruin *pRuin = pMap->GetRuin(pAction->pArgs[0]);
			pUnit->AddItem(pRuin->item);
			pRuin->bHasSearched = true;
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
