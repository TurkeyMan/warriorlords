#if !defined(_LOGINSCREEN_H)
#define _LOGINSCREEN_H

#include "Screen.h"
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
	StringBox *pEmail;

	Button *pLogin, *pOffline, *pNew, *pCreate, *pReturn;

	const char *pPrompt;
	const char *pMessage;

	int state;

	bool bAutoLogin;

	HTTPRequest createRequest;

	void AutoLogin();

	void Click(int button, int buttonID);

	void TabUser(const char *pString);
	void TabPass(const char *pString);
	void TabEmail(const char *pString);
	void CreateComplete(HTTPRequest::Status status);
	void LoginScreen::OnLogin(ServerError err, Session *pSession);
};

#endif
