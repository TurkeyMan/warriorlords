#if !defined(_WARLORDS_H)
#define _WARLORDS_H

#include "Fuji.h"
#include "MFHeap.h"
#include "MFArray.h"

#include "Middleware/FastDelegate.h"

#include "Game.h"
#include "GameData.h"
#include "Session.h"

void DivideRect_Horiz(const MFRect &rect, float split, float margin, MFRect *pLeft, MFRect *pRight, bool bSplitPixels);
void DivideRect_Vert(const MFRect &rect, float split, float margin, MFRect *pTop, MFRect *pBottom, bool bSplitPixels);
void DivideRect_Quad(const MFRect &rect, float hSplit, float vSplit, float margin, MFRect *pTL, MFRect *pTR, MFRect *pBL, MFRect *pBR, bool bSplitPixels);
void AdjustRect_Margin(MFRect *pRect, float margin, bool bPixels = true);
void DrawTicker(float x, float y);

int LookupString(const char *pString, const char *pStrings[]);

#endif
