#if !defined(_HOMESCREEN_H)
#define _HOMESCREEN_H

#include "Screen.h"
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
	void ResumeGame(uint32 game);

	MFMaterial *pIcons;
	MFFont *pFont;

	ListBox *pWaiting;
	ListBox *pMyGames;

	Button *pCreate, *pJoin, *pFind, *pOffline, *pContinue;

	bool bOffline;
	Session *pSession;

	uint32 continueGame;

	static void CreateGame(int button, void *pUserData, int buttonID);
	static void JoinGame(int button, void *pUserData, int buttonID);
	static void FindGame(int button, void *pUserData, int buttonID);
	static void ContinueGame(int button, void *pUserData, int buttonID);

	static void SelectPending(int item, void *pUserData);
	static void SelectGame(int item, void *pUserData);

	static void EnterGame(int item, void *pUserData);
	static void EnterPending(int item, void *pUserData);
};

#endif
