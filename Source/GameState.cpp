#include "Warlords.h"
#include "GameState.h"
#include "Unit.h"
#include "Group.h"
#include "Profile.h"
#include "Lobby.h"

#include "Fuji/MFDocumentJSON.h"

#include <map>

std::map<uint32, GameState*> gameMap;
MFObjectPool GameState::groupPool;

void GameState::Init()
{
	groupPool.Init(sizeof(Group), 128, 64);
}

void GameState::Deinit()
{
	groupPool.Deinit();
}

GameState *GameState::Get(uint32 id)
{
	return gameMap[id];
}

GameState *GameState::FromJson(MFJSONValue *pJson)
{
	MFJSONValue *pID = pJson->Member("id");
	uint32 id = pID->Int();

	GameState *pGame = GameState::Get(id);
	if(!pGame)
	{
		MFJSONValue *pMapValue = pJson->Member("map");
		if(!pMapValue)
			return NULL;

		MapTemplate *pMapTemplate = MapTemplate::Create(pMapValue->String());
		if(!pMapTemplate)
			return NULL;

		pGame = new GameState(*pMapTemplate);
		pGame->gameID = id;

		gameMap[id] = pGame;
	}

	//...
/*
	// get first action
	Action *pAction = history.GetNext();
	MFDebug_Assert(pAction && pAction->type == AT_BeginGame, "Expected: BeginGame action");
	Action &beginAction = *pAction;

	// if a map was not supplied
	if(!pMap)
	{
		// load the map
		this->pMap = pMap = new ::Map(*(new MapTemplate(beginAction.beginGame.pMap)));

		// construct the map from slices
		int races[16];
		for(int a=0; a<beginAction.beginGame.numPlayers; ++a)
			races[a] = beginAction.beginGame.players[a].race;
		map.ConstructMap(races);

		// populate ruins
		for(int a=0; a<beginAction.beginGame.numRuins; ++a)
		{
			int place = beginAction.beginGame.pRuins[a].place;
			map.GetPlace(place)->InitRuin(beginAction.beginGame.pRuins[a].item);
		}
	}

	// setup players
	numPlayers = beginAction.beginGame.numPlayers;
	MFZeroMemory(players, sizeof(players));
	for(int a=0; a<beginAction.beginGame.numPlayers; ++a)
	{
		players[a].pUser = Profile::Get(beginAction.beginGame.players[a].id);
		players[a].race = beginAction.beginGame.players[a].race;
		players[a].colourID = beginAction.beginGame.players[a].colour;
		players[a].colour = map.UnitDefs()->GetRaceColour(beginAction.beginGame.players[a].colour);
	}
*/

	return pGame;
}

GameState::GameState(MapTemplate &mapTemplate)
: map(*(new ::Map(mapTemplate, *this)))
{
	pMap = &map;

	// init fields
	gameID = 0;

	currentPlayer = -1;
	currentRound = 0;
}

GameState::GameState(MapTemplate &mapTemplate, ::Map &map)
: pMap(NULL)
, map(map)
{
	// init fields
	gameID = 0;

	currentPlayer = -1;
	currentRound = 0;
}

GameState::~GameState()
{
	if(pMap)
		delete pMap;
}

