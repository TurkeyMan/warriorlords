#include "Warlords.h"
#include "Entity.h"
#include "Action.h"
#include "Display.h"

#include "ImageProp.h"
#include "ModelProp.h"
#include "TextProp.h"
#include "LayoutProp.h"
#include "ButtonProp.h"
#include "StringProp.h"
#include "ListProp.h"
#include "SelectBoxProp.h"

#include "DataProp.h"
#include "SessionProp.h"

#include "MFSystem.h"

extern bool SplitLine(MFIniLine *pLine, MFString &left, MFString &right);

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

const MFVector uiEntity::anchorOffset[AnchorMax] =
{
	MakeVector( 0,   0,   0, 0 ),
	MakeVector( 0.5, 0,   0, 0 ),
	MakeVector( 1,   0,   0, 0 ),
	MakeVector( 0,   0.5, 0, 0 ),
	MakeVector( 0.5, 0.5, 0, 0 ),
	MakeVector( 1,   0.5, 0, 0 ),
	MakeVector( 0,   1,   0, 0 ),
	MakeVector( 0.5, 1,   0, 0 ),
	MakeVector( 1,   1,   0, 0 )
};

void uiEntity::RegisterEntity()
{
	// add some global actions
	uiActionManager::RegisterInstantAction("if", If, NULL);
	uiActionManager::RegisterInstantAction("select", Select, NULL);

	// register base entity
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

	uiActionManager::RegisterInstantAction("setfocus", SetFocus, pType);

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
	bHasFocus = false;
}

uiEntity::~uiEntity()
{
}

void uiEntity::Init(MFIniLine *pEntityData)
{
	while(pEntityData)
	{
		InitLine(pEntityData);

		pEntityData = pEntityData->Next();
	}

	// init all the entities
	int numChildren = children.size();
	for(int a=0; a<numChildren; ++a)
		children[a]->SignalEvent("init", NULL);
}

