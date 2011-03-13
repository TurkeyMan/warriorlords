#include "Warlords.h"
#include "SelectBoxProp.h"
#include "Action.h"

void uiSelectBoxProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("SelectBox", Create, "Entity");

}

uiSelectBoxProp::uiSelectBoxProp()
{
	size.x = 0.f;
	size.y = 0.f;
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

}
