#include "Warlords.h"
#include "Map.h"
#include "Group.h"

uint32 MapTile::GetTerrain() const
{
	return pMap->GetTerrain(terrain);
}

void MapTile::AddGroup(Group *_pGroup)
{
	MFDebug_Assert(_pGroup->ValidateGroup(), "EEK!");

	_pGroup->pTile = this;
	_pGroup->x = x;
	_pGroup->y = y;
	_pGroup->pNext = pGroup;
	pGroup = _pGroup;
}

void MapTile::AddGroupToBack(Group *_pGroup)
{
	MFDebug_Assert(_pGroup->ValidateGroup(), "EEK!");

	_pGroup->pTile = this;
	_pGroup->x = x;
	_pGroup->y = y;
	_pGroup->pNext = NULL;

	if(!pGroup)
	{
		pGroup = _pGroup;
	}
	else
	{
		Group *pT = pGroup;
		while(pT->pNext)
			pT = pT->pNext;
		pT->pNext = _pGroup;
	}
}

void MapTile::RemoveGroup(Group *_pGroup)
{
	if(!pGroup || !_pGroup || _pGroup->pTile != this)
		return;

	if(pGroup == _pGroup)
	{
		pGroup = pGroup->pNext;
	}
	else
	{
		Group *pG = pGroup;
		while(pG->pNext && pG->pNext != _pGroup)
			pG = pG->pNext;
		if(pG->pNext)
			pG->pNext = pG->pNext->pNext;
	}

	_pGroup->pNext = NULL;
	_pGroup->pTile = NULL;
}

int MapTile::GetNumGroups() const
{
	int groups = 0;
	for(Group *pG = pGroup; pG; pG = pG->pNext)
		++groups;
	return groups;
}

const Group *MapTile::GetGroup(int group) const
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		if(!group--)
			return pG;
	}
	return NULL;
}

Group *MapTile::FindUnitGroup(Unit *pUnit)
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		if(pG->IsInGroup(pUnit))
			return pG;
	}
	return NULL;
}

Unit *MapTile::FindVehicle()
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		Unit *pVehicle = pG->GetVehicle();
		if(pVehicle)
			return pVehicle;
	}
	return NULL;
}

void MapTile::BringGroupToFront(Group *_pGroup)
{
	RemoveGroup(_pGroup);
	_pGroup->pTile = this;
	_pGroup->pNext = pGroup;
	pGroup = _pGroup;
}

int MapTile::GetNumUnits() const
{
	int units = 0;
	for(Group *pG = pGroup; pG; pG = pG->pNext)
		units += pG->GetNumUnits();
	return units;
}

const Unit *MapTile::GetUnit(int unit) const
{
	for(Group *pG = pGroup; pG; pG = pG->pNext)
	{
		int numUnits = pG->GetNumUnits();
		if(unit < numUnits)
			return pG->GetUnit(unit);
		unit -= numUnits;
	}
	return NULL;
}

Castle *MapTile::GetCastle()
{
	if(type == OT_Castle)
		return pMap->GetCastle(index);
	return NULL;
}

const Castle *MapTile::GetCastle() const
{
	if(type == OT_Castle)
		return pMap->GetCastle(index);
	return NULL;
}

Place *MapTile::GetPlace()
{
	if(type == OT_Place)
		return pMap->GetPlace(index);
	return NULL;
}

const Place *MapTile::GetPlace() const
{
	if(type == OT_Place)
		return pMap->GetPlace(index);
	return NULL;
}

bool MapTile::IsFriendlyTile(int player) const
{
	if(pGroup)
		return pGroup->player == player;
	const Castle *pCastle = GetCastle();
	if(pCastle)
		return pCastle->player == player;
	return false;
}

bool MapTile::IsEnemyTile(int player) const
{
	if(pGroup)
		return pGroup->player != player;
	const Castle *pCastle = GetCastle();
	if(pCastle)
		return pCastle->player != player;
	return false;
}

bool MapTile::CanMove(Group *_pGroup) const
{
	if(type == OT_Place)
		return false;
	if(IsEnemyTile(_pGroup->player))
		return false;
	if(pGroup)
		return _pGroup->GetNumUnits() <= GetAvailableUnitSpace();
	return true;
}

bool MapTile::IsRoad() const
{
	return type == OT_Road;
}

int MapTile::GetPlayer() const
{
	if(pGroup)
		return pGroup->GetPlayer();
	if(type == OT_Castle)
		return GetCastle()->GetPlayer();
//	if(type == OT_Flag)
//		return (int8)index;
	return -2;
}

Map::Map(MapTemplate &mapTemplate, ::GameState &gameState)
: mapTemplate(mapTemplate)
, gameState(gameState)
, width(mapTemplate.mapWidth)
, height(mapTemplate.mapHeight)
{
	// allocate the runtime map data
	pMap = (MapTile*)MFHeap_AllocAndZero(sizeof(MapTile) * width * height);

	for(int a=0; a<width * height; ++a)
	{
		pMap[a].x = a % width;
		pMap[a].y = a / width;
	}

	path.Init(this);
}

Map::~Map()
{
	path.Deinit();

	MFHeap_Free(pMap);

	mapTemplate.Release();
}

