#include "Warlords.h"
#include "Entity.h"
#include "Action.h"
#include "Display.h"

#include "ImageProp.h"
#include "ModelProp.h"
#include "TextProp.h"
#include "LayoutProp.h"

#include "MFSystem.h"

Factory<uiEntity> uiEntityManager::entityFactory;

uiDrawState uiEntity::identity;

const char *uiEntity::pAnchorNames[AnchorMax] =
{
	"topleft",
	"topcenter",
	"topright",
	"centerleft",
	"center",
	"centerright",
	"bottomLeft",
	"bottomcenter",
	"bottomright"
};

static const MFVector anchorOffset[uiEntity::AnchorMax] =
{
	{ 0,   0,   0, 0 },
	{ 0.5, 0,   0, 0 },
	{ 1,   0,   0, 0 },
	{ 0,   0.5, 0, 0 },
	{ 0.5, 0.5, 0, 0 },
	{ 1,   0.5, 0, 0 },
	{ 0,   1,   0, 0 },
	{ 0.5, 1,   0, 0 },
	{ 1,   1,   0, 0 }
};

void uiEntity::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Entity", Create);

	uiActionManager::RegisterProperty("position", GetPos, SetPos, pType);
	uiActionManager::RegisterProperty("size", GetSize, SetSize, pType);
	uiActionManager::RegisterProperty("rotation", GetRot, SetRot, pType);
	uiActionManager::RegisterProperty("scale", GetScale, SetScale, pType);
	uiActionManager::RegisterProperty("colour", GetColour, SetColour, pType);
	uiActionManager::RegisterProperty("name", GetName, NULL, pType);
	uiActionManager::RegisterProperty("enabled", GetEnabled, SetEnable, pType);
	uiActionManager::RegisterProperty("visible", GetVisible, SetVisible, pType);
	uiActionManager::RegisterProperty("anchor", NULL, SetAnchor, pType);

	uiActionManager::RegisterInstantAction("setposition", SetPos, pType);
	uiActionManager::RegisterInstantAction("setsize", SetSize, pType);
	uiActionManager::RegisterInstantAction("setrotation", SetRot, pType);
	uiActionManager::RegisterInstantAction("setscale", SetScale, pType);
	uiActionManager::RegisterInstantAction("setcolour", SetColour, pType);
	uiActionManager::RegisterInstantAction("enable", SetEnable, pType);
	uiActionManager::RegisterInstantAction("show", SetVisible, pType);

	uiActionManager::RegisterDeferredAction("move", uiAction_Move::Create, pType);
	uiActionManager::RegisterDeferredAction("fade", uiAction_Fade::Create, pType);
	uiActionManager::RegisterDeferredAction("spin", uiAction_Rotate::Create, pType);
	uiActionManager::RegisterDeferredAction("resize", uiAction_Scale::Create, pType);
}

uiEntity::uiEntity()
{
	pos = MFVector::zero;
	size = MFVector::zero;
	rot = MFVector::zero;
	scale = MFVector::one;
	colour = MFVector::one;
	anchor = TopLeft;
	bEnabled = bVisible = true;
}

uiEntity::~uiEntity()
{
}

void uiEntity::Init(MFIniLine *pEntityData)
{
	uiActionManager *pManager = GetActionManager();

	while(pEntityData)
	{
		if(pEntityData->IsSection("Events"))
		{
			MFIniLine *pEvents = pEntityData->Sub();
			while(pEvents)
			{
				uiEvent *pEvent = &events.push();
				pEvent->pEvent = pEvents->GetString(0);
				pEvent->pScript = pManager->ParseScript(pEvents->GetLineData().CStr());

				pEvents = pEvents->Next();
			}
		}
		else if(pEntityData->IsSection("Children"))
		{
			MFIniLine *pChild = pEntityData->Sub();
			while(pChild)
			{
				if(pChild->IsString(0, "section"))
				{
					MFIniLine *pEntity = pChild->Sub();
					while(pEntity)
					{
						if(pEntity->IsString(0, "name"))
						{
							uiEntity *pNewEntity = GetEntityManager()->Create(pChild->GetString(1), pEntity->GetString(1), this);
							pNewEntity->Init(pChild->Sub());
							break;
						}

						pEntity = pEntity->Next();
					}
				}

				pChild = pChild->Next();
			}
		}
		else
		{
			pManager->SetEntityProperty(this, pEntityData->GetString(0), pEntityData->GetLineData().CStr());
		}

		pEntityData = pEntityData->Next();
	}

	// init all the entities
	int numChildren = children.size();
	for(int a=0; a<numChildren; ++a)
	{
		children[a]->SignalEvent("init", NULL);
	}
}

