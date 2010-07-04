#include "Warlords.h"
#include "SelectBox.h"

#include "MFMaterial.h"
#include "MFPrimitive.h"
#include "MFInput.h"
#include "MFFont.h"

SelectBox *SelectBox::Create(const MFRect *pPosition, MFFont *pFont, MFMaterial *pIcons, float iconSize)
{
	float itemHeight = MFMax(pIcons ? iconSize : 0.f, pFont ? MFFont_GetFontHeight(pFont) : 0.f);

	MFRect r = *pPosition;
	r.height = itemHeight;

	SelectBox *pNew = (SelectBox*)MFHeap_Alloc(sizeof(SelectBox));
	pNew = new(pNew) SelectBox(r);

	// init the control
	pNew->pFont = pFont;
	pNew->pIcons = pIcons;
	pNew->iconSize = iconSize;
	pNew->pCallback = NULL;
	pNew->pUserData = NULL;
	pNew->bShowList = false;
	pNew->bEnabled = true;

	pNew->itemHeight = itemHeight;
	pNew->textOffset = pIcons && pFont ? (pNew->itemHeight - MFFont_GetFontHeight(pFont)) * 0.5f: 0.f;

	// create the listbox
	r.y += itemHeight;
	pNew->pList = ListBox::Create(&r, pFont, pIcons, iconSize);
	pNew->pList->SetSelectCallback(ListCallback, pNew);
	pNew->pList->HighlightCursor(true);

	return pNew;
}

void SelectBox::Destroy()
{
	MFHeap_Free(this);
}

bool SelectBox::HandleInputEvent(InputEvent ev, InputInfo &info)
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
			pInputManager->PushReceiver(pList);
			pInputManager->SetExclusiveReceiver(pList);
			bShowList = true;
			return true;
		case IE_Up:
		case IE_Tap:
		case IE_Drag:
			return true;
	}

	return false;
}

void SelectBox::Draw()
{
	MFPrimitive_DrawUntexturedQuad(MakeVector(rect.x, rect.y, 1), MakeVector(rect.x + rect.width, rect.y + rect.height, 1), MakeVector(0, 0, 0.3f, 1.f));

	int selection = pList->GetSelection();
	if(selection >= 0)
	{
		const char *pText = pList->GetItemText(selection);
		const MFVector &itemColour = pList->GetItemColour(selection);
		int icon = pList->GetItemIcon(selection);

		int x = (int)(rect.x + 10.f + (pIcons ? iconSize : 0.f));
		int y = (int)rect.y;
		int to = (int)textOffset;

		MFFont_BlitText(pFont, x, y + to, itemColour, pText);

		if(pIcons && icon > -1)
		{
		//			MFMaterial_SetMaterial(pIcons);
			//...
		}
	}

	if(bShowList)
		pList->Draw();
}

void SelectBox::SetPos(const MFRect *pPos)
{
	UpdateRect(pPos);
}

void SelectBox::SetSelectCallback(SelectCallback *_pCallback, void *_pUserData, int _id)
{
	pCallback = _pCallback;
	pUserData = _pUserData;
	id = _id;
}

void SelectBox::Clear()
{
	pList->Clear();
}

int SelectBox::AddItem(const char *pText, int icon, void *pUserData, const MFVector &colour)
{
	int item = pList->AddItem(pText, icon, pUserData, colour);

	if(pList->GetSelection() == -1)
		pList->SetSelection(0);

	MFRect r = rect;
	int numItems = pList->GetNumItems();
	float itemHeight = pList->GetItemHeight();
	r.y += itemHeight + 2;
	r.height = MFClamp(2.f, (float)numItems * itemHeight, 280.f);
	pList->SetPos(&r);

	return item;
}

const char *SelectBox::GetItemText(int item)
{
	return pList->GetItemText(item);
}

const void *SelectBox::GetItemData(int item)
{
	return pList->GetItemData(item);
}

void SelectBox::SetSelection(int item)
{
	pList->SetSelection(item);
}

int SelectBox::GetSelection()
{
	return pList->GetSelection();
}

void SelectBox::ListCallback(int item, void *pThis)
{
	SelectBox *pSB = (SelectBox*)pThis;

	pSB->bShowList = false;
	pInputManager->PopReceiver(pSB->pList);

	if(pSB->pCallback)
		pSB->pCallback(item, pSB->pUserData, pSB->id);
}
