#if !defined(_LOGINSCREEN_H)
#define _LOGINSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"
#include "StringBox.h"

#include "MFFont.h"

class LoginScreen : public Screen
{
public:
	LoginScreen();
	virtual ~LoginScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

protected:
	MFMaterial *pIcons;
	MFFont *pFont;

	StringBox *pUsername;
	StringBox *pPassword;

	Button *pLogin, *pOffline;

	const char *pMessage;

	static void Login(int button, void *pUserData, int buttonID);
	static void Offline(int button, void *pUserData, int buttonID);

	static void TabUser(const char *pString, void *pUserData);
	static void TabPass(const char *pString, void *pUserData);
};

#endif