void uiEntity::Update()
{
}

void uiEntity::Draw(const uiDrawState &state)
{
}

bool uiEntity::HandleInputEvent(InputEvent ev, const InputInfo &info)
{
	switch(ev)
	{
		case IE_Down:
			return SignalEvent("down", NULL);
		case IE_Up:
			return SignalEvent("up", NULL);
		case IE_Tap:
			return SignalEvent("tap", NULL);
	}
	return false;
}

void uiEntity::UpdateEntity()
{
	Update();

	int numChildren = children.size();
	for(int a=0; a<numChildren; ++a)
		children[a]->UpdateEntity();
}

void uiEntity::DrawEntity(const uiDrawState &state)
{
	if(!bVisible)
		return;

	uiDrawState drawState;
	CalculateDrawState(drawState, state);

	Draw(drawState);

	int numChildren = children.size();
	for(int a=0; a<numChildren; ++a)
		children[a]->DrawEntity(drawState);
}

bool uiEntity::HandleInput(InputEvent ev, const InputInfo &info)
{
	if(!bEnabled || !bVisible)
		return false;

	MFMatrix local;
	GetMatrix(&local);
	local.Inverse();

	InputInfo localInfo = info;

	if(info.ev < IE_ButtonTriggered)
	{
		MFRect rect =
		{
			0, 0,
			size.x, size.y
		};

		MFVector v = local.TransformVectorH(MakeVector(localInfo.down.x, localInfo.down.y));
		if(!MFTypes_PointInRect(v.x, v.y, &rect))
			return false;

		localInfo.down.x = v.x;
		localInfo.down.y = v.y;

		switch(info.ev)
		{
			case IE_Drag:
				v = local.TransformVectorH(MakeVector(localInfo.drag.startX, localInfo.drag.startY));
				localInfo.drag.startX = v.x;
				localInfo.drag.startY = v.y;
			case IE_Hover:
			case IE_Up:
				v = local.TransformVectorH(MakeVector(localInfo.hover.deltaX, localInfo.hover.deltaY));
				localInfo.hover.deltaX = v.x;
				localInfo.hover.deltaY = v.y;
		}
	}

	int numChildren = children.size();
	for(int a=numChildren-1; a>=0; --a)
	{
		if(children[a]->HandleInput(ev, localInfo))
			return true;
	}

	return HandleInputEvent(ev, localInfo);
}

bool uiEntity::CalculateDrawState(uiDrawState &output, const uiDrawState &parent)
{
	MFMatrix mat;
	GetMatrix(&mat);
	output.mat.Multiply(mat, parent.mat);
	output.colour = parent.colour * colour;
	return false;
}

uiEntityManager *uiEntity::GetEntityManager()
{
	return GameData::Get()->GetEntityManager();
}

uiActionManager *uiEntity::GetActionManager()
{
	return GameData::Get()->GetActionManager();
}

bool uiEntity::IsType(FactoryType *pExpectedType)
{
	FactoryType *pT = pType;
	while(pT)
	{
		if(pT == pExpectedType)
			return true;
		pT = pT->pParent;
	}
	return false;
}

MFVector uiEntity::GetContainerSize()
{
	if(pParent)
		return pParent->size;

	MFMatrix screenMat;
	GetOrthoMatrix(&screenMat);
	return MakeVector((1.f / screenMat.m[0]) * 2.f, (1.f / -screenMat.m[5]) * 2.f, 1.f, 1.f);
}

bool uiEntity::SignalEvent(const char *pEvent, const char *pParams)
{
//	MFDebug_Assert(false, "parse params!");

	bool bFound = false;
	for(int a=0; a<events.size(); ++a)
	{
		// *** HASH TABLE?!? ***
		if(!MFString_CaseCmp(pEvent, events[a].pEvent))
		{
			GetActionManager()->RunScript(events[a].pScript, this, NULL);
			bFound = true;
		}
	}

	return bFound;
}