GameState *GameState::NewGame(Lobby *pLobby)
{
	MapTemplate &map = pLobby->Map();
	map.AddRef();

	// create the GameState object
	GameState *pGame = new GameState(map);

	UnitDefinitions *pUnitDefs = pGame->map.UnitDefs();

	// randomise players
	int randomMap[16];
	int numPlayers = 0;
	while(numPlayers < pLobby->NumPlayers())
	{
try_again:
		randomMap[numPlayers] = MFRand() % pLobby->NumPlayers();
		for(int a=0; a<numPlayers; ++a)
		{
			if(randomMap[a] == randomMap[numPlayers])
				goto try_again;
		}
		++numPlayers;
	}

	// configure players
	Action::Player players[16];
	for(int a=0; a<numPlayers; ++a)
	{
		// get randomised player
		Lobby::Player &player = pLobby->Players(randomMap[a]);

		// setup the players starting state
		players[a].id = player.pUser ? player.pUser->ID() : 0;
		players[a].race = player.race;
		players[a].colour = player.colour;

		if(players[a].race == 0)
		{
			// select random race
			while(players[a].race == 0)
			{
				int r = (MFRand() % 4) + 1;
//				if(game.mapDetails.bRacePresent[r])
					players[a].race = r;
			}
		}

		// add to the GameState players array
		::Player &p = pGame->players.push();
		MFZeroMemory(&p, sizeof(player));
		p.pUser = player.pUser;
		p.race = players[a].race;
		p.colourID = player.colour;
		p.colour = pUnitDefs->GetRaceColour(player.colour);
	}

	// construct the map
	pGame->map.ConstructMap();

	// populate the ruins with items
	MFArray<Action::Ruin> ruins;
	int numPlaces = pGame->map.GetNumPlaces();
	for(int a=0; a<numPlaces; ++a)
	{
		Place *pPlace = pGame->map.GetPlace(a);
		if(pPlace->GetType() == Special::ST_Searchable)
		{
			// select an item for the ruin...
			// TODO: should we allow duplicate items?
			int item = -1;
			do
			{
				item = MFRand() % pUnitDefs->GetNumItems();
				if(!pUnitDefs->GetItem(item).bCollectible)
					item = -1;
			}
			while(item < 0);

			pPlace->ruin.item = item;
			pPlace->ruin.bHasSearched = false;

			Action::Ruin &r = ruins.push();
			r.place = a;
			r.item = item;
		}
	}

	pGame->history.PushBeginGame(pLobby->Map().FileName(), players, numPlayers, ruins.getCopy(), ruins.size());

	// create starting units
	for(int p=0; p<numPlayers; ++p)
	{
		::Player &player = pGame->players[p];

		pGame->BeginTurn(p);

		int hero = pLobby->Players(randomMap[p]).hero;

		// choose random hero
		if(hero == -1)
			hero = MFRand() % pUnitDefs->GetNumHeroesForRace(player.race);

		int heroID = pUnitDefs->GetHeroForRace(player.race, hero);
		MFDebug_Assert(heroID != -1, "Couldn't find hero!");

		for(int a=0; a<pGame->map.GetNumCastles(); ++a)
		{
			Castle *pCastle = pGame->map.GetCastle(a);
			if(pCastle->player == p)
			{
				// set each players map focus on starting castle
				player.cursorX = pCastle->details.x;
				player.cursorY = pCastle->details.y;

				// produce starting hero
				pGame->CreateUnit(heroID, pCastle, NULL);

				// pirates also start with a galleon
				int pirateID = pUnitDefs->FindRace("Pirates");
				int galleonID = pUnitDefs->FindUnit("Galleon");
				if(player.race == pirateID && galleonID >= 0)
					pGame->CreateUnit(galleonID, pCastle, NULL);
			}
		}
	}

	return pGame;
}

bool GameState::PlayerHasHero(int player, int hero) const
{
	for(int a=0; a<players[player].numHeroes; ++a)
	{
		if(players[player].pHero[a]->GetType() == hero)
			return true;
	}
	return false;
}

bool GameState::IsCurrentPlayer(int player) const
{
	if(player == -1)
		return false;

	if(!IsOnline())
		return player == currentPlayer;

	Session *pSession = Session::Get();
	if(!pSession)
		return false;

	return player == currentPlayer && players[player].pUser->ID() == pSession->UserID();
}

void GameState::BeginTurn(int player)
{
	if(player < currentPlayer)
		++currentRound;

	history.PushBeginTurn(player);
	currentPlayer = player;
}

void GameState::ReplayUntil(int action)
{
	while(history.NumApplied() < action)
	{
		Action *pAction = history.GetNext();
		if(!pAction)
			break;

		ReplayAction(pAction);
	}
}

void GameState::ReplayAction(Action *pAction)
{
	switch(pAction->type)
	{
		case AT_BeginGame:
			MFDebug_Assert(false, "We should have already done this!");
			break;
		case AT_BeginTurn:
			if(pAction->beginTurn.player < currentPlayer)
				++currentRound;
			currentPlayer = pAction->beginTurn.player;
			break;
		case AT_PlayerEliminated:
			// TODO: show dialog
			players[pAction->playerEliminated.player].bEliminated = true;
			break;
		case AT_Victory:
			// TODO: show dialog
			break;
		case AT_SetBuild:
		{
			Castle *pCastle = map.GetCastle(pAction->setBuild.castle);
			pCastle->building = pAction->setBuild.unit;
			pCastle->buildTime = pCastle->GetUnitBuildTime();
			break;
		}
		case AT_CreateUnit:
		{
			Unit *pUnit = CreateUnit(pAction->createUnit.unitType, currentPlayer);
  			break;
		}
		case AT_CreateGroup:
		{
			Group *pGroup = Group::Create(currentPlayer);
			for(int a=0; a<11; ++a)
			{
				if(pAction->createGroup.units[a] == -1)
					continue;

				Unit *pUnit = units[pAction->createGroup.units[a]];

				if(a < 5)
					pGroup->AddForwardUnit(pUnit);
				else if(a < 10)
					pGroup->AddRearUnit(pUnit);
				else
					pGroup->AddUnit(pUnit);
			}
			pGroup->id = AddGroup(pGroup);
			break;
		}
		case AT_Resurrect:
			units[pAction->resurrect.unit]->Revive();
			break;
		case AT_Restack:
		{
			MapTile *pTile = map.GetTile(pAction->restack.x, pAction->restack.y);
			Group *pGroup;
			while(pGroup = pTile->GetGroup(0))
				pTile->RemoveGroup(pGroup);
			for(int a = pAction->restack.numGroups - 1; a >= 0; --a)
				pTile->AddGroup(groups[pAction->restack.groupStack[a]]);
			break;
		}
		case AT_Move:
		{
			Group *pGroup = groups[pAction->move.group];
			pGroup->GetTile()->RemoveGroup(pGroup);
			MapTile *pTile = map.GetTile(pAction->move.destX, pAction->move.destY);
			pTile->AddGroup(pGroup);

			int numUnits = pGroup->GetNumUnits();
			for(int a=0; a<numUnits; ++a)
				pGroup->GetUnit(a)->SetMovement(pAction->move.endMovement[a]);
			if(pGroup->GetVehicle())
				pGroup->GetVehicle()->SetMovement(pAction->move.endMovement[numUnits]);
			break;
		}
		case AT_Search:
		{
			Place *pPlace = map.GetPlace(pAction->search.place);
			units[pAction->search.unit]->AddItem(pPlace->ruin.item);
			pPlace->ruin.bHasSearched = true;
			break;
		}
		case AT_Battle:
			for(int a=0; a<pAction->battle.numUnits; ++a)
			{
				Action::Unit &result = pAction->battle.pResults[a];
				Unit *pUnit = units[result.unit];
				if(result.health)
				{
					pUnit->SetHP(result.health);
					pUnit->SetKills(result.kills);
				}
				else
				{
					Group *pGroup = pUnit->GetGroup();
					pGroup->RemoveUnit(pUnit);

					if(!pUnit->IsHero())
					{
						units[pUnit->GetID()] = NULL;
						delete pUnit;
					}

					if(pGroup->GetNumUnits() == 0 && !pGroup->GetVehicle())
					{
						groups[pGroup->GetID()] = NULL;
						pGroup->GetTile()->RemoveGroup(pGroup);
						pGroup->Destroy();
					}
				}
			}
			break;
		case AT_CaptureCastle:
		{
			Castle *pCastle = map.GetCastle(pAction->captureCastle.castle);
			pCastle->ClearBuildUnit();
			pCastle->player = currentPlayer;
			break;
		}
		case AT_CaptureUnits:
			groups[pAction->captureUnits.group]->SetPlayer(currentPlayer);
			break;
	}
}

