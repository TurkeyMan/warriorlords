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

protected:
	MFMaterial *pIcons;
	MFFont *pFont;

	ListBox *pListBox;

	Button *pStart, *pEdit;

	static void StartGame(int button, void *pUserData, int buttonID);
	static void EditMap(int button, void *pUserData, int buttonID);
};

#endif
