#if !defined(_SCREEN_H)
#define _SCREEN_H

#include "InputHandler.h"

class Screen : public InputHandler
{
public:
	Screen();
	virtual ~Screen();

	virtual void Select() = 0;
	virtual int Update() = 0;
	virtual void Draw() = 0;
	virtual void Deselect() = 0;

	static int UpdateScreen();
	static void DrawScreen();
	static void SetNext(Screen *pNext);
	static Screen *GetCurrent() { return pCurrent; }

	static Screen *pCurrent;
	static Screen *pNext;
};

#endif
