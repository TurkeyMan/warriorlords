#include "Warlords.h"
#include "MiniMap.h"

#include "MFFont.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFPrimitive.h"

MiniMap::MiniMap()
: Window(true)
{
	bDragging = false;
}

MiniMap::~MiniMap()
{
}

bool MiniMap::DrawContent()
{
	// draw map
	MFMaterial_SetMaterial(pMiniMap);

	float tx = (float)MFUtil_NextPowerOf2(width);
	float ty = (float)MFUtil_NextPowerOf2(height);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float texelCenterX = texelCenter / tx;
	float texelCenterY = texelCenter / ty;

	MFPrimitive_DrawQuad(window.x, window.y, tx, ty, MFVector::one, texelCenterX, texelCenterY, 1.f+texelCenterX, 1.f+texelCenterY);

	return true;
}

bool MiniMap::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Down:
			bDragging = true;
		case IE_Hover:
			if(bDragging)
			{
				int mapWidth, mapHeight;
				pMap->GetMapSize(&mapWidth, &mapHeight);

				float x = (info.down.x - window.x) / width * (float)mapWidth;
				float y = (info.down.y - window.y) / height * (float)mapHeight;

				pMap->CenterView(x, y);
			}
			break;
		case IE_Up:
			bDragging = false;
			break;
	}

	return Window::HandleInputEvent(ev, info);
}

void MiniMap::Show(Map *_pMap)
{
	pMap = _pMap;
	pMiniMap = pMap->GetMinimap(&width, &height);

	SetWindowSize((float)width, (float)height);
	Window::Show();
}

void MiniMap::Hide()
{
	Window::Hide();

	if(pMiniMap)
	{
		MFMaterial_Destroy(pMiniMap);
		pMiniMap = NULL;
	}
}
