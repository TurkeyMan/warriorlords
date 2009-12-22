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
	pNew->uvs = *pUVs;
	pNew->pCallback = pCallback;
	pNew->pUserData = pUserData;
	pNew->isPressed = -1;
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
	delete this;
}

bool Button::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			if(button == -1 || info.buttonID == button)
			{
				if(bTriggerOnDown)
				{
					if(pCallback)
						pCallback(info.buttonID, pUserData, buttonID);
				}
				else
				{
					isPressed = info.contact;
					pInputManager->SetExclusiveContactReceiver(info.contact, this);
				}
			}
			return true;
		case IE_Up:
			if(button == -1 || info.buttonID == button && info.contact == isPressed)
			{
				isPressed = -1;

				if(MFTypes_PointInRect(info.up.x, info.up.y, &rect))
				{
					if(pCallback)
						pCallback(info.buttonID, pUserData, buttonID);
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

	if(isPressed >= 0)
	{
		float x = MFInput_Read(Mouse_XPos, IDD_Mouse);
		float y = MFInput_Read(Mouse_YPos, IDD_Mouse);

		if(MFTypes_PointInRect(x, y, &rect))
			bDark = true;
	}

	// outline?
	if(bOutline)
		MFPrimitive_DrawUntexturedQuad(rect.x - 2, rect.y - 2, rect.width + 4, rect.height + 4, outlineColour);

	MFMaterial_SetMaterial(pMaterial);
	MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height, bDark ? MakeVector(0.6f, 0.6f, 0.6f, 1.f) : MFVector::one, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
	if(pOverlay)
	{
		MFMaterial_SetMaterial(pOverlay);
		MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height, bDark ? MakeVector(0.6f, 0.6f, 0.6f, 1.f) * overlayColour : overlayColour, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
	}
}

void Button::SetPos(MFRect *pPos)
{
	UpdateRect(pPos);
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
