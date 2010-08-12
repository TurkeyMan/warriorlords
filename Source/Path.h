#if !defined(_PATH_H)
#define _PATH_H

#include "MFPoolHeap.h"

class Map;
class Group;
class MapTile;

struct Step
{
	bool CanMove() { return !!(flags & 1); }
	bool IsRoad() { return !!(flags & 2); }

	uint16 x, y;
	uint8 terrain;
	uint8 cost;
	uint8 icon;
	uint8 flags;
};

class Path
{
public:
	void Init(Map *pMap);
	void Deinit();

	bool FindPath(Group *pGroup, int destX, int destY);
	Step *StripStep(Step *pPath);
	void Destroy() { pathStart = MaxPath; }

	Step *GetPath() { return pathStart < MaxPath ? &path[pathStart] : NULL; }
	Step *GetStep(int step) { return pathStart < MaxPath ? &path[pathStart + step] : NULL; }
	Step *GetLast() { return pathStart < MaxPath ? &path[MaxPath-1] : NULL; }
	int GetPathLength() const { return MaxPath - pathStart; }

	bool IsEnd() const { return GetPathLength() == 1; }

private:
	static const int MaxPath = 1024;

	Map *pMap;

	Step path[MaxPath];
	int pathStart;
};

#endif
