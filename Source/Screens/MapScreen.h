#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"

#include "Unit.h"

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
	Game *pGame;

	MFMaterial *pIcons;

	Button *pEndTurn;
	Button *pMiniMap;

	Group *pSelection;

	bool bMoving;
	float countdown;
};

#endif
