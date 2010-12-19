#include "Warlords.h"
#include "ModelProp.h"
#include "Action.h"
#include "Tools/ResourceCache.h"

void uiModelProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Model", Create, "Entity");

	uiActionManager::RegisterProperty("model", NULL, SetModel, pType);

	uiActionManager::RegisterInstantAction("setmodel", SetModel, pType);
}

uiModelProp::uiModelProp()
{
	pModel = NULL;
}

uiModelProp::~uiModelProp()
{
	if(pModel)
		MFModel_Destroy(pModel);
}

void uiModelProp::Update()
{
	uiEntity::Update();
}

void uiModelProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	MFModel_SetWorldMatrix(pModel, state.mat);
	MFModel_Draw(pModel);
}

void uiModelProp::SetModel(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiModelProp *pModelProp = (uiModelProp*)pEntity;
	pModelProp->pModel = GameData::Get()->GetResourceCache()->FindModel(pArguments->GetString(0).CStr());
}