void uiEntity::GetPosition(MFVector *pPosition)
{
	*pPosition = pos - size*scale*anchorOffset[anchor];
}

void uiEntity::GetMatrix(MFMatrix *pMat)
{
	pMat->SetRotationYPR(rot.y, rot.x, rot.z);

	MFVector xAxis = pMat->GetXAxis() * scale.x;
	MFVector yAxis = pMat->GetYAxis() * scale.y;
	MFVector zAxis = pMat->GetZAxis() * scale.z;

	pMat->SetXAxis4(xAxis);
	pMat->SetYAxis4(yAxis);
	pMat->SetZAxis4(zAxis);

	MFVector offset = size*anchorOffset[anchor];
	pMat->SetTrans3(pos - (xAxis*offset.x + yAxis*offset.y + zAxis*offset.z));
}


void uiEntityManager::InitManager()
{
	uiEntity::identity.mat = MFMatrix::identity;
	uiEntity::identity.colour = MFVector::white;

	uiEntity::RegisterEntity();
	uiLayoutProp::RegisterEntity();
	uiImageProp::RegisterEntity();
	uiModelProp::RegisterEntity();
	uiTextProp::RegisterEntity();
}

void uiEntityManager::DeinitManager()
{
}

FactoryType *uiEntityManager::RegisterEntityType(const char *pEntityTypeName, Factory_CreateFunc *pCreateFunc, const char *pParentType)
{
	return entityFactory.RegisterType(pEntityTypeName, pCreateFunc, entityFactory.FindType(pParentType));
}

bool uiEntityManager::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	return pRoot->HandleInput(ev, info);
}

void uiEntityManager::Init()
{
	entityPool.Create(256, 2048, 256);

	pRoot = NULL;
	pFocus = NULL;

	MFMatrix screenMat;
	GetOrthoMatrix(&screenMat);
	MFRect rect = { 0.f, 0.f, (1.f / screenMat.m[0]) * 2.f, (1.f / -screenMat.m[5]) * 2.f };
	UpdateRect(&rect);

	pInputManager->PushReceiver(this);
}

void uiEntityManager::Deinit()
{
	entityPool.Destroy();
}

void uiEntityManager::Update()
{
	if(!pRoot)
		return;

	pRoot->UpdateEntity();
}

void uiEntityManager::Draw()
{
	if(!pRoot)
		return;

	uiDrawState state;
	state.mat = MFMatrix::identity;
	state.colour = MFVector::white;

	pRoot->DrawEntity(state);
}

uiEntity *uiEntityManager::Create(const char *pTypeName, const char *pName, uiEntity *pParent)
{
	FactoryType *pType;
	uiEntity *pEntity = entityFactory.Create(pTypeName, &pType);
	if(pEntity)
	{
		// assign its type
		pEntity->pType = pType;

		// give it a name
		pEntity->name = pName;

		// add it to the entity registry
		entityPool.Add(pEntity, pName);

		// set its parent
		pEntity->pParent = pParent;
		if(pParent)
			pParent->children.push(pEntity);
	}
	return pEntity;
}

uiEntity *uiEntityManager::Find(const char *pName)
{
	return entityPool.Find(pName);
}

uiEntity *uiEntityManager::Iterate(uiEntity *pLast)
{
	return entityPool.Next(pLast);
}

void uiEntityManager::Destroy(uiEntity *pEntity)
{
	// remove any actions from the action list that belong to this entity
	uiActionManager *pActionManager = GameData::Get()->GetActionManager();
	pActionManager->DestroyEntity(pEntity);

	// destroy it
	entityPool.Remove(pEntity);
	delete pEntity;
}

void uiEntityManager::LoadRootNode(const char *pRootLayout)
{
	pRoot = uiLayoutProp::CreateLayout("Root", pRootLayout, NULL);
}

MFString uiEntity::GetPos(uiEntity *pEntity)
{
	return MFString::Format("{%s}", pEntity->pos.ToString3());
}

MFString uiEntity::GetSize(uiEntity *pEntity)
{
	return MFString::Format("{%s}", pEntity->size.ToString3());
}

