#if !defined(_INVENTORY_H)
#define _INVENTORY_H

#include "Window.h"

class Inventory : public Window
{
public:
	Inventory(Game *pGame);
	virtual ~Inventory();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Game *pGame, Unit *pUnit);
	virtual void Hide();

protected:
	Unit *pUnit;

	MFMaterial *pItems;

	Button *pInventory[8];
	int numItems;

	int selected;

	void SelectItem(int button, int buttonID);
};

#endif
