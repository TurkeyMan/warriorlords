#if !defined(_MENUSCREEN_H)
#define _MENUSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"
#include "ListBox.h"

#include "MFFont.h"

class MenuScreen : public Screen
{
public:
	MenuScreen();
	virtual ~MenuScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void SetGameType(int type) { gameType = type; }

protected:
	struct MapData
	{
		char name[64];
		MapDetails details;
		bool bDetailsLoaded;
		MapData * pNext;
	};

	MFMaterial *pIcons;
	MFFont *pFont;

	ListBox *pMapList;
	MapData *pMaps;

	Button *pStart, *pEdit, *pReturn;

	int gameType;

	static void Click(int button, void *pUserData, int buttonID);
	static void SelectMap(int item, void *pUserData);
};

#endif
