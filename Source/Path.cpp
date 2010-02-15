#include "Warlords.h"
#include "Path.h"
#include "Map.h"

struct Cell
{
	uint16 from;
	uint16 gScore, hScore, fScore;
	uint8 x, y;
	uint8 open;

	void Set(int _x, int _y, int _gScore, int _hScore, int _fScore)
	{
		x = _x; y = _y;
		gScore = _gScore;
		hScore = _hScore;
		fScore = _fScore;
		from = 0;
		open = 1;
	}
};

Cell *gpSearchList = NULL;

void Path::Init(Map *_pMap)
{
	pMap = _pMap;

	int width, height;
	pMap->GetMapSize(&width, &height);
	gpSearchList = (Cell*)MFHeap_Alloc(sizeof(Cell)*width*height);

	stepPool.Init("Step pool", 8*1024);
}

void Path::Deinit()
{
	stepPool.Deinit();
	MFHeap_Free(gpSearchList);
	gpSearchList = NULL;
}

Step *Path::FindPath(Group *pGroup, int destX, int destY)
{
	Game *pGame = Game::GetCurrent();
	Map *pMap = pGame->GetMap();
	Tileset *pTileset = pMap->GetTileset();

	MapTile *pTile = pGroup->GetTile();
	int player = pGroup->GetPlayer();
	int numUnits = pGroup->GetNumUnits();
	int startX = pTile->GetX();
	int startY = pTile->GetY();

	// calculate the terrain penalties from the group
	int terrainPenalties[64];
	bool bRoadWalk = false;
	MFMemSet(terrainPenalties, 0xFF, sizeof(terrainPenalties));

	int numTerrainTypes = pTileset->GetNumTerrainTypes();

	Unit *pVehicle = pGroup->GetVehicle();
	if(pVehicle)
	{
		for(int a=0; a<numTerrainTypes; ++a)
			terrainPenalties[a] = pVehicle->GetTerrainPenalty(a);
		bRoadWalk = pVehicle->HasRoadWalk();
	}
	else
	{
		int numUnits = pGroup->GetNumUnits();
		for(int a=0; a<numUnits; ++a)
		{
			Unit *pUnit = pGroup->GetUnit(a);
			bRoadWalk = bRoadWalk || pUnit->HasRoadWalk();

			for(int b=0; b<numTerrainTypes; ++b)
			{
				if(terrainPenalties[b] != 0)
				{
					int penalty = pUnit->GetTerrainPenalty(b);
					if(penalty == 0)
						terrainPenalties[b] = 0;
					else
						terrainPenalties[b] = MFMax(terrainPenalties[b], penalty);
				}
			}
		}
	}

	int traversible[64];
	int numTraversible = 0;
	for(int a=0; a<numTerrainTypes; ++a)
	{
		if(terrainPenalties[a] != 0)
			traversible[numTraversible++] = a;
	}

	// check target is not the source
	if(startX == destX && startY == destY)
		return NULL;

	// check the unit can actually move to target
	//...

	// initialise the search data
	int width, height;
	pMap->GetMapSize(&width, &height);

	int dist = MFMax(MFAbs(destX - startX), MFAbs(destY - startY));
	gpSearchList[0].Set(startX, startY, 0, dist, dist);
	int item = 0;
	int numItems = 1;

	// start the search...
	while(1)
	{
		// get x from list
		int x = -1;
		int cf = 1 << 30;
		for(int a=0; a<numItems; ++a)
		{
			if(gpSearchList[a].open == 1 && gpSearchList[a].fScore < cf)
			{
				cf = gpSearchList[a].fScore;
				x = a;
			}
		}

		if(x == -1)
			break;

		Cell &item = gpSearchList[x];

		if(item.x == destX && item.y == destY)
		{
			// found the target!
			Step *pPath = NULL;

			while(x != 0)
			{
				Step *pStep = stepPool.Create();
				pStep->pNext = pPath;
				pPath = pStep;

				pStep->x = gpSearchList[x].x;
				pStep->y = gpSearchList[x].y;

				x = gpSearchList[x].from;
			}

			return pPath;
		}

		item.open = 2;

		uint32 roadDirections = 0;
		MapTile *pCurTile = pMap->GetTile(item.x, item.y);
		roadDirections = pCurTile->GetRoadDirections();

		uint32 terrain = pMap->GetTerrainAt(item.x, item.y);

		for(int ty = MFMax(item.y-1, 0); ty < MFMin(item.y+2, height); ++ty)
		{
			for(int tx = MFMax(item.x-1, 0); tx < MFMin(item.x+2, width); ++tx)
			{
				if(tx == item.x && ty == item.y)
					continue;

				bool bNonTraversible = true;

				// check if there is a road
				if(bRoadWalk && (((roadDirections & 8) && ty < item.y && tx == item.x) ||
					((roadDirections & 4) && ty > item.y && tx == item.x) ||
					((roadDirections & 2) && ty == item.y && tx < item.x) ||
					((roadDirections & 1) && ty == item.y && tx > item.x)))
					bNonTraversible = false;

				// check if unit can cross tile
				if(bNonTraversible)
				{
					// mask out the irrelevant quadrants
					uint32 mask = 0;
					if(tx < item.x)
						mask |= 0xFF00FF00;
					else if(tx > item.x)
						mask |= 0x00FF00FF;
					if(ty < item.y)
						mask |= 0xFFFF0000;
					else if(ty > item.y)
						mask |= 0x0000FFFF;

					// check if we can pass in the direction of travel
					for(int a=0; a<numTraversible; ++a)
					{
						if(TileContains(terrain|mask, traversible[a]))
						{
							bNonTraversible = false;
							break;
						}
					}

				}

				// if we can't move that way
				if(bNonTraversible)
					continue;

				int y = -1;
				for(int a=0; a<numItems; ++a)
				{
					if(tx == gpSearchList[a].x && ty == gpSearchList[a].y)
					{
						y = a;
						break;
					}
				}

				if(y == -1 || gpSearchList[y].open != 2)
				{
					// calculate the path score
					MapTile *pTile = pMap->GetTile(tx, ty);
					int terrainSpeed = GetMovementPenalty(pTile, terrainPenalties, player, bRoadWalk);
					int cornerPenalty = (tx != item.x) && (ty != item.y) ? 1 : 0;

					int tg = item.gScore + (terrainSpeed*2) + cornerPenalty;

					// avoid ruins
					if(pTile->GetType() == OT_Special)
						tg += 50;

					// avoid enemy tiles
					if(pTile->IsEnemyTile(player))
						tg += 50;

					// avoid full squares
					if(pTile->IsFriendlyTile(player) && pTile->GetNumUnits() + numUnits > 10)
						tg += 100;

					bool isBetter = false;
					if(y == -1)
					{
						y = numItems;
						gpSearchList[numItems++].Set(tx, ty, 0, MFMax(MFAbs(destX - tx), MFAbs(destY - ty)), 0);
						isBetter = true;
					}
					else if(tg < gpSearchList[y].gScore)
						isBetter = true;

					if(isBetter)
					{
						gpSearchList[y].from = x;
						gpSearchList[y].gScore = tg;
						gpSearchList[y].fScore = tg + gpSearchList[y].hScore;
					}
				}
			}
		}
	}

	// there is no path!
	return NULL;
}

