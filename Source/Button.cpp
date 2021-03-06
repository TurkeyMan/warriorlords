#include "Warlords.h"
#include "Button.h"

#include "Fuji/MFMaterial.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFInput.h"
#include "Fuji/MFFont.h"

Button *Button::Create(const MFMaterial *pImage, const MFRect *pPosition, const MFRect *pUVs, const MFVector &colour, int buttonID, bool bTriggerOnDown)
{
	Button *pNew = (Button*)MFHeap_Alloc(sizeof(Button));
	pNew = new(pNew) Button(*pPosition);

	pNew->pMaterial = pImage;
	pNew->uvs = *pUVs;
	pNew->clickCallback.clear();
	pNew->isPressed = -1;
	pNew->bTriggerOnDown = bTriggerOnDown;
	pNew->bOutline = false;
	pNew->colour = colour;
	pNew->outlineColour = MFVector::white;
	pNew->button = 0;
	pNew->buttonID = buttonID;
	pNew->pOverlay = NULL;
	pNew->bEnabled = true;

	return pNew;
}

void Button::Destroy()
{
	this->~Button();
	MFHeap_Free(this);
}

bool Button::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(!bEnabled)
		return false;

	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	if(info.buttonID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			if(button == -1 || info.buttonID == button)
			{
				if(bTriggerOnDown)
				{
					if(clickCallback)
						clickCallback(info.buttonID, buttonID);
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

				MFRect client = { 0, 0, rect.width - 1, rect.height - 1 };
				if(MFTypes_PointInRect(info.up.x, info.up.y, &client))
				{
					if(clickCallback)
						clickCallback(info.buttonID, buttonID);
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
	bool bDark = !bEnabled;

	if(bEnabled && isPressed >= 0)
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
	MFPrimitive_DrawQuad(rect.x, rect.y, rect.width, rect.height, bDark ? MakeVector(0.6f, 0.6f, 0.6f, 1.f)*colour : colour, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
}

void Button::SetPos(const MFRect *pPos)
{
	UpdateRect(pPos);
}

void Button::SetImage(const MFMaterial *pImage, const MFRect *pUVs, const MFVector &_colour)
{
	pMaterial = pImage;
	uvs = *pUVs;
	colour = _colour;
}

void Button::SetOutline(bool bEnable, const MFVector &colour)
{
	bOutline = bEnable;
	outlineColour = colour;
}

CheckBox *CheckBox::Create(const MFRect *pPosition, const char *pText, const MFVector &colour, int value, int ButtonID)
{
	CheckBox *pNew = (CheckBox*)MFHeap_Alloc(sizeof(CheckBox));
	pNew = new(pNew) CheckBox(*pPosition);

	pNew->pIcons = MFMaterial_Create("Icons");

	MFRect uvs = { 0.5f - 0.25f*(float)value, 0.f, 0.25f, 0.25f };
	MFRect buttonRect;
	buttonRect.x = pPosition->x;
	buttonRect.y = pPosition->y;
	buttonRect.width = buttonRect.height = pPosition->height;
	pNew->pButton = Button::Create(pNew->pIcons, &buttonRect, &uvs, MFVector::one, 0, false);
	pNew->pButton->SetClickCallback(MakeDelegate(pNew, &CheckBox::ButtonCallback));

	MFString_Copy(pNew->text, pText);

	pNew->pFont = MFFont_Create("FranklinGothic");
	pNew->colour = colour;
	pNew->isPressed = false;
	pNew->value = value;
	pNew->buttonID = ButtonID;
	pNew->clickCallback.clear();
	pNew->changeCallback.clear();

	return pNew;
}

void CheckBox::Destroy()
{
	pButton->Destroy();

	this->~CheckBox();
	MFHeap_Free(this);
}

bool CheckBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	return pButton->HandleInputEvent(ev, info);
}

void CheckBox::Draw()
{
	pButton->Draw();

	if(text[0])
	{
		float height = MFFont_GetFontHeight(pFont);
		MFFont_BlitText(pFont, (int)(rect.x + rect.height + 5), (int)(rect.y + (rect.height - height)*0.5f), colour, text);
	}
}

void CheckBox::SetFont(MFFont *pFont)
{
	pFont = pFont;
}

int CheckBox::SetValue(int newValue)
{
	int oldValue = value;
	value = newValue;

	MFRect uvs = { 0.5f - 0.25f*(float)value, 0.f, 0.25f, 0.25f };
	pButton->SetImage(pIcons, &uvs, MFVector::white);

	return oldValue;
}

void CheckBox::ButtonCallback(int button, int buttonID)
{
	value = 1 - value;

	MFRect uvs = { 0.5f - 0.25f*(float)value, 0.f, 0.25f, 0.25f };
	pButton->SetImage(pIcons, &uvs, MFVector::white);

	if(clickCallback)
		clickCallback(button, buttonID);

	if(changeCallback)
		changeCallback(value, buttonID);
}
