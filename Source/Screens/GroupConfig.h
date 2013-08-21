#if !defined(_GROUPCONFIG_H)
#define _GROUPCONFIG_H

#include "Window.h"
#include "UnitConfig.h"

class GroupConfig : public Window
{
public:
	GroupConfig(Game *pGame);
	virtual ~GroupConfig();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Game *pGame, MapTile *pTile);
	virtual void Hide();

protected:
	UnitConfig battleConfig;

	MapTile *pTile;
	const UnitDefinitions *pDefs;

	MFMaterial *pMelee, *pRanged;

	MFArray<UnitRender> renderUnits;

	MFRect top, rear, front, lower, bottom[4], empty;

	struct UnitGroup
	{
		union
		{
			Unit *pUnits[11];
			struct
			{
				Unit *pForward[5];
				Unit *pRear[5];
				Unit *pVehicle;
			};
		};
		int numForward;
		int numRear;
		int totalUnits;
	} groups[MapTile::MaxUnitsOnTile * 2];

	UnitGroup *pGroups[MapTile::MaxUnitsOnTile * 2];
	int numGroups;

	int dragUnit;
	int dragGroup;

	int dragX, dragY;

//	void PositionUnits();
	int GetUnitFromPoint(float x, float y);
	int GetFileFromPoint(float x, float y);
	int GetGroupFromPoint(float x, float y);

	bool GetUnitPos(int group, int unit, MFRect *pRect);
	void GetBottomPanel(int i, MFRect *pRect);
};

#endif