void uiEntity::InitLine(MFIniLine *pEntityData)
{
	uiActionManager *pManager = GetActionManager();

	if(pEntityData->IsSection("Events"))
	{
		MFIniLine *pEvents = pEntityData->Sub();
		while(pEvents)
		{
			MFString left, right;
			if(SplitLine(pEvents, left, right))
			{
				uiActionScript *pEvent = pManager->ParseScript(left.CStr(), right.CStr());
				events.push(pEvent);
			}

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
			GetEntityManager()->SetFocus(this);
			SignalEvent("ondown", MFString::Format("%d, {%g, %g}", info.buttonID, info.down.x, info.down.y).CStr());
			return true;
		case IE_Up:
			SignalEvent("onup", MFString::Format("%d, {%g, %g}", info.buttonID, info.up.x, info.up.y).CStr());
			return true;
		case IE_Tap:
			SignalEvent("ontap", MFString::Format("%d, {%g, %g}", info.buttonID, info.tap.x, info.tap.y).CStr());
			return true;
		case IE_Hover:
			return true;
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

bool uiEntity::TransformInputInfo(InputInfo &info, bool bCalculateOutside)
{
	if(info.ev < IE_ButtonTriggered)
	{
		MFRect rect =
		{
			0, 0,
			size.x - 1, size.y - 1
		};

		MFMatrix local;
		GetMatrix(&local);
		local.Inverse();

		MFVector v = local.TransformVectorH(MakeVector(info.down.x, info.down.y));
		if(!bCalculateOutside && !MFTypes_PointInRect(v.x, v.y, &rect))
			return false;

		info.down.x = v.x;
		info.down.y = v.y;

		switch(info.ev)
		{
			case IE_Drag:
				v = local.TransformVectorH(MakeVector(info.drag.startX, info.drag.startY));
				info.drag.startX = v.x;
				info.drag.startY = v.y;
			case IE_Hover:
			case IE_Up:
				v = local.TransformVector3(MakeVector(info.hover.deltaX, info.hover.deltaY));
				info.hover.deltaX = v.x;
				info.hover.deltaY = v.y;
		}
	}

	return true;
}

bool uiEntity::FullTransformInputInfo(InputInfo &info, bool bCalculateOutside)
{
	if(pParent && !pParent->FullTransformInputInfo(info, bCalculateOutside))
		return false;
	return TransformInputInfo(info, bCalculateOutside);
}

bool uiEntity::HandleInput(InputEvent ev, const InputInfo &info)
{
	if(!bVisible)
		return false;

	InputInfo localInfo = info;
	if(!TransformInputInfo(localInfo))
		return false;

	if(ev == IE_Hover)
		GetEntityManager()->SetHover(this, MakeVector(localInfo.hover.x, localInfo.hover.y));

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

void uiEntity::RegisterAction(const char *pName, uiActionManager::InstantActionHandler *pHandler)
{
	EntityAction a = { pName, pHandler };
	actions.push(a);
}

bool uiEntity::ExecuteAction(const char *pName, uiRuntimeArgs *pArguments)
{
	for(int a=0; a<actions.size(); ++a)
	{
		if(!MFString_Compare(pName, actions[a].pActionName))
		{
			actions[a].pHandler(this, pArguments);

			return true;
		}
	}

	return false;
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

void uiEntity::AddChild(uiEntity *pEntity)
{
	children.push(pEntity);
	pEntity->pParent = this;
}

void uiEntity::RemoveChild(uiEntity *pEntity)
{
	int a=0;
	for(; a<children.size(); ++a)
	{
		if(children[a] == pEntity)
			break;
	}

	for(; a<children.size()-1; ++a)
		children[a] = children[a + 1];

	if(a < children.size())
		children.pop();

	pEntity->pParent = NULL;
}

uiEntity *uiEntity::FindChild(const char *pName) const
{
	for(int a=0; a<children.size(); ++a)
	{
		if(children[a]->name.CompareInsensitive(pName))
			return children[a];
	}
	return NULL;
}

void uiEntity::SetEnable(bool _bEnabled)
{
	if(bEnabled == _bEnabled)
		return;

	if(!_bEnabled && bHasFocus)
		GetEntityManager()->SetFocus(NULL);

	bEnabled = _bEnabled;
	OnEnable(bEnabled);
	SignalEvent(_bEnabled ? "onenable" : "ondisable", NULL);
}

void uiEntity::SetVisible(bool _bVisible)
{
//	if(bVisible == _bVisible)
//		return;

	bVisible = _bVisible;
	OnShow(bVisible);
	SignalEvent(_bVisible ? "onshow" : "onhide", NULL);
}

bool uiEntity::SignalEvent(const char *pEvent, const char *pParams)
{
	bool bFound = false;
	for(int a=0; a<events.size(); ++a)
	{
		// *** HASH TABLE?!? ***
		if(events[a]->name.CompareInsensitive(pEvent))
		{
			uiActionManager *pActionManager = GetActionManager();
			uiRuntimeArgs *pArgs = pParams ? pActionManager->ParseArgs(pParams, this) : NULL;

			pActionManager->RunScript(events[a], this, pArgs);

			if(pArgs)
				pArgs->Release();

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

void uiEntity::GetWorldMatrix(MFMatrix *pMat)
{
	if(pParent)
	{
		pParent->GetWorldMatrix(pMat);

		MFMatrix mat;
		GetMatrix(&mat);
		pMat->Multiply(mat);
	}
	else
	{
		GetMatrix(pMat);
	}
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
	uiButtonProp::RegisterEntity();
	uiStringProp::RegisterEntity();
	uiListProp::RegisterEntity();
	uiSelectBoxProp::RegisterEntity();

	uiDataProp::RegisterEntity();
	uiSessionProp::RegisterEntity();
}

void uiEntityManager::DeinitManager()
{
}

FactoryType *uiEntityManager::RegisterEntityType(const char *pEntityTypeName, Factory_CreateFunc *pCreateFunc, const char *pParentType)
{
	return entityFactory.RegisterType(pEntityTypeName, pCreateFunc, entityFactory.FindType(pParentType));
}

uiEntity *uiEntityManager::SetFocus(uiEntity *pNewFocus)
{
	if(pFocus == pNewFocus)
		return pFocus;

	uiEntity *pOld = pFocus;
	pFocus = pNewFocus && pNewFocus->bEnabled ? pNewFocus : NULL;

	if(pOld)
	{
		pOld->bHasFocus = false;
		pOld->ChangeFocus(false);
		pOld->SignalEvent("onfocuslost", NULL);
	}

	if(pFocus)
	{
		pFocus->bHasFocus = true;
		pFocus->ChangeFocus(true);
		pFocus->SignalEvent("onfocus", NULL);
	}

	return pOld;
}

uiEntity *uiEntityManager::SetExclusiveReceiver(uiEntity *pReceiver)
{
	pInputManager->SetExclusiveReceiver(pReceiver ? this : NULL);

	uiEntity *pOld = pExclusiveReceiver;
	pExclusiveReceiver = pReceiver;
	return pOld;
}

uiEntity *uiEntityManager::SetExclusiveContactReceiver(int contact, uiEntity *pReceiver)
{
	pInputManager->SetExclusiveContactReceiver(contact, pReceiver ? this : NULL);

	uiEntity *pOld = pContactReceivers[contact];
	pContactReceivers[contact] = pReceiver;
	return pOld;
}

void uiEntityManager::SetHover(uiEntity *pEntity, const MFVector &pos)
{
	pHover = pEntity;
	hoverPos = pos;
}

void uiEntityManager::ClearContactCallback(int contact)
{
	pContactReceivers[contact] = NULL;
}

bool uiEntityManager::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	if(pContactReceivers[info.contact])
	{
		// transform coordinates into entity space...
		InputInfo localInfo = info;
		pContactReceivers[info.contact]->FullTransformInputInfo(localInfo, true);

		if(pContactReceivers[info.contact]->HandleInputEvent(info.ev, localInfo))
			return true;
	}

	if(pExclusiveReceiver)
	{
		// transform coordinates into entity space...
		InputInfo localInfo = info;
		pExclusiveReceiver->FullTransformInputInfo(localInfo, true);

		if(pExclusiveReceiver->HandleInputEvent(info.ev, localInfo))
			return true;
	}

	return pRoot->HandleInput(ev, info);
}

void uiEntityManager::Init()
{
	entityPool.Create(256, 2048, 256);

	pRoot = NULL;
	pFocus = NULL;
	pExclusiveReceiver = NULL;
	pHover = NULL;
	MFZeroMemory(pContactReceivers, sizeof(pContactReceivers));

	MFMatrix screenMat;
	GetOrthoMatrix(&screenMat);
	MFRect rect = { 0.f, 0.f, (1.f / screenMat.m[0]) * 2.f, (1.f / -screenMat.m[5]) * 2.f };
	UpdateRect(&rect);

	pInputManager->RegisterNewContactCallback(MakeDelegate(this, &uiEntityManager::ClearContactCallback));
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

#if defined(_DEBUG)
	if(pHover)
		MFFont_BlitText(MFFont_GetDebugFont(), 10, 10, MFVector::white, MFStr("%s: %.2f %.2f", pHover->name.CStr(), hoverPos.x, hoverPos.y));
	if(pFocus)
		MFFont_BlitText(MFFont_GetDebugFont(), 10, 35, MFVector::white, pFocus->GetName().CStr());
#endif
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
	pRoot = uiLayoutProp::CreateLayout("root", pRootLayout, NULL);
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
	MFVector t = pEntity->rot;
	t *= 57.295779513082320876798154814105f; // convert to degrees

	return MFString::Format("{%s}", t.ToString3());
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
	pEntity->rot *= 0.01745329251994329576923690768489f; // convert to radians
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
	pEntity->SetEnable(pArguments->GetBool(0));
}

void uiEntity::SetVisible(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->SetVisible(pArguments->GetBool(0));
}

void uiEntity::SetAnchor(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->anchor = (Anchor)LookupString(pArguments->GetString(0).CStr(), pAnchorNames);
}

void uiEntity::SetFocus(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	pEntity->GetEntityManager()->SetFocus(pEntity);
}

void uiEntity::If(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	MFString action;

	if(pArguments->GetBool(0))
		action = pArguments->GetString(1);
	else if(pArguments->GetNumArgs() > 2)
		action = pArguments->GetString(2);

	if(action)
	{
		uiActionManager *pAM = GameData::Get()->GetActionManager();

		uiActionScript *pScript = pAM->FindAction(action.CStr());
		if(pScript)
			pAM->RunScript(pScript, pEntity, NULL);
		else if(pEntity)
			pEntity->ExecuteAction(action.CStr(), pArguments);
	}
}

void uiEntity::Select(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	int i = pArguments->GetInt(0);

	if(pArguments->GetNumArgs() > i)
	{
		MFString action = pArguments->GetString(i);

		if(action)
		{
			uiActionManager *pAM = GameData::Get()->GetActionManager();

			uiActionScript *pScript = pAM->FindAction(action.CStr());
			if(pScript)
				pAM->RunScript(pScript, pEntity, NULL);
		}
	}
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
	target *= 0.01745329251994329576923690768489f; // convert to radians

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
