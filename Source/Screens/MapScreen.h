#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Window.h"
#include "Game.h"
#include "Button.h"
#include "ListBox.h"

#include "Unit.h"

#include "MFFont.h"

class GroupConfig;

class Inventory : public InputReceiver
{
public:
	Inventory();
	~Inventory();

	bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(Unit *pUnit);
	void Hide();

protected:
	Unit *pUnit;

	MFMaterial *pItems;
	MFFont *pFont;

	MFRect window;

	Button *pInventory[8];
	int numItems;

	int selected;

	bool bVisible;

	static void SelectItem(int button, void *pUserData, int buttonID);
};

class UnitConfig : public InputReceiver
{
public:
	UnitConfig();
	~UnitConfig();

	bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Show(Unit *pUnit);
	void Hide();

protected:
	Inventory inventory;

	Unit *pUnit;

	MFMaterial *pIcons;
	MFFont *pFont;

	MFRect window, top, bottom;

	Button *pInventory;
	CheckBox *pStrategySelect[6];

	bool bVisible;

	static void ShowInventory(int button, void *pUserData, int buttonID);
	static void SelectStrat(int value, void *pUserData, int buttonID);
};

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
	UnitConfig battleConfig;

	MapTile *pTile;

	MFFont *pFont;

	MFRect window, top, rear, front, lower, bottom[4], empty;

	struct GroupUnit
	{
		int x, y;
		int rank;
		Unit *pUnit;
		Group *pGroup;
	} units[20];

	int numUnits;
	int numFront;
	int numRear;
	int numExtraGroups;
	Group *pExtraGroups[4];

	int dragUnit;
	Group *pDragGroup;

	bool bVisible;

	void PositionUnits();
	int GetUnitFromPoint(float x, float y);
	int GetFileFromPoint(float x, float y);
	int GetGroupFromPoint(float x, float y);
};

class CastleConfig : public Window
{
public:
	CastleConfig();
	virtual ~CastleConfig();

	virtual bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Castle *pCastle);
	virtual void Hide();

protected:
	Castle *pCastle;

	MFFont *pFont;

	MFRect window, title, units, lower, right;

	Button *pBuildUnits[4];
	int numBuildUnits;

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
	void DeselectGroup();
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
