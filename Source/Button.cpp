#include "Warlords.h"
#include "Button.h"

#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"

Button *Button::Create(MFMaterial *pImage, MFRect *pPosition, MFRect *pUVs, TriggerCallback *pCallback, void *pUserData, int buttonID, bool bTriggerOnDown)
{
	Button *pNew = (Button*)MFHeap_Alloc(sizeof(Button));
	pNew = new(pNew) Button(*pPosition);

	pNew->pMaterial = pImage;
	pNew->pos = *pPosition;
	pNew->uvs = *pUVs;
	pNew->pCallback = pCallback;
	pNew->pUserData = pUserData;
	pNew->bIsPressed = false;
	pNew->bTriggerOnDown = bTriggerOnDown;
	pNew->bOutline = false;
	pNew->outlineColour = MFVector::white;
	pNew->button = 0;
	pNew->buttonID = buttonID;
	pNew->pOverlay = NULL;

	return pNew;
}

void Button::Destroy()
{
	MFHeap_Free(this);
}

bool Button::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			if(info.buttonID == button)
			{
				if(bTriggerOnDown)
				{
					if(pCallback)
						pCallback(0, pUserData, buttonID);
				}
				else
				{
					bIsPressed = true;
				}

				pInputManager->SetExclusiveContactReceiver(info.contact, this);
			}
			return true;
		case IE_Up:
			if(info.buttonID == button)
			{
				bIsPressed = false;
				if(!bTriggerOnDown)
				{
					if(MFTypes_PointInRect(info.up.x, info.up.y, &pos))
					{
						if(pCallback)
							pCallback(0, pUserData, buttonID);
					}
				}
			}
			return true;
		case IE_Tap:
		case IE_Drag:
			return true;
	}

	return false;
}

void Button::Draw()
{
	bool bDark = false;

	if(bIsPressed)
	{
		float x = MFInput_Read(Mouse_XPos, IDD_Mouse);
		float y = MFInput_Read(Mouse_YPos, IDD_Mouse);

		if(MFTypes_PointInRect(x, y, &pos))
			bDark = true;
	}

	// outline?
	if(bOutline)
		MFPrimitive_DrawUntexturedQuad(pos.x - 2, pos.y - 2, pos.width + 4, pos.height + 4, outlineColour);

	MFMaterial_SetMaterial(pMaterial);
	MFPrimitive_DrawQuad(pos.x, pos.y, pos.width, pos.height, bDark ? MakeVector(0.6f, 0.6f, 0.6f, 1.f) : MFVector::one, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
	if(pOverlay)
	{
		MFMaterial_SetMaterial(pOverlay);
		MFPrimitive_DrawQuad(pos.x, pos.y, pos.width, pos.height, bDark ? MakeVector(0.6f, 0.6f, 0.6f, 1.f) * overlayColour : overlayColour, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
	}
}

void Button::SetPos(MFRect *pPos)
{
	pos = *pPos;
}

void Button::SetImage(MFMaterial *pImage, MFRect *pUVs)
{
	pMaterial = pImage;
	uvs = *pUVs;
}

void Button::SetOutline(bool bEnable, const MFVector &colour)
{
	bOutline = bEnable;
	outlineColour = colour;
}

void Button::SetOverlay(MFMaterial *pImage, const MFVector &colour)
{
	pOverlay = pImage;
	overlayColour = colour;
}
