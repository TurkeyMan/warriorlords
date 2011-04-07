#include "Warlords.h"
#include "ListProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

#include "MFRenderer.h"
#include "MFFont.h"
#include "MFPrimitive.h"

int GetJustificationFromString(const char *pString);

void uiListProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("List", Create, "Entity");

	uiActionManager::RegisterProperty("items", GetItems, SetItems, pType);
	uiActionManager::RegisterProperty("isselected", IsSelected, NULL, pType);
	uiActionManager::RegisterProperty("selection", GetSelected, SetSelected, pType);
	uiActionManager::RegisterProperty("text", GetCurrent, NULL, pType);
	uiActionManager::RegisterProperty("font", NULL, SetFont, pType);

	uiActionManager::RegisterInstantAction("clear", ClearItems, pType);
	uiActionManager::RegisterInstantAction("add", AddItem, pType);
	uiActionManager::RegisterInstantAction("remove", RemoveItem, pType);
}

uiListProp::uiListProp()
{
	pFont = MFFont_GetDebugFont();
	size.y = MFFont_GetFontHeight(pFont) * 6.f + 4.f;
	size.x = size.y * 0.75f;

	selection = -1;

	yOffset = 0.f;
	velocity = 0.f;

	downPos = 0.f;
	bSelecting = false;

	bFollowCursor = false;

	selectCallback.clear();
	dblClickCallback.clear();
}

uiListProp::~uiListProp()
{
}

bool uiListProp::HandleInputEvent(InputEvent ev, const InputInfo &info)
{
	switch(ev)
	{
		case IE_Hover:
			hoverX = info.hover.x - 2.f;
			hoverY = info.hover.y - 2.f;
			break;

		case IE_Down:
			if(bFollowCursor)
			{
				MFRect rect = { 0, 0, size.x - 1, size.y - 1 };
				if(!MFTypes_PointInRect(info.up.x, info.up.y, &rect))
				{
					GetEntityManager()->SetExclusiveReceiver(NULL);
					SetVisible(false);

					// disconnect from UI tree
					if(selection)
						selectCallback(selection);
					return true;
				}
			}

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
				float itemHeight = MFFont_GetFontHeight(pFont);

				MFRect client = { 2, 2, size.x - 5, size.y - 5 };
				if(MFTypes_PointInRect(info.up.x, info.up.y, &client))
				{
					if(bFollowCursor)
					{
						int newSel = (int)((info.tap.y - yOffset) / itemHeight);
						if(newSel >= 0 && newSel < items.size())
							selection = newSel;

						// disconnect from UI tree
						if(selectCallback)
							selectCallback(selection);
					}
					else
						SetSelection((int)((info.tap.y - yOffset) / itemHeight));
				}
			}
			return true;

		case IE_Drag:
			float itemHeight = MFFont_GetFontHeight(pFont);
			float max = itemHeight * (float)items.size() - size.y;
			yOffset = MFClamp(-max - 4.f, yOffset + info.drag.y - info.drag.startY, 0.f);
			return true;
	}

	return uiEntity::HandleInputEvent(ev, info);
}

void uiListProp::Update()
{
	uiEntity::Update();
}

void uiListProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float textHeight = MFFont_GetFontHeight(pFont);

	int sel = selection;
	if(bFollowCursor)
	{
		// calculate selection
		sel = (int)((hoverY - yOffset) / textHeight);
		if(sel >= items.size())
			sel = -1;
	}

	bool bDrawSelection = sel != -1;

	MFRect rect = { 0, 0, size.x - 1, size.y - 1 };
	if(!MFTypes_PointInRect(hoverX, hoverY, &rect))
		bDrawSelection = false;

	// draw the frame and selection
	MFPrimitive(PT_TriStrip | PT_Prelit | PT_Untextured);
	MFSetMatrix(state.mat);

	MFBegin(bDrawSelection ? 16 : 10);

	MFSetColour(MFVector::white);
	MFSetPosition(0.f, 0.f, 0);
	MFSetPosition(size.x, 0.f, 0);
	MFSetPosition(0.f, size.y, 0);
	MFSetPosition(size.x, size.y, 0);

	// degen
	MFSetPosition(0.f+size.x, 0.f+size.y, 0);
	MFSetPosition(2.f, 2.f, 0);

	MFSetColour(MFVector::black);
	MFSetPosition(2.f, 2.f, 0);
	MFSetPosition(size.x-2.f, 2.f, 0);
	MFSetPosition(2.f, size.y-2.f, 0);
	MFSetPosition(size.x-2.f, size.y-2.f, 0);

	if(bDrawSelection)
	{
		// degen
		MFSetPosition(size.x-2.f, size.y-2.f, 0);
		MFSetPosition(2.f, 2.f + sel*textHeight + yOffset, 0);

		MFSetColour(0, 0, 0.6f, 1);
		MFSetPosition(2.f, 2.f + sel*textHeight + yOffset, 0);
		MFSetPosition(size.x-2.f, 2.f + sel*textHeight + yOffset, 0);
		MFSetPosition(2.f, 2.f + (sel+1)*textHeight + yOffset, 0);
		MFSetPosition(size.x-2.f, 2.f + (sel+1)*textHeight + yOffset, 0);
	}

	MFEnd();

	// draw items
	for(int a=0; a<items.size(); ++a)
		MFFont_DrawText(pFont, 8.f - texelCenter, 2.f + textHeight*(float)a + yOffset - texelCenter, textHeight, state.colour, items[a].text.CStr(), -1, state.mat);
}

void uiListProp::SetSelection(int item)
{
	int newSelection = (item < 0 || item >= items.size()) ? -1 : item;

	if(newSelection == selection)
		return;

	selection = newSelection;
	if(selection > -1 && selectCallback)
		selectCallback(selection);
	SignalEvent("onselectionchange");

	if(selection < 0)
		SignalEvent("onselectionclear");
}

void uiListProp::ClearItems()
{
	items.clear();
	selection = -1;
}

void uiListProp::AddItem(const char *pItem)
{
	ListItem &item = items.push();
	item.text = pItem;
}

void uiListProp::FollowCursor(bool _bFollowCursor)
{
	bFollowCursor = _bFollowCursor;
}

MFString uiListProp::GetItems(uiEntity *pEntity)
{
	uiListProp *pList = (uiListProp*)pEntity;

	MFString t("{");

	for(int a=0; a<pList->items.size(); ++a)
		t += MFString::Format(a == 0 ? "\"%s\"" : ",\"%s\"", pList->items[a].text.CStr());

	return t + "}";
}

void uiListProp::SetItems(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;

	uiRuntimeArgs *pItems = pArguments->GetArray(0);
	int count = pItems->GetNumArgs();

	pList->items.clear();
	pList->selection = -1;

	for(int a=0; a<count; ++a)
	{
		ListItem &item = pList->items.push();
		item.text = pItems->GetString(a);
	}
}

void uiListProp::ClearItems(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;
	pList->items.clear();
	pList->selection = -1;
}

void uiListProp::AddItem(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;

	ListItem &item = pList->items.push();
	item.text = pArguments->GetString(0);
}

void uiListProp::RemoveItem(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;

	MFDebug_Assert(false, "!");
	//...
}

MFString uiListProp::IsSelected(uiEntity *pEntity)
{
	uiListProp *pList = (uiListProp*)pEntity;
	return MFString::Static(pList->selection > -1 ? "true" : "false");
}

MFString uiListProp::GetSelected(uiEntity *pEntity)
{
	uiListProp *pList = (uiListProp*)pEntity;
	MFString t;
	return t.FromInt(pList->selection);
}

void uiListProp::SetSelected(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;
	pList->SetSelection(pArguments->GetInt(0));
}

MFString uiListProp::GetCurrent(uiEntity *pEntity)
{
	uiListProp *pList = (uiListProp*)pEntity;
	return pList->selection >= 0 ? pList->items[pList->selection].text : NULL;
}

void uiListProp::SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiListProp *pList = (uiListProp*)pEntity;
	pList->pFont = GameData::Get()->GetResourceCache()->FindFont(pArguments->GetString(0).CStr());
	pList->size.y = MFFont_GetFontHeight(pList->pFont) * 6.f + 4.f;
	pList->size.x = pList->size.y * 0.75f;
}
