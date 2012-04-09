#include "Warlords.h"
#include "Inventory.h"

#include "MFFont.h"
#include "MFMaterial.h"
#include "MFRenderer.h"

Inventory::Inventory()
: Window(true)
{
	pItems = MFMaterial_Create("Items");

	float x = window.x + window.width*0.5f - (64.f*4+8*3)*0.5f;
	for(int a=0; a<8; ++a)
	{
		MFRect pos = { x + (a & 0x3)*72.f, window.y + 40.f + (a >> 2)*72.f, 64.f, 64.f };
		pInventory[a] = Button::Create(pItems, &pos, &pos, MFVector::one, a);
		pInventory[a]->SetClickCallback(MakeDelegate(this, &Inventory::SelectItem));
	}
}

Inventory::~Inventory()
{
	MFMaterial_Destroy(pItems);
}

bool Inventory::DrawContent()
{
	Game::GetCurrent()->DrawLine(window.x + 16, window.y + 192, window.x + window.width - 16, window.y + 192);

	MFFont *pFont = Game::GetCurrent()->GetTextFont();
	float height = MFFont_GetFontHeight(pFont);
	float width = MFFont_GetStringWidth(pFont, "Inventory", height);
	MFFont_BlitText(pFont, (int)(window.x + window.width*0.5f - width*0.5f), (int)window.y + 10, MFVector::yellow, "Inventory");

	for(int a=0; a<numItems; ++a)
		pInventory[a]->Draw();

	if(selected != -1)
	{
		Item *pItem = pUnit->GetItem(selected);
		width = MFFont_GetStringWidth(pFont, pItem->pName, height);
		MFFont_BlitText(pFont, (int)(window.x + window.width*0.5f - width*0.5f), (int)window.y + 200, MFVector::white, pItem->pName);
		MFFont_BlitText(pFont, (int)(window.x + window.width*0.5f - width*0.5f), (int)window.y + 210 + (int)height, MFVector::white, pItem->pDescription);
	}

	return true;
}

bool Inventory::HandleInputEvent(InputEvent ev, InputInfo &info)
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

void Inventory::Show(Unit *_pUnit)
{
	Window::Show();

	pUnit = _pUnit;

	selected = -1;

	UnitDefinitions *pDefs = pUnit->GetDefs();
	float texelCenter = MFRenderer_GetTexelCenterOffset();

	numItems = pUnit->GetNumItems();
	for(int a=0; a<numItems; ++a)
	{
		Item *pItem = pUnit->GetItem(a);

		MFRect uvs;
		pDefs->GetItemUVs(pUnit->GetItemID(a), &uvs, texelCenter);
		pInventory[a]->SetImage(pDefs->GetItemMaterial(), &uvs);
		pInputManager->PushReceiver(pInventory[a]);
	}
}

void Inventory::Hide()
{
	Window::Hide();
}

void Inventory::SelectItem(int button, int buttonID)
{
	selected = buttonID;
}
