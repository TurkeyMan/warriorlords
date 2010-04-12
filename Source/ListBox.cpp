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

	pNew->itemHeight = MFMax(pIcons ? iconSize : 0.f, pFont ? MFFont_GetFontHeight(pFont) : 0.f);
	pNew->textOffset = pIcons && pFont ? (pNew->itemHeight - MFFont_GetFontHeight(pFont)) * 0.5f: 0.f;

	pNew->numAllocated = 64;
	pNew->pItems = (ListItem*)MFHeap_AllocAndZero(sizeof(ListItem) * pNew->numAllocated);

	pNew->selection = -1;

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
			return true;
		case IE_Up:
			velocity = 0.f; // calculate velocity by keeping time or something?
			return true;
		case IE_Tap:
			selection = (int)((info.tap.y + yOffset) / itemHeight);
			if(selection < 0 || selection >= numItems)
				selection = -1;

			if(pSelectCallback)
				pSelectCallback(selection, pSelectUserData);
			return true;
		case IE_Drag:
			return true;
	}

	return false;
}

void ListBox::Draw()
{
	MFPrimitive_DrawUntexturedQuad(MakeVector(rect.x, rect.y, 1), MakeVector(rect.x + rect.width, rect.y + rect.height, 1), MakeVector(0, 0, 0.3f, 1.f));

	int x = (int)(rect.x + 10.f + (pIcons ? iconSize : 0.f));
	int y = (int)(rect.y - yOffset);
	int to = (int)textOffset;

	for(int a=0; a<numItems; ++a)
	{
		if(selection == a)
			MFPrimitive_DrawUntexturedQuad(MakeVector(rect.x, (float)y, 1), MakeVector(rect.x + rect.width, (float)y + itemHeight, 1), MFVector::blue);

		MFFont_BlitText(pFont, x, y + to, MFVector::yellow, pItems[a].text);

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

int ListBox::AddItem(const char *pText, int icon)
{
	if(numItems == numAllocated)
	{
		numAllocated *= 2;
		MFHeap_Realloc(pItems, sizeof(ListItem) * numAllocated);
	}

	MFString_Copy(pItems[numItems].text, pText);
	pItems[numItems].icon = icon;
	return numItems++;
}

const char *ListBox::GetItemText(int item)
{
	if(item >= 0 && item < numItems)
		return pItems[item].text;
	return NULL;
}
