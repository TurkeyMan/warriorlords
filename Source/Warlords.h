#if !defined(_WARLORDS_H)
#define _WARLORDS_H

#pragma warning(disable:4530)

#include "Fuji/Fuji.h"
#include "Fuji/MFHeap.h"
#include "Fuji/MFArray.h"

#include "Fuji/Middleware/FastDelegate.h"
using namespace fastdelegate;

#include "Game.h"
#include "GameData.h"
#include "Session.h"

void DivideRect_Horiz(const MFRect &rect, float split, float margin, MFRect *pLeft, MFRect *pRight, bool bSplitPixels);
void DivideRect_Vert(const MFRect &rect, float split, float margin, MFRect *pTop, MFRect *pBottom, bool bSplitPixels);
void AdjustRect_Margin(MFRect *pRect, float margin, bool bPixels = true);

enum RenderLayer
{
	RL_Map = 0,
	RL_Scene = 1,
	RL_UI = 2
};

struct MFRenderLayer;
MFRenderLayer* GetRenderLayer(RenderLayer layer);
void BeginLayer(RenderLayer layer);

#endif
