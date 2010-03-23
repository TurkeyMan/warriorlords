#if !defined(_WINDOW_H)
#define _WINDOW_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"

class Window : public InputReceiver
{
public:
	Window(bool bCloseButton);
	virtual ~Window();

	virtual bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show();
	virtual void Hide();

protected:
	MFRect window;
	Button *pCloseButton;
	MFMaterial *pIcons;

	bool bVisible;
	bool bCloseButton;

	static void CloseWindow(int button, void *pUserData, int buttonID);
};

#endif
