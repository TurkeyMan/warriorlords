#if !defined(_INVENTORY_H)
#define _INVENTORY_H

#include "Window.h"

class Inventory : public Window
{
public:
	Inventory();
	virtual ~Inventory();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Unit *pUnit);
	virtual void Hide();

protected:
	Unit *pUnit;

	MFMaterial *pItems;

	Button *pInventory[8];
	int numItems;

	int selected;

	static void SelectItem(int button, void *pUserData, int buttonID);
};

#endif
