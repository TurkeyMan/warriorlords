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

extern const char *gJustifyStrings[MFFontJustify_Max];

void uiButtonProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Button", Create, "Entity");

	uiActionManager::RegisterProperty("state", GetState, SetState, pType);
	uiActionManager::RegisterProperty("style", NULL, SetStyle, pType);
	uiActionManager::RegisterProperty("hoverMessage", NULL, SetHoverMessage, pType);
	uiActionManager::RegisterProperty("triggerMode", NULL, SetTriggerMode, pType);

	uiActionManager::RegisterProperty("image", NULL, SetImage, pType);

	uiActionManager::RegisterProperty("text", NULL, SetText, pType);
	uiActionManager::RegisterProperty("font", NULL, SetFont, pType);
	uiActionManager::RegisterProperty("textheight", NULL, SetTextHeight, pType);
	uiActionManager::RegisterProperty("justification", NULL, SetJustification, pType);
}

uiButtonProp::uiButtonProp()
{
	pImage = NULL;

	style = Button;
	mode = TriggerOnClick;
	button = 0;
	id = 0;
	state = -1;

	pFont = MFFont_GetDebugFont();
	textHeight = MFFont_GetFontHeight(pFont);
	justification = MFFontJustify_Top_Left;

	imageSize = textSize = MFVector::zero;
}

uiButtonProp::~uiButtonProp()
{
}

void uiButtonProp::Update()
{
	uiEntity::Update();
}

bool uiButtonProp::HandleInputEvent(InputEvent ev, const InputInfo &info)
{
	uiEntity::HandleInputEvent(ev, info);

	if(!bEnabled)
		return false;

	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	if(info.buttonID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			if(button == -1 || info.buttonID == button)
			{
				if(mode == TriggerOnDown)
				{
					SignalEvent("click", MFString::Format("%d, %d", info.buttonID, id).CStr());
				}
				else
				{
					state = info.contact;
//					pInputManager->SetExclusiveContactReceiver(info.contact, this);
				}
			}
			return true;
		case IE_Up:
			if(button == -1 || info.buttonID == button && info.contact == state)
			{
				state = -1;

				MFRect client = { 0, 0, size.x, size.y };
				if(MFTypes_PointInRect(info.up.x, info.up.y, &client))
				{
					SignalEvent("click", MFString::Format("%d, %d", info.buttonID, id).CStr());
				}
			}
			return true;
		case IE_Tap:
		case IE_Drag:
			return true;
	}

	return false;
}

void uiButtonProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	if(pImage)
	{
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

		if(this->state > -1)
			MFSetColour(state.colour * MakeVector(.5f, .5f, .5f, 1.f));
		else
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

	if(text && text.NumBytes() > 0)
		MFFont_DrawTextAnchored(pFont, text.CStr(), MFVector::zero, justification, textSize.x, textHeight, state.colour, -1, state.mat);
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

void uiButtonProp::SetHoverMessage(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{

}

void uiButtonProp::SetTriggerMode(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButton = (uiButtonProp*)pEntity;
	pButton->mode = (TriggerMode)LookupString(pArguments->GetString(0).CStr(), pModes);
}

void uiButtonProp::SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButtonProp = (uiButtonProp*)pEntity;
	pButtonProp->pImage = GameData::Get()->GetResourceCache()->FindMaterial(pArguments->GetString(0).CStr());

	if(pButtonProp->pImage)
	{
		MFTexture *pTex;
		int diffuse = MFMaterial_GetParameterIndexFromName(pButtonProp->pImage, "diffuseMap");
		MFMaterial_GetParameter(pButtonProp->pImage, diffuse, 0, &pTex);
		if(pTex)
		{
			int width, height;
			MFTexture_GetTextureDimensions(pTex, &width, &height);

			pButtonProp->imageSize.x = (float)width;
			pButtonProp->imageSize.y = (float)height;
		}
	}
	else
		pButtonProp->imageSize = MFVector::zero;

	pButtonProp->UpdateSize();
}

void uiButtonProp::SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButtonProp = (uiButtonProp*)pEntity;
	pButtonProp->text = pArguments->GetString(0);

	pButtonProp->UpdateTextSize();
}


void uiButtonProp::SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButtonProp = (uiButtonProp*)pEntity;
	pButtonProp->pFont = GameData::Get()->GetResourceCache()->FindFont(pArguments->GetString(0).CStr());
	pButtonProp->textHeight = MFFont_GetFontHeight(pButtonProp->pFont);

	pButtonProp->UpdateTextSize();
}

void uiButtonProp::SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButtonProp = (uiButtonProp*)pEntity;
	pButtonProp->textHeight = pArguments->GetFloat(0);

	pButtonProp->UpdateTextSize();
}

void uiButtonProp::SetJustification(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiButtonProp *pButtonProp = (uiButtonProp*)pEntity;
	pButtonProp->justification = (MFFontJustify)LookupString(pArguments->GetString(0).CStr(), gJustifyStrings);
}

void uiButtonProp::UpdateTextSize()
{
	const char *pText = text.CStr();
	if(pText)
		textSize.x = MFFont_GetStringWidth(pFont, pText, textHeight, 0.f, -1, &textSize.y);	
	else
		textSize = MFVector::zero;

	UpdateSize();
}

void uiButtonProp::UpdateSize()
{
	size = MFMax(imageSize, textSize);
}
