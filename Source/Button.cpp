#include "Warlords.h"
#include "Button.h"

#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"

Button *Button::Create(MFMaterial *pImage, MFRect *pPosition, MFRect *pUVs, TriggerCallback *pCallback, void *pUserData, int buttonID, bool bTriggerOnDown)
{
	Button *pNew = (Button*)MFHeap_Alloc(sizeof(Button));
	pNew = new(pNew) Button;

	pNew->pMaterial = pImage;
	pNew->pos = *pPosition;
	pNew->uvs = *pUVs;
	pNew->pCallback = pCallback;
	pNew->pUserData = pUserData;
	pNew->bIsPressed = false;
	pNew->bTriggerOnDown = bTriggerOnDown;
	pNew->bOutline = false;
	pNew->outlineColour = MFVector::white;
	pNew->buttonID = buttonID;

	return pNew;
}

void Button::Destroy()
{
	MFHeap_Free(this);
}

int Button::UpdateInput()
{
	float x = MFInput_Read(Mouse_XPos, IDD_Mouse);
	float y = MFInput_Read(Mouse_YPos, IDD_Mouse);

	if(MFInput_WasPressed(Mouse_LeftButton, IDD_Mouse))
	{
		if(MFTypes_PointInRect(x, y, &pos))
		{
			if(bTriggerOnDown)
			{
				if(pCallback)
					pCallback(0, pUserData, buttonID);
			}
			else
				bIsPressed = true;

			SetExclusive();
			return 1;
		}
	}

	if(MFInput_WasReleased(Mouse_LeftButton, IDD_Mouse))
	{
		ReleaseExclusive();
		bIsPressed = false;

		if(bIsPressed && MFTypes_PointInRect(x, y, &pos))
		{
			if(pCallback)
				pCallback(0, pUserData, buttonID);
			return 1;
		}
	}

	return 0;
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
}

void Button::SetPos(MFRect *pPos)
{
	pos = *pPos;
}

void Button::SetUVs(MFRect *pUVs)
{
	uvs = *pUVs;
}

void Button::SetOutline(bool bEnable, const MFVector &colour)
{
	bOutline = bEnable;
	outlineColour = colour;
}