int Path::GetMovementPenalty(MapTile *pTile, int *pTerrainPenalties, int player, bool bRoadWalk)
{
	if(bRoadWalk)
	{
		ObjectType type = pTile->GetType();
		if(type == OT_Road || (type == OT_Castle && pTile->IsFriendlyTile(player)))
			return 1;
	}

	uint32 terrain = pTile->GetTerrain();
	int penalty = pTerrainPenalties[terrain & 0xFF];
	penalty = MFMax(penalty, pTerrainPenalties[(terrain >> 8) & 0xFF]);
	penalty = MFMax(penalty, pTerrainPenalties[(terrain >> 16) & 0xFF]);
	penalty = MFMax(penalty, pTerrainPenalties[(terrain >> 24) & 0xFF]);
	return penalty;
}

Step *Path::StripStep(Step *pPath)
{
	Step *pNext = pPath->pNext;
	stepPool.Destroy(pPath);
	return pNext;
}

void Path::Destroy(Step *pPath)
{
	while(pPath)
	{
		Step *pNext = pPath->pNext;
		stepPool.Destroy(pPath);
		pPath = pNext;
	}
}

/*
function A*(start,goal)
     closedset := the empty set                 % The set of nodes already evaluated.
     openset := set containing the initial node % The set of tentative nodes to be evaluated.
     g_score[start] := 0                        % Distance from start along optimal path.
     h_score[start] := heuristic_estimate_of_distance(start, goal)
     f_score[start] := h_score[start]           % Estimated total distance from start to goal through y.
     while openset is not empty
         x := the node in openset having the lowest f_score[] value
         if x = goal
             return reconstruct_path(came_from,goal)
         remove x from openset
         add x to closedset
         foreach y in neighbor_nodes(x)
             if y in closedset
                 continue
             tentative_g_score := g_score[x] + dist_between(x,y)
             tentative_is_better := false
             if y not in openset
                 add y to openset
                 h_score[y] := heuristic_estimate_of_distance(y, goal)
                 tentative_is_better := true
             elseif tentative_g_score < g_score[y]
                 tentative_is_better := true
             if tentative_is_better = true
                 came_from[y] := x
                 g_score[y] := tentative_g_score
                 f_score[y] := g_score[y] + h_score[y]
     return failure
 
 function reconstruct_path(came_from,current_node)
     if came_from[current_node] is set
         p = reconstruct_path(came_from,came_from[current_node])
         return (p + current_node)
     else
         return the empty path
*/
