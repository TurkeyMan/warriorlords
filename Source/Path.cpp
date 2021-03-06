#include "Warlords.h"
#include "Path.h"
#include "Map.h"
#include "Group.h"

void Path::Init(Map *_pMap)
{
	pMap = _pMap;

	int width, height;
	pMap->GetMapSize(&width, &height);
	pSearchList = (Cell*)MFHeap_Alloc(sizeof(Cell)*width*height);

	pathStart = MaxPath;
}

void Path::Deinit()
{
	MFHeap_Free(pSearchList);
	pSearchList = NULL;
}

bool Path::FindPath(Group *pGroup, int destX, int destY)
{
	Map &map = *pMap;
	const Tileset &tileset = map.Tileset();

	MapTile *pTile = pGroup->GetTile();
	int player = pGroup->GetPlayer();
	int numUnits = pGroup->GetNumUnits();
	int startX = pTile->GetX();
	int startY = pTile->GetY();

	// calculate the terrain penalties from the group
	int terrainPenalties[64];
	bool bRoadWalk = false;
	MFMemSet(terrainPenalties, 0xFF, sizeof(terrainPenalties));

	int numTerrainTypes = tileset.NumTerrainTypes();

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
	map.GetMapSize(&width, &height);

	int dist = MFMax(MFAbs(destX - startX), MFAbs(destY - startY));
	pSearchList[0].Set(startX, startY, 0, dist, dist);
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
			if(pSearchList[a].open == 1 && pSearchList[a].fScore < cf)
			{
				cf = pSearchList[a].fScore;
				x = a;
			}
		}

		if(x == -1)
			break;

		Cell &item = pSearchList[x];

		if(item.x == destX && item.y == destY)
		{
			// found the target!
			pathStart = MaxPath;

			while(x != 0)
			{
				Step &step = path[--pathStart];

				step.x = pSearchList[x].x;
				step.y = pSearchList[x].y;
				step.flags = pSearchList[x].traversible ? 4 : 0;

				x = pSearchList[x].from;
			}

			// calculate the movement distance
			struct UnitMove
			{
				Unit *pUnit;
				int movement;
			} unitMove[10];

			int numUnits = 1;
			unitMove[0].pUnit = pGroup->GetVehicle();
			if(unitMove[0].pUnit)
			{
				unitMove[0].movement = unitMove[0].pUnit->GetMovement();
			}
			else
			{
				numUnits = pGroup->GetNumUnits();

				for(int a=0; a<numUnits; ++a)
				{
					unitMove[a].pUnit = pGroup->GetUnit(a);
					unitMove[a].movement = unitMove[a].pUnit->GetMovement();
				}
			}

			for(int p = pathStart; p < MaxPath; ++p)
			{
				Step *pT = &path[p];

				MapTile *pTile = pMap->GetTile(pT->x, pT->y);

				pT->cost = 0;

				uint32 canMove = 1;
				for(int a=0; a<numUnits; ++a)
				{
					int terrain;
					int penalty = unitMove[a].pUnit->GetMovementPenalty(pTile, &terrain);
					unitMove[a].movement -= penalty;
					if(!penalty || unitMove[a].movement < 0)
						canMove = 0;

					if(penalty > pT->cost)
					{
						pT->cost = penalty;
						pT->terrain = terrain;
						if(penalty == 1)
							pT->flags |= 2;
					}
				}
				pT->flags |= canMove;

				pT->icon = pT->terrain;
				if(!(pT->flags & 1))
					pT->icon = 14;
				else if(pT->flags & 2)
					pT->icon = 13;
				else if(pTile->IsEnemyTile(unitMove[0].pUnit->GetPlayer()))
					pT->icon = 15;
			}

			return true;
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
//				if(bNonTraversible)
//					continue;

				int y = -1;
				for(int a=0; a<numItems; ++a)
				{
					if(tx == pSearchList[a].x && ty == pSearchList[a].y)
					{
						y = a;
						break;
					}
				}

				if(y == -1 || pSearchList[y].open != 2)
				{
					// calculate the path score
					MapTile *pTile = pMap->GetTile(tx, ty);
					int terrainSpeed = !bNonTraversible ? Map::GetMovementPenalty(pTile, terrainPenalties, player, bRoadWalk) : 1000;
					int cornerPenalty = (tx != item.x) && (ty != item.y) ? 1 : 0;

					int tg = item.gScore + (terrainSpeed*2) + cornerPenalty;

					// avoid ruins
					if(pTile->GetType() == OT_Place)
						tg += 50;

					// avoid enemy tiles
					if(pTile->IsEnemyTile(player))
						tg += 50;

					// avoid full squares
					if(pTile->IsFriendlyTile(player) && pTile->GetNumUnits() + numUnits > MapTile::MaxUnitsOnTile)
						tg += 100;

					bool isBetter = false;
					if(y == -1)
					{
						y = numItems;
						pSearchList[numItems++].Set(tx, ty, 0, MFMax(MFAbs(destX - tx), MFAbs(destY - ty)), 0);
						isBetter = true;
					}
					else if(tg < pSearchList[y].gScore)
						isBetter = true;

					if(isBetter)
					{
						pSearchList[y].from = x;
						pSearchList[y].gScore = tg;
						pSearchList[y].fScore = tg + pSearchList[y].hScore;
						pSearchList[y].traversible = bNonTraversible ? 0 : 1;
					}
				}
			}
		}
	}

	// there is no path!
	return NULL;
}

Step *Path::StripStep(Step *pPath)
{
	pathStart = (int)(++pPath - path);

	if(pathStart == MaxPath)
		return NULL;

	return pPath;
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
