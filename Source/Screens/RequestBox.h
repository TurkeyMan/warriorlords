#if !defined(_REQUESTBOX_H)
#define _REQUESTBOX_H

#include "Window.h"

class RequestBox : public Window
{
public:
	typedef FastDelegate1<int> SelectCallback;

	RequestBox();
	virtual ~RequestBox();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(const char *pMessage, SelectCallback selectCallback, bool bNotification);
	virtual void Hide();

protected:
	char message[1024];

	SelectCallback selectCallback;
	Button *pYes, *pNo;

	bool bNotification;

	void Select(int button, int buttonID);
};

#endif
