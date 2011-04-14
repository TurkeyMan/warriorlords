#include "Warlords.h"
#include "SelectBoxProp.h"
#include "Action.h"

#include "MFRenderer.h"
#include "MFFont.h"
#include "MFPrimitive.h"

void uiSelectBoxProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("SelectBox", Create, "Entity");

	uiActionManager::RegisterProperty("items", GetItems, SetItems, pType);
	uiActionManager::RegisterProperty("selection", GetSelected, SetSelected, pType);
	uiActionManager::RegisterProperty("text", GetCurrent, NULL, pType);
	uiActionManager::RegisterProperty("font", NULL, SetFont, pType);

	uiActionManager::RegisterInstantAction("clear", ClearItems, pType);
	uiActionManager::RegisterInstantAction("add", AddItem, pType);
	uiActionManager::RegisterInstantAction("remove", RemoveItem, pType);
}

uiSelectBoxProp::uiSelectBoxProp()
{
	pFont = MFFont_GetDebugFont();
	size.y = MFFont_GetFontHeight(pFont) + 4.f;
	size.x = 6.f;

	changeCallback.clear();

	list.FollowCursor(true);
	list.SetSelectCallback(MakeDelegate(this, &uiSelectBoxProp::SelectItem));
}

uiSelectBoxProp::~uiSelectBoxProp()
{
}

void uiSelectBoxProp::Update()
{
	uiEntity::Update();
}

void uiSelectBoxProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float textHeight = MFFont_GetFontHeight(pFont);

	// draw the frame and selection
	MFPrimitive(PT_TriStrip | PT_Prelit | PT_Untextured);
	MFSetMatrix(state.mat);

	MFBegin(10);

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

	MFEnd();

	// draw items
	if(list.GetSelection() > -1)
		MFFont_DrawText(pFont, 8.f - texelCenter, 2.f - texelCenter, textHeight, state.colour, list.GetItem(list.GetSelection()).CStr(), -1, state.mat);
}

bool uiSelectBoxProp::HandleInputEvent(InputEvent ev, const InputInfo &info)
{
	switch(ev)
	{
		case IE_Down:
			// add list to the top of the root stack
			uiEntity *pRoot = GetEntityManager()->GetRoot();
			pRoot->AddChild(&list);

			// set the list position
			MFMatrix mat;
			GetWorldMatrix(&mat);
			list.SetPos(mat.GetTrans());

			list.SetVisible(true);
			GetEntityManager()->SetExclusiveReceiver(&list);
			return true;
	}

	return false;
}

void uiSelectBoxProp::SelectItem(uiListProp *pList, int item, void *pUserData)
{
	GetEntityManager()->SetExclusiveReceiver(NULL);

	uiEntity *pRoot = GetEntityManager()->GetRoot();
	pRoot->RemoveChild(&list);

	pList->SetVisible(false);

	if(changeCallback)
		changeCallback(this, item, pUserData);
}

void uiSelectBoxProp::SetSelection(int item)
{
	list.SetSelection(item);
}

int uiSelectBoxProp::GetSelection()
{
	return list.GetSelection();
}

void uiSelectBoxProp::AddItem(const char *pItem, void *pUserData)
{
	list.AddItem(pItem, pUserData);
	if(list.GetItemCount() == 1)
		list.SetSelection(0);
}

MFString uiSelectBoxProp::GetItems(uiEntity *pEntity)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	return pSelect->list.GetItems(&pSelect->list);
}

void uiSelectBoxProp::SetItems(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->list.SetItems(&pSelect->list, pArguments);
	pSelect->list.SetSelection(0);
}

void uiSelectBoxProp::ClearItems(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->list.ClearItems(&pSelect->list, pArguments);
}

void uiSelectBoxProp::AddItem(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->list.AddItem(&pSelect->list, pArguments);
	if(pSelect->list.GetItemCount() == 1)
		pSelect->list.SetSelection(0);
}

void uiSelectBoxProp::RemoveItem(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->list.RemoveItem(&pSelect->list, pArguments);
}

MFString uiSelectBoxProp::GetSelected(uiEntity *pEntity)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	return pSelect->list.GetSelected(&pSelect->list);
}

void uiSelectBoxProp::SetSelected(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->list.SetSelected(&pSelect->list, pArguments);
}

MFString uiSelectBoxProp::GetCurrent(uiEntity *pEntity)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	return pSelect->list.GetCurrent(&pSelect->list);
}

void uiSelectBoxProp::SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiSelectBoxProp *pSelect = (uiSelectBoxProp*)pEntity;
	pSelect->pFont = GameData::Get()->GetResourceCache()->FindFont(pArguments->GetString(0).CStr());
	pSelect->size.y = MFFont_GetFontHeight(pSelect->pFont) + 4.f;
	pSelect->size.x = pSelect->size.y * 6.f;

	pSelect->list.SetFont(&pSelect->list, pArguments);
}
