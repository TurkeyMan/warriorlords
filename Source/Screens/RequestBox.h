#if !defined(_REQUESTBOX_H)
#define _REQUESTBOX_H

#include "Window.h"

class RequestBox : public Window
{
public:
	typedef void (SelectCallback)(int selection);

	RequestBox();
	virtual ~RequestBox();

	virtual bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(const char *pMessage, SelectCallback *pSelectCallback);
	virtual void Hide();

protected:
	char message[1024];

	SelectCallback *pSelectCallback;
	Button *pYes, *pNo;

	static void Select(int button, void *pUserData, int buttonID);
};

#endif