Group *GameState::AllocGroup()
{
	return new(groupPool.Alloc()) Group;
}

void GameState::DestroyGroup(Group *pGroup)
{
	pGroup->~Group();
	groupPool.Free(pGroup);
}

Unit *GameState::CreateUnit(int unit, int player)
{
	Unit *pUnit = map.UnitDefs()->CreateUnit(unit, player, this);

	if(pUnit->details.type == UT_Hero)
		players[player].pHero[players[player].numHeroes++] = pUnit;

	pUnit->id = AddUnit(pUnit);

	return pUnit;
}

Group *GameState::CreateUnit(int unit, Castle *pCastle, Place *pPlace)
{
	UnitDefinitions *pUnitDefs = map.UnitDefs();

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

	const UnitDetails &details = pUnitDefs->GetUnitDetails(unit);
	uint32 castleTerrain = map.GetTerrainAt(x, y);

	int movePenalty = pUnitDefs->GetMovementPenalty(details.movementClass, castleTerrain & 0xFF);
	if(movePenalty == 0 || movePenalty >= 100)
	{
		// the unit can't go on the castle, it must be a boat or something
		// find a patch of terrain around the castle where it can begin
		const int searchTable[12] = { 4, 8, 7, 11, 13, 14, 1, 2, 12, 15, 0, 3 };

		int width, height;
		map.GetMapSize(&width, &height);

		for(int i = 0; i < 12; ++i)
		{
			int tx = x - 1 + (searchTable[i] & 3);
			int ty = y - 1 + (searchTable[i] >> 2);

			if(tx < 0 || ty < 0 || tx >= width || ty >= height)
				continue;

			MapTile *pT = map.GetTile(tx, ty);

			if(pT->IsEnemyTile(player))
				continue;

			uint32 terrain = pT->GetTerrain();
			for(int j=0; j<4; ++j)
			{
				if(pUnitDefs->GetMovementPenalty(details.movementClass, terrain & 0xFF) != 0)
				{
					if((details.type == UT_Vehicle && pT->GetNumGroups() < 10) || pT->GetNumUnits() < 10)
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
			MapTile *pT = map.GetTile(x + (i & 1), y + (i >> 1));
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
		if(details.type == UT_Hero)
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
				history.PushCreateUnit(pUnit2);

				pGroup->AddUnit(pUnit2);
			}
			else
			{
				// find space on a new tile
				for(int i = 0; i < 4; ++i)
				{
					MapTile *pT = map.GetTile(x + (i & 1), y + (i >> 1));
					int numUnits = pT->GetNumUnits();
					if(numUnits < 10)
					{
						// create the second on the new tile
						Unit *pUnit2 = CreateUnit(unit, player);
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

bool GameState::CanCastleBuildHero(Castle *pCastle, int hero) const
{
	const ::Player &p = Player(pCastle->player);
	if(!p.pHero[hero] || !p.pHero[hero]->IsDead())
		return false;
	return p.pHeroReviveLocation[hero] == NULL || p.pHeroReviveLocation[hero] == pCastle;
}

void GameState::SetHeroRebuildCastle(int player, int hero, Castle *pCastle)
{
	Player(player).pHeroReviveLocation[hero] = pCastle;
}
