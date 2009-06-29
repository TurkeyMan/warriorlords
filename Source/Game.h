#if !defined(_GAME_H)
#define _GAME_H

#include "Screen.h"
#include "Map.h"
#include "Button.h"

class Game : public Screen
{
public:
	Game(const char *pMap);
	virtual ~Game();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

	static void ShowMiniMap(int button, void *pUserData, int buttonID);

protected:
	Map *pMap;

	MFMaterial *pIcons;

	Button *pMiniMap;
};

#endif
