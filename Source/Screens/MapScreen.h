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

	void EndTurn(int button, int buttonID);
	void FinishTurn(int selection);
	void ShowMiniMap(int button, int buttonID);
	void UndoMove(int button, int buttonID);

	void SelectGroup(Group *pGroup);
	Group *GetSelected();

	void ShowCastleConfig(Castle *pCastle);
	void ShowUndoButton();

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

	bool bUndoVisible;

	Group *pSelection;

	bool bMoving;
	float countdown;

	Castle *pShowCastle;

	float lastUpdateTime;
};

#endif
