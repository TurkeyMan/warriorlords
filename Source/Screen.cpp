#include "Warlords.h"
#include "Screen.h"

Screen *Screen::pCurrent = NULL;
Screen *Screen::pNext = NULL;

Screen::Screen()
{
}

Screen::~Screen()
{
}

int Screen::UpdateScreen()
{
	if(pNext)
	{
		if(pCurrent)
			pCurrent->Deselect();
		pNext->Select();
		pCurrent = pNext;
		pNext = NULL;
	}

	if(pCurrent)
		return pCurrent->Update();
	return 0;
}

void Screen::DrawScreen()
{
	if(pCurrent)
		return pCurrent->Draw();
}

void Screen::SetNext(Screen *_pNext)
{
	pNext = _pNext;
}
