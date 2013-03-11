#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Window.h"
#include "Game.h"
#include "Button.h"

#include "Unit.h"

#include "Fuji/MFFont.h"

#include "GroupConfig.h"

class MapScreen : public Screen
{
public:
	MapScreen(Game *pGame);
	virtual ~MapScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

protected:
	Game *pGame;
};

#endif
