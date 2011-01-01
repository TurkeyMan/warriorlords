#include "Warlords.h"
#include "ImageProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

#include "MFRenderer.h"
#include "MFTexture.h"
#include "MFPrimitive.h"

void uiImageProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Image", Create, "Entity");

	uiActionManager::RegisterProperty("image", NULL, SetImage, pType);
	uiActionManager::RegisterProperty("orientation", NULL, SetOrientation, pType);
}

uiImageProp::uiImageProp()
{
	pImage = NULL;
	size.x = 0.f;
	size.y = 0.f;
	orientation = Normal;
	imageSize = MFVector::zero;
}

uiImageProp::~uiImageProp()
{
}

void uiImageProp::Update()
{
	uiEntity::Update();
}

void uiImageProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	MFMaterial_SetMaterial(pImage);

	float xc = 0.f, yc = 0.f;
	if(imageSize == size && scale == MFVector::one)
	{
		float texelCenter = MFRenderer_GetTexelCenterOffset();
		xc = texelCenter / size.x;
		yc = texelCenter / size.y;
	}

	float xs, ys, xd, yd, w, h;

	switch(orientation)
	{
		case Normal:
			xs = xc;
			ys = yc;
			xd = 1 + xc;
			yd = 1 + yc;
			w = size.x;
			h = size.y;
			break;
		case Rotate_90cw:
			xs = xc;
			ys = yc;
			xd = 1 + xc;
			yd = 1 + yc;
			w = size.y;
			h = size.x;
			break;
		case Rotate_180:
			xs = xc;
			ys = yc;
			xd = 1 + xc;
			yd = 1 + yc;
			w = size.x;
			h = size.y;
			break;
		case Rotate_90ccw:
			xs = xc;
			ys = yc;
			xd = 1 + xc;
			yd = 1 + yc;
			w = size.y;
			h = size.x;
			break;
		case HFlip:
			xs = 1 - xc;
			ys = yc;
			xd = -xc;
			yd = 1 + yc;
			w = size.x;
			h = size.y;
			break;
		case VFlip:
			xs = xc;
			ys = 1 - yc;
			xd = 1 + xc;
			yd = -yc;
			w = size.x;
			h = size.y;
			break;
	}

	MFPrimitive(PT_TriStrip | PT_Prelit);

	MFSetMatrix(state.mat);

	MFBegin(4);

	MFSetColour(state.colour);

	MFSetTexCoord1(xs, ys);
	MFSetPosition(0, 0, 0);

	MFSetTexCoord1(xd, ys);
	MFSetPosition(w, 0, 0);

	MFSetTexCoord1(xs, yd);
	MFSetPosition(0, h, 0);

	MFSetTexCoord1(xd, yd);
	MFSetPosition(w, h, 0);

	MFEnd();
}

void uiImageProp::SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiImageProp *pImageProp = (uiImageProp*)pEntity;
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

			pImageProp->size.x = pImageProp->imageSize.x = (float)width;
			pImageProp->size.y = pImageProp->imageSize.y = (float)height;
		}
	}
}

void uiImageProp::SetOrientation(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiImageProp *pImageProp = (uiImageProp*)pEntity;
	MFString arg = pArguments->GetString(0);

	if(arg == "normal")
		pImageProp->orientation = Normal;
	else if(arg == "rotate_90cw")
		pImageProp->orientation = Rotate_90cw;
	else if(arg == "rotate_180")
		pImageProp->orientation = Rotate_180;
	else if(arg == "rotate_90ccw")
		pImageProp->orientation = Rotate_90ccw;
	else if(arg == "hflip")
		pImageProp->orientation = HFlip;
	else if(arg == "vflip")
		pImageProp->orientation = VFlip;
}