MFString uiEntity::GetRot(uiEntity *pEntity)
{
	return MFString::Format("{%s}", pEntity->rot.ToString3());
}

MFString uiEntity::GetScale(uiEntity *pEntity)
{
	return MFString::Format("{%s}", pEntity->scale.ToString3());
}

MFString uiEntity::GetColour(uiEntity *pEntity)
{
	return MFString::Format("{%s}", pEntity->colour.ToString4());
}

MFString uiEntity::GetName(uiEntity *pEntity)
{
	return pEntity->name;
}

MFString uiEntity::GetVisible(uiEntity *pEntity)
{
	MFString t(pEntity->bVisible ? "true" : "false", true);
	return t;
}

MFString uiEntity::GetEnabled(uiEntity *pEntity)
{
	MFString t(pEntity->bEnabled ? "true" : "false", true);
	return t;
}

void uiEntity::SetPos(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->pos = pArguments->GetVector(0);
}

void uiEntity::SetSize(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->size = pArguments->GetVector(0);
}

void uiEntity::SetRot(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->rot = pArguments->GetVector(0);
}

void uiEntity::SetScale(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->scale = pArguments->GetVector(0);
}

void uiEntity::SetColour(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->colour = pArguments->GetVector(0);
}

void uiEntity::SetEnable(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->bEnabled = pArguments->GetBool(0);
}

void uiEntity::SetVisible(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->bVisible = pArguments->GetBool(0);
}

void uiEntity::SetAnchor(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->anchor = (Anchor)LookupString(pArguments->GetString(0).CStr(), pAnchorNames);
}

void uiAction_Move::Init(uiRuntimeArgs *pParameters)
{
	start = pEntity->pos;
	target = pParameters->GetVector(0);
	time = 0.f;

	bTarget = pParameters->GetNumArgs() > 1;
	if(bTarget)
		length = pParameters->GetFloat(1);
}

bool uiAction_Move::Update()
{
	if(bTarget)
	{
		time += MFSystem_TimeDelta();
		float t = MFMin(time/length, 1.f);
		pEntity->pos = start + (target-start)*t;
		return t >= 1.f;
	}
	else
	{
		pEntity->pos += target * MFSystem_TimeDelta();
		return false;
	}
}

void uiAction_Fade::Init(uiRuntimeArgs *pParameters)
{
	start = pEntity->colour;
	target = pParameters->GetVector(0);
	time = 0.f;

	bTarget = pParameters->GetNumArgs() > 1;
	if(bTarget)
		length = pParameters->GetFloat(1);
}

bool uiAction_Fade::Update()
{
	if(bTarget)
	{
		time += MFSystem_TimeDelta();
		float t = MFMin(time/length, 1.f);
		pEntity->colour = start + (target-start)*t;
		return t >= 1.f;
	}
	else
	{
		pEntity->colour += target * MFSystem_TimeDelta();
		return false;
	}
}

void uiAction_Scale::Init(uiRuntimeArgs *pParameters)
{
	start = pEntity->scale;
	target = pParameters->GetVector(0);
	time = 0.f;

	bTarget = pParameters->GetNumArgs() > 1;
	if(bTarget)
		length = pParameters->GetFloat(1);
}

bool uiAction_Scale::Update()
{
	if(bTarget)
	{
		time += MFSystem_TimeDelta();
		float t = MFMin(time/length, 1.f);
		pEntity->scale = start + (target-start)*t;
		return t >= 1.f;
	}
	else
	{
		pEntity->scale += target * MFSystem_TimeDelta();
		return false;
	}
}

void uiAction_Rotate::Init(uiRuntimeArgs *pParameters)
{
	start = pEntity->rot;
	target = pParameters->GetVector(0);
	time = 0.f;

	bTarget = pParameters->GetNumArgs() > 1;
	if(bTarget)
		length = pParameters->GetFloat(1);
}

bool uiAction_Rotate::Update()
{
	if(bTarget)
	{
		time += MFSystem_TimeDelta();
		float t = MFMin(time/length, 1.f);
		pEntity->rot = start + (target-start)*t;
		return t >= 1.f;
	}
	else
	{
		pEntity->rot += target * MFSystem_TimeDelta();
		return false;
	}
}
