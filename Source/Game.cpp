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
	Init(pParams->pMap, pParams->bEditMap);

	bOnline = pParams->bOnline;
	gameID = pParams->gameID;

	numActions = 0;
	numArgs = 0;

	lastAction = 0;
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
			players[a].playerID = -1;//pParams->players[a];

			players[a].race = pParams->playerRaces[a];
			players[a].colour = pUnitDefs->GetRaceColour(pParams->playerColours[a]);
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
	Init(pState->map, false);

	bOnline = true;
	gameID = pState->id;

	// TODO: we should try loading game state and pending actions from disk...
	numActions = 0;
	numArgs = 0;
	lastAction = 0;

	currentPlayer = pState->currentPlayer;
	currentTurn = pState->currentTurn;

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
	if(!bOnline)
		return player == currentPlayer;

	Session *pSession = Session::GetCurrent();
	if(!pSession || pSession->IsOffline())
		return false;

	return players[currentPlayer].playerID == pSession->GetUserID();
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
			Group *pGroup = CreateUnit(players[pCastle->player].race - 1, pCastle);
			players[pCastle->player].pHero = pGroup->GetUnit(0);

			// pirates also start with a galleon
			if(players[pCastle->player].race == 3)
				CreateUnit(38, pCastle);
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

	// get money + add new units
	int numCastles = 0;
	int numOpponentCastles = 0;

	for(int a = 0; a < pMap->GetNumCastles(); ++a)
	{
		Castle *pCastle = pMap->GetCastle(a);
		if(pCastle->player == currentPlayer)
		{
			// show the starting castle build screen
			if(currentTurn == 0)
				pMapScreen->ShowCastleConfig(pCastle);

			++numCastles;
			players[currentPlayer].gold += pCastle->details.income;

			if(pCastle->building != -1 && --pCastle->buildTime <= 0)
			{
				BuildUnit &buildUnit = pCastle->details.buildUnits[pCastle->building];

				if(buildUnit.cost <= players[currentPlayer].gold)
				{
					Group *pGroup = CreateUnit(buildUnit.unit, pCastle);
					if(pGroup)
					{
						players[currentPlayer].gold -= buildUnit.cost;

						pCastle->SetBuildUnit(pCastle->building);
					}
				}
			}
		}
		else if(pCastle->player != -1)
		{
			++numOpponentCastles;
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

			if(IsMyTurn())
			{
				PushAction(GA_SETBUILDING);
				PushActionArg(pCastle->id);
				PushActionArg(pCastle->building);
				PushActionArg(pCastle->buildTime);
			}
		}
	}

	// end the turn
	if(IsMyTurn())
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
			pCastle->Capture(pGroup->GetPlayer());
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

Group *Game::CreateUnit(int unit, Castle *pCastle)
{
	// find space in the castle for the unit
	MapTile *pTile = NULL;

	int x = pCastle->details.x;
	int y = pCastle->details.y;

	UnitDetails *pDetails = pUnitDefs->GetUnitDetails(unit);
	uint32 castleTerrain = pMap->GetTerrainAt(x, y);

	if(pUnitDefs->GetMovementPenalty(pDetails->movementClass, castleTerrain & 0xFF) == 0)
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
		}
		else
		{
			pUnit = pUnitDefs->CreateUnit(unit, pCastle->player);
			AddUnit(pUnit);
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
				AddUnit(pUnit2);
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
						AddUnit(pUnit2);

						// create a group for the unit, and add it to the tile
						Group *pGroup2 = Group::Create(pCastle->player);
						pGroup2->AddUnit(pUnit2);
						pT->AddGroup(pGroup2);
						AddGroup(pGroup2);
						break;
					}
				}
			}
		}

		// add the group to the manager
		AddGroup(pGroup);

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

void Game::AddUnit(Unit *pUnit)
{
	pUnit->SetID(numUnits);
	ppUnits[numUnits++] = pUnit;

	// notify the server of the new unit
	if(bOnline && !bUpdating)
	{
		PushAction(GA_CREATEUNIT);
		PushActionArg(pUnit->GetPlayer());
		PushActionArg(pUnit->GetType());
		PushActionArg(pUnit->GetMaxMovement());
		PushActionArg(pUnit->GetMaxHP());
	}
}

