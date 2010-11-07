#if !defined(_UNITCONFIG_H)
#define _UNITCONFIG_H

#include "Window.h"
#include "Inventory.h"

class UnitConfig : public Window
{
public:
	UnitConfig();
	virtual ~UnitConfig();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Unit *pUnit);
	virtual void Hide();

protected:
	Inventory inventory;

	Unit *pUnit;

	MFRect top, bottom;

	Button *pInventory;
	CheckBox *pStrategySelect[6];

	void ShowInventory(int button, int buttonID);
	void SelectStrat(int value, int buttonID);
};

#endif
