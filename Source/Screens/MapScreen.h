#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"

#include "Unit.h"

#include "MFFont.h"

class GroupConfig : public InputReceiver
{
public:
	GroupConfig();
	~GroupConfig();

	void Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(MapTile *pTile);
	void Hide();

protected:
	MapTile *pTile;

	MFFont *pFont;

	MFRect window, top, rear, front, bottom;

	struct GroupEdit
	{
		int group;
		int numFront, numRear;
		Group *pGroup;
	} group[20];

	struct GroupUnit
	{
		int x, y;
		int rank;
		Unit *pUnit;
		GroupEdit *pGroup;
	} units[10];

	int numGroups;
	int numUnits;

	bool bVisible;
};

class CastleConfig : public InputReceiver
{
public:
	CastleConfig();
	~CastleConfig();

	void Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(Castle *pCastle);
	void Hide();

protected:
	Castle *pCastle;

	MFFont *pFont;

	MFRect window, title, units, lower, right;

	Button *pBuildUnits[4];


	bool bVisible;

	static void SelectUnit(int button, void *pUserData, int buttonID);
};

class MapScreen : public Screen
{
public:
	MapScreen(Game *pGame);
	virtual ~MapScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	virtual int UpdateInput();

	static void EndTurn(int button, void *pUserData, int buttonID);
	static void ShowMiniMap(int button, void *pUserData, int buttonID);

	void SelectGroup(Group *pGroup);
	void DeselectGroup(Group *pGroup);
	Group *GetSelected();

protected:
	GroupConfig groupConfig;
	CastleConfig castleConfig;

	Game *pGame;

	MFMaterial *pIcons;
	MFFont *pFont;

	Button *pEndTurn;
	Button *pMiniMap;

	Group *pSelection;

	bool bMoving;
	float countdown;
};

#endif