void Game::AddGroup(Group *pGroup)
{
	pGroup->SetID(numGroups);
	ppGroups[numGroups++] = pGroup;

	// notify the server of the new group
	if(bOnline && !bUpdating)
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

void Game::ShowRequest(const char *pMessage, RequestBox::SelectCallback *pCallback, bool bNotification, void *pUserData)
{
	pRequestBox->Show(pMessage, pCallback, bNotification, pUserData);
}

bool Game::DrawRequest()
{
	return pRequestBox->Draw();
}

void Game::PushMoveAction(Group *pGroup)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Move;
	pAction->pGroup = pGroup;

	MapTile *pTile = pGroup->GetTile();
	pAction->move.startX = pGroup->x;
	pAction->move.startY = pGroup->y;
	pAction->move.destX = pAction->move.startX;
	pAction->move.destY = pAction->move.startY;

	int numUnits = pGroup->GetNumUnits();
	for(int a=0; a<numUnits; ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		pAction->move.startMove[a] = pUnit->GetMovement();
		pAction->move.destMove[a] = pAction->move.startMove[a];
	}

	Unit *pUnit = pGroup->GetVehicle();
	if(pUnit)
	{
		pAction->move.startMove[numUnits] = pUnit->GetMovement();
		pAction->move.destMove[numUnits] = pAction->move.startMove[numUnits];
	}

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushRearrange(Group *pGroup, Unit **ppNewOrder)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Rearrange;
	pAction->pGroup = pGroup;

	pAction->rearrange.beforeForward = pGroup->numForwardUnits;
	pAction->rearrange.beforeRear = pGroup->numRearUnits;

	for(int a=0; a<pGroup->numForwardUnits; ++a)
		pAction->rearrange.pBefore[a] = pGroup->pForwardUnits[a];
	for(int a=0; a<pGroup->numRearUnits; ++a)
		pAction->rearrange.pBefore[a + 5] = pGroup->pRearUnits[a];

	for(int a=0; a<10; ++a)
	{
		pAction->rearrange.pAfter[a] = ppNewOrder[a];
		if(ppNewOrder[a])
		{
			if(a < 5)
				++pAction->rearrange.afterForward;
			else
				++pAction->rearrange.afterRear;
		}
	}
	pGroup->numForwardUnits = pAction->rearrange.afterForward;
	pGroup->numRearUnits = pAction->rearrange.afterRear;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushRegroup(Group **ppBefore, int numBefore, Group *pAfter)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Regroup;
	pAction->pGroup = pAfter;

	pAction->regroup.numBefore = numBefore;
	for(int a=0; a<numBefore; ++a)
	{
		pAction->regroup.pBefore[a] = ppBefore[a];
		AddActions(pAction, ppBefore[a]->pLastAction);
	}

	pAction->pParent = NULL; // regroup actions do not have a single parent
	pAfter->pLastAction = pAction;
}

void Game::PushSearch(Group *pGroup, Ruin *pRuin)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_Search;
	pAction->pGroup = pGroup;
	pAction->search.pUnit = pGroup->GetHero();
	pAction->search.pRuin = pRuin;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushCaptureCastle(Group *pGroup, Castle *pCastle)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_CaptureCastle;
	pAction->pGroup = pGroup;
	pAction->captureCastle.pCastle = pCastle;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::PushCaptureUnits(Group *pGroup, Group *pUnits)
{
	Action *pAction = (Action*)actionCache.Alloc();
	MFZeroMemory(pAction, sizeof(Action));

	pAction->type = Action::AT_CaptureUnits;
	pAction->pGroup = pGroup;
	pAction->captureUnits.pUnits = pUnits;

	AddActions(pAction, pGroup->pLastAction);
	pGroup->pLastAction = pAction;
}

