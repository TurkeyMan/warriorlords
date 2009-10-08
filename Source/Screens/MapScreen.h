#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"

class MapScreen : public Screen
{
public:
	MapScreen(GameData *pGameData);
	virtual ~MapScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

	static void ShowMiniMap(int button, void *pUserData, int buttonID);

protected:
  GameData *pGameData;

	MFMaterial *pIcons;

	Button *pMiniMap;
};

#endif
