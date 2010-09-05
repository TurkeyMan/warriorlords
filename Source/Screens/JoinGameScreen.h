#if !defined(_JOINGAMESCREEN_H)
#define _JOINGAMESCREEN_H

#include "Screen.h"
#include "Button.h"
#include "StringBox.h"

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
	MFMaterial *pIcons;
	MFFont *pFont;

	StringBox *pGame;

	Button *pJoin, *pReturn;

	const char *pMessage;

	static void Click(int button, void *pUserData, int buttonID);
};

#endif