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

	HTTPRequest request;

	void CreateGame(int button, int buttonID);
	void JoinGame(int button, int buttonID);
	void FindGame(int button, int buttonID);
	void ContinueGame(int button, int buttonID);

	void SelectPending(int item);
	void SelectGame(int item);

	void EnterGame(int item);
	void EnterPending(int item);

	void Resume(HTTPRequest::Status status);
	void UpdateSession(ServerError error, Session *pSession);
};

#endif
