#include "Warlords.h"
#include "Path.h"
#include "Map.h"

void Path::Init(Map *_pMap)
{
	pMap = _pMap;

	int width, height;
	pMap->GetMapSize(&width, &height);

	stepPool.Init("Step pool", 1024);
}

void Path::Deinit()
{
	stepPool.Deinit();
}

Step *Path::FindPath(int startX, int startY, int destX, int destY)
{
	// check target is not the source
	if(startX == destX && startY == destY)
		return NULL;

	// initialise the search data
	int width, height;
	pMap->GetMapSize(&width, &height);

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

	Cell searchList[1*1024];
	int dist = MFMax(MFAbs(destX - startX), MFAbs(destY - startY));
	searchList[0].Set(startX, startY, 0, dist, dist);
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
			if(searchList[a].open == 1 && searchList[a].fScore < cf)
			{
				cf = searchList[a].fScore;
				x = a;
			}
		}

		if(x == -1)
			break;

		Cell &item = searchList[x];

		if(item.x == destX && item.y == destY)
		{
			// found the target!
			Step *pPath = NULL;

			while(x != 0)
			{
				Step *pStep = stepPool.Create();
				pStep->pNext = pPath;
				pPath = pStep;

				pStep->x = searchList[x].x;
				pStep->y = searchList[x].y;

				x = searchList[x].from;
			}

			return pPath;
		}

		item.open = 2;

		for(int ty = MFMax(item.y-1, 0); ty < MFMin(item.y+2, height); ++ty)
		{
			for(int tx = MFMax(item.x-1, 0); tx < MFMin(item.x+2, width); ++tx)
			{
				if(tx == x && ty == ty)
					continue;

				if(1)// if not flying...
				{
					// check our neighbour does not cross water
					// TODO: add a rule to allow crossing water if there is a bridge!
					uint32 mask = 0xFFFFFFFF;
					if(tx < item.x)
						mask &= 0x00FF00FF;
					else if(tx > item.x)
						mask &= 0xFF00FF00;
					if(ty < item.y)
						mask &= 0x0000FFFF;
					else if(ty > item.y)
						mask &= 0xFFFF0000;

					if(1) // if land unit
					{
						// if we have water on the neighbouring edge, we can't traverse
						if((pMap->GetTerrain(item.x, item.y) & mask) == (0x01010101 & mask))
							continue;
					}
//					else // if sea unit
//					{
//						// if we don't have water on the neighbouring edge, we can't traverse
//						if((pMap->GetTerrain(item.x, item.y) & mask) != (0x01010101 & mask))
//							continue;
//					}
				}

				int y = -1;
				for(int a=0; a<numItems; ++a)
				{
					if(tx == searchList[a].x && ty == searchList[a].y)
					{
						y = a;
						break;
					}
				}

				if(y == -1 || searchList[y].open != 2)
				{
					int tg = item.gScore + (pMap->GetDetailType(tx, ty) == OT_Road ? 1 : pMap->GetTile(tx, ty)->speed * 2);
					bool isBetter = false;

					if(y == -1)
					{
						y = numItems;
						searchList[numItems++].Set(tx, ty, 0, MFMax(MFAbs(destX - tx), MFAbs(destY - ty)), 0);
						isBetter = true;
					}
					else if(tg < searchList[y].gScore)
						isBetter = true;

					if(isBetter)
					{
						searchList[y].from = x;
						searchList[y].gScore = tg;
						searchList[y].fScore = tg + searchList[y].hScore;
					}
				}
			}
		}
	}

	// there is no path!
	return NULL;
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
