#include "Warlords.h"
#include "MiniMap.h"

#include "MFFont.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFPrimitive.h"

MiniMap::MiniMap()
: Window(true)
{
}

MiniMap::~MiniMap()
{
}

bool MiniMap::Draw()
{
	if(!Window::Draw())
		return false;

	// draw map
	MFMaterial_SetMaterial(pMiniMap);

	float tx = (float)MFUtil_NextPowerOf2(width);
	float ty = (float)MFUtil_NextPowerOf2(height);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float texelCenterX = texelCenter / tx;
	float texelCenterY = texelCenter / ty;

	MFPrimitive_DrawQuad(window.x + 16.f, window.y + 16.f, tx, ty, MFVector::one, texelCenterX, texelCenterY, 1.f+texelCenterX, 1.f+texelCenterY);

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
		case IE_Tap:
			break;
	}

	return Window::HandleInputEvent(ev, info);
}

void MiniMap::Show(Map *_pMap)
{
	Window::Show();

	pMap = _pMap;
	pMiniMap = pMap->GetMinimap(&width, &height);
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
