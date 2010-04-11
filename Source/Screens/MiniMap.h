#if !defined(_MINIMAP_H)
#define _MINIMAP_H

#include "Window.h"

class MiniMap : public Window
{
public:
	MiniMap();
	virtual ~MiniMap();

	virtual bool Draw();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	virtual void Show(Map *pMap);
	virtual void Hide();

protected:
	Map *pMap;
	MFMaterial *pMiniMap;

	int width, height;
};

#endif
