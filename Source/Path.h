#if !defined(_PATH_H)
#define _PATH_H

#include "MFPtrList.h"

class Map;
class Group;
class MapTile;

struct Step
{
	uint16 x, y;
	Step *pNext;
};

class Path
{
public:
	void Init(Map *pMap);
	void Deinit();

	Step *FindPath(Group *pGroup, int destX, int destY);
	Step *StripStep(Step *pPath);
	void Destroy(Step *pPath);

	int GetMovementPenalty(MapTile *pTile, int *pTerrainPenalties, int player, bool bRoadWalk);

private:
	Map *pMap;
	MFPtrListDL<Step> stepPool;
};

#endif
