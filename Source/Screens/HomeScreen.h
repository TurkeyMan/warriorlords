#if !defined(_HOMESCREEN_H)
#define _HOMESCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"
#include "ListBox.h"

#include "MFFont.h"

class HomeScreen : public Screen
{
public:
	HomeScreen();
	virtual ~HomeScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

protected:
	MFMaterial *pIcons;
	MFFont *pFont;

	ListBox *pMyTurn;
	ListBox *pMyGames;

	Button *pCreate, *pJoin, *pFind, *pOffline, *pContinue;

	static void CreateGame(int button, void *pUserData, int buttonID);
	static void JoinGame(int button, void *pUserData, int buttonID);
	static void FindGame(int button, void *pUserData, int buttonID);
	static void OfflineGame(int button, void *pUserData, int buttonID);
	static void ContinueGame(int button, void *pUserData, int buttonID);
};

#endif
