#if !defined(_JOINGAMESCREEN_H)
#define _JOINGAMESCREEN_H

#include "Screen.h"
#include "Button.h"
#include "StringBox.h"
#include "ListBox.h"

#include "MFFont.h"

class JoinGameScreen : public Screen
{
public:
	JoinGameScreen();
	virtual ~JoinGameScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

protected:
	static const int MaxGames = 20;

	MFMaterial *pIcons;
	MFFont *pFont;

	StringBox *pGame;

	Button *pJoin, *pReturn;

	ListBox *pGames;

	const char *pMessage;

	GameLobby games[MaxGames];
	int numGames;

	GameDetails details;

	HTTPRequest search;
	HTTPRequest find;
	HTTPRequest join;

	void Change(const char *pName);
	void Click(int button, int buttonID);
	void SelectGame(int item);
	void Join(int item);
	void PopulateGames(HTTPRequest::Status status);
	void FindGame(HTTPRequest::Status status);
	void JoinGame(HTTPRequest::Status status);
};

#endif
