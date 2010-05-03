#if !defined(_GROUPCONFIG_H)
#define _GROUPCONFIG_H

#include "Window.h"
#include "UnitConfig.h"

class GroupConfig : public Window
{
public:
	GroupConfig();
	virtual ~GroupConfig();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(MapTile *pTile);
	virtual void Hide();

protected:
	UnitConfig battleConfig;

	MapTile *pTile;

	MFRect top, rear, front, lower, bottom[4], empty;

	struct GroupUnit
	{
		int x, y;
		int rank;
		Unit *pUnit;
		Group *pGroup;
	} units[MapTile::MaxUnitsOnTile * 2];

	int numUnits;
	int numFront;
	int numRear;
	int numExtraGroups;
	Group *pExtraGroups[4];

	int dragUnit;
	Group *pDragGroup;

	void PositionUnits();
	int GetUnitFromPoint(float x, float y);
	int GetFileFromPoint(float x, float y);
	int GetGroupFromPoint(float x, float y);
};

#endif