void Map::ConstructMap(int race)
{
	// check race has map defined
	if(race > -1 && !mapTemplate.templates[race].pMap)
		race = 0; // set the template for undefined maps

	// copy the map data
	int numTiles = width * height;
	for(int a=0; a<numTiles; ++a)
	{
		pMap[a].pMap = this;

		pMap[a].region = mapTemplate.pRegions[a];

		int slice = race;
		if(race == -1)
			slice = pMap[a].region == 0xF ? 0 : gameState.Player(pMap[a].region).race;

		pMap[a].terrain = mapTemplate.templates[slice].pMap[a].terrain;
		pMap[a].type = mapTemplate.templates[slice].pMap[a].type;
		pMap[a].index = mapTemplate.templates[slice].pMap[a].index;

		if(pMap[a].type == OT_Place)
		{
			int placeId = places.size();

			Place &place = places.push();
			place.id = placeId;
			place.pUnitDefs = UnitDefs();
			place.pTile = &pMap[a];
			place.pSpecial = &UnitDefs()->GetSpecial(pMap[a].index);

			// THIS MUST COME AFTER pSpecial IS ASSIGNED!
			pMap[a].index = placeId;

			if(place.GetType() == Special::ST_Recruit)
			{
				place.recruit.recruiting = -1;
			}
		}
	}

	// update the surrounding squares for multi-square places
	for(size_t a=0; a<places.size(); ++a)
	{
		Place *pPlace = GetPlace(a);
		const Special *pDesc = pPlace->GetPlaceDesc();
		MapTile *pTile = pPlace->pTile;

		for(int y=0; y<pDesc->height; ++y)
		{
			for(int x=0; x<pDesc->width; ++x)
			{
				MapTile &tile = pTile[y*width + x];
				tile.type = pTile->type;
				tile.index = pTile->index;
				tile.objectX = x;
				tile.objectY = y;
			}
		}
	}

	// build the castle list
	for(int a=0; a<16; ++a)
	{
		for(size_t c=0; c<mapTemplate.templates[a].castles.size(); ++c)
		{
			MapTile *pTile = GetTile(mapTemplate.templates[a].castles[c].x, mapTemplate.templates[a].castles[c].y);

			int slice = race;
			if(race == -1)
				slice = pTile->region == 0xF ? 0 : gameState.Player(pTile->region).race;

			if(slice == a)
			{
				int castleID = castles.size();
				Castle &castle = castles.push();

				castle.Init(castleID, mapTemplate.templates[a].castles[c], mapTemplate.templates[a].castles[c].bCapital ? pTile->GetRegion() : -1);
				castle.pTile = pTile;

				for(int a=0; a<castle.details.numBuildUnits; ++a)
				{
					const UnitDetails &unitDetails = UnitDefs()->GetUnitDetails(castle.details.buildUnits[a].unit);
					castle.details.buildUnits[a].buildTime += unitDetails.buildTime;
				}

				for(int a=0; a<4; ++a)
				{
					MapTile &tile = pTile[(a >> 1)*width + (a & 1)];
					tile.type = OT_Castle;
					tile.index = castleID;
					tile.objectX = a & 1;
					tile.objectY = a >> 1;
				}
			}
		}
	}
}

int Map::GetMovementPenalty(const MapTile *pTile, int *pTerrainPenalties, int player, bool bRoadWalk, int *pTerrainType)
{
	if(bRoadWalk)
	{
		ObjectType type = pTile->GetType();
		if(type == OT_Road || (type == OT_Castle && pTile->IsFriendlyTile(player)))
			return 1;
	}

	// find which terrain is most prominant
	uint32 terrain = pTile->GetTerrain();
	struct { int t, c; } t[4];
	int numT = 0;
	for(int a=0; a<4; ++a, terrain >>= 8)
	{
		int ct = terrain & 0xFF;
		if(pTerrainPenalties[ct] == 0)
			continue;

		for(int b=0; b<numT; ++b)
		{
			if(ct == t[b].t)
			{
				++t[b].c;
				goto added;
			}
		}

		t[numT].t = ct;
		t[numT++].c = 1;
added:
		continue;
	}

	// get the most prominant terrain penalty
	int penalty = 0;
	int c = 0;
	for(int a=0; a<numT; ++a)
	{
		if(t[a].c > c)
		{
			penalty = pTerrainPenalties[t[a].t];
			if(pTerrainType)
				*pTerrainType = t[a].t;
			c = t[a].c;
		}
		else if(t[a].c == c)
		{
			if(pTerrainPenalties[t[a].t] > penalty)
			{
				penalty = pTerrainPenalties[t[a].t];
				if(pTerrainType)
					*pTerrainType = t[a].t;
			}
		}
	}

	return penalty;
}

void Map::ClaimFlags(int x, int y, int player)
{
	int startX = MFMax(x - 2, 0);
	int startY = MFMax(y - 2, 0);
	int endX = MFMin(x + 3, width);
	int endY = MFMin(y + 3, height);

	for(int y = startY; y < endY; ++y)
	{
		for(int x = startX; x < endX; ++x)
		{
			MapTile *pTile = GetTile(x, y);
			if(pTile->type == OT_Flag)
				pTile->index = player;
		}
	}
}

ObjectType Map::GetDetailType(int x, int y) const
{
	return (ObjectType)pMap[y*width + x].type;
}

int Map::GetDetail(int x, int y) const
{
	if(pMap[y*width + x].type)
		return pMap[y*width + x].index;

	// check if we are in the space of an oversize castle
	if(x > 0 && pMap[y*width + x-1].type == OT_Castle)
		return pMap[y*width + x-1].index;
	if(y > 0 && pMap[(y-1)*width + x].type == OT_Castle)
		return pMap[(y-1)*width + x].index;
	if(x > 0 && y > 0 && pMap[(y-1)*width + x-1].type == OT_Castle)
		return pMap[(y-1)*width + x-1].index;

	// nothing there
	return 0;
}

Path *Map::FindPath(Group *pGroup, int destX, int destY)
{
	return path.FindPath(pGroup, destX, destY) ? &path : NULL;
}
