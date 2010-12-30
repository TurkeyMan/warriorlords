#include "Warlords.h"
#include "ButtonProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

#include "MFRenderer.h"
#include "MFTexture.h"
#include "MFPrimitive.h"

const char *uiButtonProp::pStyles[MaxStyle] =
{
	"button",
	"statebutton",
	"checkbox"
};

const char *uiButtonProp::pModes[MaxMode] =
{
	"ondown",
	"onclick"
};

void uiButtonProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Button", Create, "Entity");

	uiActionManager::RegisterProperty("state", GetState, SetState, pType);
	uiActionManager::RegisterProperty("style", NULL, SetStyle, pType);

	uiActionManager::RegisterInstantAction("setimage", SetImage, pType);
}

uiButtonProp::uiButtonProp()
{
	pImage = NULL;
}

uiButtonProp::~uiButtonProp()
{
	if(pImage)
		MFMaterial_Destroy(pImage);
}

void uiButtonProp::Update()
{
	uiEntity::Update();
}

void uiButtonProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	MFMaterial_SetMaterial(pImage);

	float xc = 0.f, yc = 0.f;
	if(scale == MFVector::one)
	{
		float texelCenter = MFRenderer_GetTexelCenterOffset();
		xc = texelCenter / size.x;
		yc = texelCenter / size.y;
	}

	MFPrimitive(PT_TriStrip | PT_Prelit);

	MFSetMatrix(state.mat);

	MFBegin(4);

	MFSetColour(state.colour);

	MFSetTexCoord1(xc, yc);
	MFSetPosition(0, 0, 0);

	MFSetTexCoord1(1+xc, yc);
	MFSetPosition(imageSize.x, 0, 0);

	MFSetTexCoord1(xc, 1+yc);
	MFSetPosition(0, imageSize.y, 0);

	MFSetTexCoord1(1+xc, 1+yc);
	MFSetPosition(imageSize.x, imageSize.y, 0);

	MFEnd();
}

void uiButtonProp::SetState(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButton = (uiButtonProp*)pEntity;
	pButton->state = pArguments->GetInt(0);
}

MFString uiButtonProp::GetState(uiEntity *pEntity)
{
	uiButtonProp *pButton = (uiButtonProp*)pEntity;
	return MFString::Format("%d", pButton->state);
}

void uiButtonProp::SetStyle(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButton = (uiButtonProp*)pEntity;
	pButton->style = (Style)LookupString(pArguments->GetString(0).CStr(), pStyles);
}

void uiButtonProp::SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pImageProp = (uiButtonProp*)pEntity;
	pImageProp->pImage = GameData::Get()->GetResourceCache()->FindMaterial(pArguments->GetString(0).CStr());

	if(pImageProp->pImage)
	{
		MFTexture *pTex;
		int diffuse = MFMaterial_GetParameterIndexFromName(pImageProp->pImage, "diffuseMap");
		MFMaterial_GetParameter(pImageProp->pImage, diffuse, 0, &pTex);
		if(pTex)
		{
			int width, height;
			MFTexture_GetTextureDimensions(pTex, &width, &height);

			pImageProp->imageSize.x = (float)width;
			pImageProp->imageSize.y = (float)height;
		}
	}
}

void uiButtonProp::SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pImageProp = (uiButtonProp*)pEntity;
	
}

void uiButtonProp::SetHoverMessage(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{

}

void uiButtonProp::SetTriggerMode(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButton = (uiButtonProp*)pEntity;
	pButton->mode = (TriggerMode)LookupString(pArguments->GetString(0).CStr(), pModes);
}
