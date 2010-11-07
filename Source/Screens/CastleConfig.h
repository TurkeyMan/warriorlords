#if !defined(_CASTLECONFIG_H)
#define _CASTLECONFIG_H

#include "Window.h"

class CastleConfig : public Window
{
public:
	CastleConfig();
	virtual ~CastleConfig();

	virtual bool DrawContent();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Castle *pCastle);
	virtual void Hide();

protected:
	Castle *pCastle;

	MFRect title, units, lower, right;

	Button *pBuildUnits[4];
	int numBuildUnits;

	void SelectUnit(int button, int buttonID);
};

#endif
