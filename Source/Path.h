#if !defined(_PATH_H)
#define _PATH_H

class Map;
class Group;
class MapTile;

struct Step
{
	bool CanMove() { return !!(flags & 1); }
	bool InvalidMove() { return !(flags & 4); }
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
	Step *GetStep(int step) { return pathStart + step < MaxPath ? &path[pathStart + step] : NULL; }
	Step *GetLast() { return pathStart < MaxPath ? &path[MaxPath-1] : NULL; }
	int GetPathLength() const { return MaxPath - pathStart; }

	bool IsEnd() const { return GetPathLength() == 1; }

private:
	struct Cell
	{
		uint16 from;
		uint16 gScore, hScore, fScore;
		uint8 x, y;
		uint8 open;
		uint8 traversible;

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

	static const int MaxPath = 1024;

	Map *pMap;
	Cell *pSearchList;

	Step path[MaxPath];
	int pathStart;
};

#endif
