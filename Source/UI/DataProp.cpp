#include "Warlords.h"
#include "DataProp.h"
#include "Action.h"

void uiDataProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Data", Create, "Entity");

	uiActionManager::RegisterProperty("maps", GetMaps, NULL, pType);

//	uiActionManager::RegisterInstantAction("login", LoginAction, pType);
}

uiDataProp::uiDataProp()
{
	pData = GameData::Get();

	bVisible = false;
	size.x = 0.f;
	size.y = 0.f;
}

uiDataProp::~uiDataProp()
{
}

void uiDataProp::Update()
{
	uiEntity::Update();
}

void uiDataProp::Draw(const uiDrawState &state)
{
	return;
}

MFString uiDataProp::GetMaps(uiEntity *pEntity)
{
	uiDataProp *pThis = (uiDataProp*)pEntity;
	return pThis->pData->GetMapList();
}
