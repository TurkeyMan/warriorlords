#if !defined(_WINDOW_H)
#define _WINDOW_H

#include "Game.h"
#include "Button.h"

class Window : public InputReceiver
{
public:
	Window(Game *pGame, bool bCloseButton);
	virtual ~Window();

	virtual bool Draw();
	virtual bool DrawContent() { return true; }

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show();
	virtual void Hide();

	void SetWindowSize(float width, float height);

	static const float margin;

protected:
	MFRect window;
	Button *pCloseButton;
	MFMaterial *pIcons;

	bool bVisible;
	bool bCloseButton;

	Game *pGame;

	void CloseWindow(int button, int buttonID);
};

#endif
