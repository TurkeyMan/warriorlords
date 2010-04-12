#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Window.h"
#include "Game.h"
#include "Button.h"
#include "ListBox.h"

#include "Unit.h"

#include "MFFont.h"

#include "CastleConfig.h"
#include "GroupConfig.h"
#include "MiniMap.h"

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
	static void FinishTurn(int selection, void *pUserData);
	static void ShowMiniMap(int button, void *pUserData, int buttonID);
	static void UndoMove(int button, void *pUserData, int buttonID);

	void SelectGroup(Group *pGroup);
	void DeselectGroup();
	Group *GetSelected();

protected:
	GroupConfig groupConfig;
	CastleConfig castleConfig;
	MiniMap miniMap;

	Game *pGame;
	Map *pMap;

	MFMaterial *pIcons;
	MFFont *pFont;
	MFFont *pSmallNumbers;

	Button *pEndTurn;
	Button *pMiniMap;
	Button *pUndo;

	Group *pSelection;

	bool bMoving;
	float countdown;
};

#endif
