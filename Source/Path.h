#if !defined(_PATH_H)
#define _PATH_H

#include "MFPtrList.h"

class Map;

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

	Step *FindPath(int startX, int startY, int destX, int destY);
	void Destroy(Step *pPath);

private:
	Map *pMap;
	MFPtrListDL<Step> stepPool;
};

#endif
