#if !defined(_MAPSCREEN_H)
#define _MAPSCREEN_H

#include "Screen.h"
#include "Game.h"
#include "Button.h"

#include "Unit.h"

class MapScreen : public Screen
{
public:
	MapScreen(Game *pGame);
	virtual ~MapScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

	static void ShowMiniMap(int button, void *pUserData, int buttonID);

protected:
	class Selection
	{
	public:
		void DrawSelection();

		void SelectUnit(Unit *_pUnit) { pUnit = _pUnit; pCastle = NULL; }
		void SelectCastle(Castle *_pCastle) { pCastle = _pCastle; pUnit = NULL; }
		void Deselect() { pUnit = NULL; pCastle = NULL; }

	protected:
		Unit *pUnit;
		Castle *pCastle;
	};

	Game *pGame;

	MFMaterial *pIcons;

	Button *pMiniMap;

	Selection selection;
};

#endif