void Game::UpdateMoveAction(Group *pGroup)
{
	MapTile *pTile = pGroup->GetTile();
	Action *pAction = pGroup->pLastAction;
	MFDebug_Assert(pAction && pAction->type == Action::AT_Move, "!?");

	pAction->move.destX = pTile->GetX();
	pAction->move.destY = pTile->GetY();

	int numUnits = pGroup->GetNumUnits();
	for(int a=0; a<numUnits; ++a)
	{
		Unit *pUnit = pGroup->GetUnit(a);
		pAction->move.destMove[a] = pUnit->GetMovement();
	}

	Unit *pUnit = pGroup->GetVehicle();
	if(pUnit)
		pAction->move.destMove[numUnits] = pUnit->GetMovement();
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
	if(bOnline)
	{
		switch(pAction->type)
		{
			case Action::AT_Move:
			{
				PushAction(GA_MOVEGROUP);
				PushActionArg(pAction->pGroup->GetID());
				PushActionArg(pAction->move.destX);
				PushActionArg(pAction->move.destY);
				int numUnits = pAction->pGroup->GetNumUnits() + (pAction->pGroup->pVehicle ? 1 : 0);
				for(int a=0; a<numUnits; ++a)
					PushActionArg(pAction->move.destMove[a]);
				break;
			}
			case Action::AT_Rearrange:
				PushAction(GA_REARRANGEGROUP);
				PushActionArg(pAction->pGroup->GetID());
				for(int a=0; a<10; ++a)
				{
					Unit *pUnit = pAction->rearrange.pAfter[a];
					PushActionArg(pUnit ? pUnit->GetID() : -1);
				}
				break;
			case Action::AT_Regroup:
				for(int a=0; a<pAction->regroup.numBefore; ++a)
				{
					Group *pBefore = pAction->regroup.pBefore[a];

					if(pBefore->pLastAction)
					{
						for(int b=0; b<pBefore->pLastAction->numChildren; ++b)
						{
							Action *pT = pBefore->pLastAction->ppChildren[b];
							if(pT == pAction)
								continue;

							for(int c=0; c<pT->regroup.numBefore; ++c)
							{
								if(pT->regroup.pBefore[c] == pBefore)
								{
									--pT->regroup.numBefore;
									for(int d=c; d<pT->regroup.numBefore; ++d)
										pT->regroup.pBefore[d] = pT->regroup.pBefore[d + 1];
									break;
								}
							}

							CommitAction(pT);
						}

						CommitAction(pBefore->pLastAction);
					}

					PushAction(GA_DESTROYGROUP);
					PushActionArg(pBefore->GetID());

					pBefore->Destroy();
				}

				AddGroup(pAction->pGroup);
				break;
			case Action::AT_Search:
				PushAction(GA_SEARCH);
				PushActionArg(pAction->search.pUnit->GetID());
				PushActionArg(pAction->search.pRuin->id);
				break;
			case Action::AT_CaptureCastle:
				PushAction(GA_CLAIMCASTLE);
				PushActionArg(pAction->captureCastle.pCastle->id);
				PushActionArg(pAction->pGroup->GetPlayer());
				break;
			case Action::AT_CaptureUnits:
				PushAction(GA_CAPTUREUNITS);
				PushActionArg(pAction->captureUnits.pUnits->GetID());
				PushActionArg(pAction->pGroup->GetPlayer());
				break;
		}
	}

	// disconnect action
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

	// make child actions top level
	for(int a=0; a<pAction->numChildren; ++a)
		AddActions(pAction->ppChildren[a], NULL);

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
	if(!pAction || pAction->numChildren)
		return pGroup;

	// perform undo
	switch(pAction->type)
	{
		case Action::AT_Move:
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
				pUnit->SetMovement(pAction->move.startMove[a]);
			}

			if(pGroup->pVehicle)
			{
				// restore vehicle movement
				pGroup->pVehicle->SetMovement(pAction->move.startMove[numUnits]);
			}

			// set the groups path target to the position we undid
			pGroup->FindPath(pAction->move.destX, pAction->move.destY);

			// pop action
			pGroup->pLastAction = pAction->pParent;
			break;
		}

		case Action::AT_Rearrange:
		{
			for(int a=0; a<10; ++a)
				pGroup->pForwardUnits[a] = pAction->rearrange.pBefore[a];
			pGroup->numForwardUnits = pAction->rearrange.beforeForward;
			pGroup->numRearUnits = pAction->rearrange.beforeRear;

			// pop action
			pGroup->pLastAction = pAction->pParent;
			break;
		}

		case Action::AT_Regroup:
		{
			Group *pNewGroup = pGroup;
			bool bDidUndoOne = false;

			for(int a=pAction->regroup.numBefore-1; a>=0; --a)
			{
				Group *pBefore = pAction->regroup.pBefore[a];
				Action *pLastAction = pBefore->pLastAction;

				// all child branches must undo before we can undo a split regroup
				bool bAllPresent = true;
				for(int b=0; b<pLastAction->numChildren; ++b)
				{
					Action *pChild = pLastAction->ppChildren[b];

					if(pChild->numChildren)
					{
						bAllPresent = false;
						break;
					}
				}

				if(bAllPresent)
				{
					MapTile *pTile = pGroup->GetTile();

					int numUnits = pBefore->GetNumUnits();
					for(int b=0; b<pLastAction->numChildren;)
					{
						Group *pGroup = pLastAction->ppChildren[b]->pGroup;

						for(int c=0; c<numUnits; ++c)
						{
							Unit *pUnit = pBefore->GetUnit(c);
							pGroup->RemoveUnit(pUnit);
							pUnit->SetGroup(pBefore);
						}

						if(pBefore->pVehicle)
						{
							pGroup->RemoveUnit(pBefore->pVehicle);
							pBefore->pVehicle->SetGroup(pBefore);
						}

						// check if we emptied the group
						if(pGroup->GetNumUnits() == 0 && !pGroup->pVehicle)
						{
							// destroy the groups regroup action
							DestroyAction(pLastAction->ppChildren[b]);

							// unhook it from patent actions
							--pLastAction->numChildren;
							for(int c=b; c<pLastAction->numChildren; ++c)
								pLastAction->ppChildren[c] = pLastAction->ppChildren[c + 1];

							// destroy the group
							pTile->RemoveGroup(pGroup);
							pGroup->Destroy();
						}
						else
						{
							// move to next child
							++b;
						}
					}

					pTile->AddGroup(pBefore);
					pNewGroup = pBefore;
					bDidUndoOne = true;
				}
			}

			if(!bDidUndoOne)
			{				
				for(int a=0; a<pAction->regroup.numBefore; ++a)
				{
					Group *pBefore = pAction->regroup.pBefore[a];
					Action *pLastAction = pBefore->pLastAction;
					for(int b=0; b<pLastAction->numChildren; ++b)
					{
						Action *pChild = pLastAction->ppChildren[b];
						if(pChild->numChildren)
						{
							pChild = FindFirstDependency(pChild);
							return RevertAction(pChild->pGroup);
						}
					}
				}
			}

			return pNewGroup;
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


void Game::PushAction(GameActions action)
{
	if(!bOnline)
		return;

	pendingActions[numActions].action = action;
	pendingActions[numActions].pArgs = actionArgs + numArgs;
	pendingActions[numActions].numArgs = 0;
	++numActions;
}

void Game::PushActionArg(int arg)
{
	if(!bOnline)
		return;

	MFDebug_Assert(arg >= -1, "!");

	actionArgs[numArgs++] = arg;
	++pendingActions[numActions-1].numArgs;
}

void Game::PushActionArgs(int *pArgs, int numArgs)
{
	if(!bOnline)
		return;

	for(int a=0; a<numArgs; ++a)
		actionArgs[numArgs++] = pArgs[a];
	pendingActions[numActions-1].numArgs += numArgs;
}

ServerError Game::ApplyActions()
{
	if(!bOnline)
		return SE_NO_ERROR;

	ServerError err = WLServ_ApplyActions(gameID, pendingActions, numActions);
	numActions = numArgs = 0;
	return err;
}

void Game::UpdateGameState()
{
	bUpdating = true;

	GameAction *pActions;
	int numActions, actionCount;

	do
	{
		ServerError err = WLServ_UpdateState(gameID, lastAction, &pActions, &numActions, &actionCount);
		lastAction += numActions;

		for(int a=0; a<numActions; ++a)
		{
			switch(pActions[a].action)
			{
				case GA_ADDCASTLES:
					//...
					break;
				case GA_ADDRUINS:
				{
					for(int b=0; b<pActions[a].numArgs; ++b)
					{
						Ruin *pRuin = pMap->GetRuin(b);
						pRuin->item = pActions[a].pArgs[0];
					}
					break;
				}
				case GA_CREATEUNIT:
				{
					int player = pActions[a].pArgs[0];
					int unit = pActions[a].pArgs[1];
					Unit *pUnit = pUnitDefs->CreateUnit(unit, player);
					if(unit < 8)
						players[player].pHero = pUnit;
					AddUnit(pUnit);
					break;
				}
				case GA_CREATEGROUP:
				{
					Group *pGroup = Group::Create(pActions[a].pArgs[0]);
					AddGroup(pGroup);

					for(int b=0; b<11; ++b)
					{
						int unit = pActions[a].pArgs[3 + b];

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

					MapTile *pTile = pMap->GetTile(pActions[a].pArgs[1], pActions[a].pArgs[2]);
					pTile->AddGroup(pGroup);
					break;
				}
				case GA_DESTROYGROUP:
				{
					Group *pGroup = ppGroups[pActions[a].pArgs[0]];

					MapTile *pTile = pGroup->GetTile();
					pTile->RemoveGroup(pGroup);

					pGroup->Destroy();
					ppGroups[pActions[a].pArgs[0]] = NULL;
					break;
				}
				case GA_MOVEGROUP:
				{
					Group *pGroup = ppGroups[pActions[a].pArgs[0]];

					MapTile *pTile = pGroup->GetTile();
					pTile->RemoveGroup(pGroup);

					pTile = pMap->GetTile(pActions[a].pArgs[1], pActions[a].pArgs[2]);
					pTile->AddGroup(pGroup);

					int numUnits = pGroup->GetNumUnits();
					for(int b=0; b<numUnits; ++b)
					{
						Unit *pUnit = pGroup->GetUnit(b);
						pUnit->SetMovement(pActions[a].pArgs[3 + b]);
					}
					if(pGroup->pVehicle)
						pGroup->pVehicle->SetMovement(pActions[a].pArgs[3 + numUnits]);
					break;
				}
				case GA_REARRANGEGROUP:
				{
					Group *pGroup = ppGroups[pActions[a].pArgs[0]];

					for(int b=0; b<10; ++b)
					{
//						pGroup->
					}
					break;
				}
				case GA_CLAIMCASTLE:
				{
					Castle *pCastle = pMap->GetCastle(pActions[a].pArgs[0]);
					pCastle->player = pActions[a].pArgs[1];
					break;
				}
				case GA_CAPTUREUNITS:
				{
					Group *pGroup = ppGroups[pActions[a].pArgs[0]];
					pGroup->SetPlayer(pActions[a].pArgs[1]);
					break;
				}
				case GA_SETBUILDING:
				{
					Castle *pCastle = pMap->GetCastle(pActions[a].pArgs[0]);
					pCastle->building = pCastle->nextBuild = pActions[a].pArgs[1];
					pCastle->buildTime = pActions[a].pArgs[2];
					break;
				}
				case GA_SETBATTLEPLAN:
					//...
					break;
				case GA_SEARCH:
				{
					Unit *pUnit = ppUnits[pActions[a].pArgs[0]];
					Ruin *pRuin = pMap->GetRuin(pActions[a].pArgs[0]);
					pUnit->AddItem(pRuin->item);
					pRuin->bHasSearched = true;
					break;
				}
				case GA_BATTLE:
				{
					Group *pGroup = ppGroups[pActions[a].pArgs[0]];
					for(int a=0; a<pGroup->GetNumUnits(); ++a)
					{
						Unit *pUnit = pGroup->GetUnit(a);
						pUnit->SetHP(pActions[a].pArgs[1 + a*2]);
						pUnit->SetKills(pActions[a].pArgs[2 + a*2]);

						if(pUnit->IsDead())
						{
							pGroup->RemoveUnit(pUnit);
							if(!pUnit->IsHero())
								pUnit->Destroy();
							--a;
						}
					}

					MapTile *pTile = pGroup->GetTile();
					if(pGroup->GetNumUnits() == 0)
					{
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
		}
	}
	while(numActions < actionCount);

	bUpdating = false;
}
