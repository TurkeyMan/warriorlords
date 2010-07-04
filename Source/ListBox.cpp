#include "Warlords.h"
#include "ListBox.h"

#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"
#include "MFFont.h"

ListBox *ListBox::Create(const MFRect *pPosition, MFFont *pFont, MFMaterial *pIcons, float iconSize)
{
	ListBox *pNew = (ListBox*)MFHeap_AllocAndZero(sizeof(ListBox));
	pNew = new(pNew) ListBox(*pPosition);

	pNew->pFont = pFont;
	pNew->pIcons = pIcons;
	pNew->iconSize = iconSize;

	pNew->backColour = MakeVector(0, 0, 0.3f, 1.f);
	pNew->bHighlightCursor = false;

	pNew->itemHeight = MFMax(pIcons ? iconSize : 0.f, pFont ? MFFont_GetFontHeight(pFont) : 0.f);
	pNew->textOffset = pIcons && pFont ? (pNew->itemHeight - MFFont_GetFontHeight(pFont)) * 0.5f: 0.f;

	pNew->numAllocated = 64;
	pNew->pItems = (ListItem*)MFHeap_AllocAndZero(sizeof(ListItem) * pNew->numAllocated);

	pNew->selection = -1;
	pNew->bSelecting = false;

	return pNew;
}

void ListBox::Destroy()
{
	MFHeap_Free(pItems);

	MFHeap_Free(this);
}

bool ListBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			downPos = info.down.y;
			velocity = 0.f;
			bSelecting = true;
			return true;
		case IE_Up:
			velocity = 0.f; // calculate velocity by keeping time or something?
			return true;
		case IE_Tap:
			if(bSelecting)
			{
				bSelecting = false;

				// change selection only if the click was inside the listbox
				MFRect client = { 0, 0, rect.width, rect.height };
				if(MFTypes_PointInRect(info.up.x, info.up.y, &client))
					selection = (int)((info.tap.y + yOffset) / itemHeight);

				if(selection < 0 || selection >= numItems)
					selection = -1;

				if(pSelectCallback)
					pSelectCallback(selection, pSelectUserData);
			}
			return true;
		case IE_Drag:
			return true;
	}

	return false;
}

void ListBox::Draw()
{
	MFPrimitive_DrawUntexturedQuad(MakeVector(rect.x, rect.y, 1), MakeVector(rect.x + rect.width, rect.y + rect.height, 1), backColour);

	int x = (int)(rect.x + 10.f + (pIcons ? iconSize : 0.f));
	int y = (int)(rect.y - yOffset);
	int to = (int)textOffset;

	int highlight = selection;

	if(bHighlightCursor)
	{
		// find the mouseover item
		float mouseY = MFInput_Read(Mouse_YPos, IDD_Mouse);
		float mouseOver = (mouseY - rect.y + yOffset) / itemHeight;

		if(mouseOver >= 0.f && mouseOver < (float)numItems)
			highlight = (int)mouseOver;
		else
			highlight = -1;
	}

	for(int a=0; a<numItems; ++a)
	{
		if(highlight == a)
			MFPrimitive_DrawUntexturedQuad(MakeVector(rect.x, (float)y, 1), MakeVector(rect.x + rect.width, (float)y + itemHeight, 1), MFVector::blue);

		MFFont_BlitText(pFont, x, y + to, pItems[a].colour, pItems[a].text);

		if(pIcons && pItems[a].icon > -1)
		{
//			MFMaterial_SetMaterial(pIcons);
			//...
		}

		y += (int)itemHeight;
	}
}

void ListBox::SetPos(const MFRect *pPos)
{
	UpdateRect(pPos);
}

int ListBox::AddItem(const char *pText, int icon, void *pUserData, const MFVector &colour)
{
	if(numItems == numAllocated)
	{
		numAllocated *= 2;
		MFHeap_Realloc(pItems, sizeof(ListItem) * numAllocated);
	}

	MFString_Copy(pItems[numItems].text, pText);
	pItems[numItems].icon = icon;
	pItems[numItems].colour = colour;
	pItems[numItems].pUserData = pUserData;
	return numItems++;
}

const char *ListBox::GetItemText(int item)
{
	if(item >= 0 && item < numItems)
		return pItems[item].text;
	return NULL;
}

const void *ListBox::GetItemData(int item)
{
	if(item >= 0 && item < numItems)
		return pItems[item].pUserData;
	return NULL;
}

const MFVector &ListBox::GetItemColour(int item)
{
	if(item >= 0 && item < numItems)
		return pItems[item].colour;
	return MFVector::white;
}

int ListBox::GetItemIcon(int item)
{
	if(item >= 0 && item < numItems)
		return pItems[item].icon;
	return -1;
}

void ListBox::Clear()
{
	numItems = 0;
	selection = -1;
}

float ListBox::GetItemHeight()
{
	return itemHeight;
}
