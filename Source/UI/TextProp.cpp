#include "Warlords.h"
#include "TextProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

static const char * gJustifyStrings[MFFontJustify_Max] =
{
	"top_left",
	"top_center",
	"top_right",
	"top_full",
	"center_left",
	"center",
	"center_right",
	"center_full",
	"bottom_left",
	"bottom_center",
	"bottom_right",
	"bottom_full"
};

int GetJustificationFromString(const char *pString)
{
	for(int a=0; a<sizeof(gJustifyStrings) / sizeof(gJustifyStrings[0]); ++a)
	{
		if(!MFString_CaseCmp(pString, gJustifyStrings[a]))
			return a;
	}

	return 0;
}

void uiTextProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("Text", Create, "Entity");

	uiActionManager::RegisterProperty("font", NULL, SetFont, pType);
	uiActionManager::RegisterProperty("text", NULL, SetText, pType);
	uiActionManager::RegisterProperty("textheight", NULL, SetTextHeight, pType);
	uiActionManager::RegisterProperty("justification", NULL, SetJustification, pType);

	uiActionManager::RegisterInstantAction("setfont", SetFont, pType);
	uiActionManager::RegisterInstantAction("settext", SetText, pType);
	uiActionManager::RegisterInstantAction("settextheight", SetTextHeight, pType);
	uiActionManager::RegisterInstantAction("setjustification", SetJustification, pType);
}

uiTextProp::uiTextProp()
{
	pFont = GameData::Get()->GetResourceCache()->FindFont("text");
	textHeight = MFFont_GetFontHeight(pFont);
	justification = MFFontJustify_Top_Left;
	size.x = 0.f;
}

uiTextProp::~uiTextProp()
{
	if(pFont)
		MFFont_Destroy(pFont);
}

void uiTextProp::Update()
{
	uiEntity::Update();
}

void uiTextProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	if(text.NumBytes() > 0)
		MFFont_DrawTextAnchored(pFont, text.CStr(), MFVector::zero, justification, size.x, textHeight, state.colour, -1, state.mat);
}

void uiTextProp::SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiTextProp *pTextProp = (uiTextProp*)pEntity;
	pTextProp->pFont = GameData::Get()->GetResourceCache()->FindFont(pArguments->GetString(0).CStr());

	pTextProp->UpdateSize();
}

void uiTextProp::SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiTextProp *pTextProp = (uiTextProp*)pEntity;
	pTextProp->text = pArguments->GetString(0);

	pTextProp->UpdateSize();
}

void uiTextProp::SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiTextProp *pTextProp = (uiTextProp*)pEntity;
	pTextProp->textHeight = pArguments->GetFloat(0);

	pTextProp->UpdateSize();
}

void uiTextProp::UpdateSize()
{
	const char *pText = text.CStr();
	if(pText)
		size.x = MFFont_GetStringWidth(pFont, pText, textHeight, 0.f, -1, &size.y);	
}

void uiTextProp::SetJustification(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiTextProp *pTextProp = (uiTextProp*)pEntity;
	MFString arg = pArguments->GetString(0);
	const char *pStr = arg.CStr();

	if(!MFString_CaseCmp(pStr, "top_left"))
		pTextProp->justification = MFFontJustify_Top_Left;
	else if(!MFString_CaseCmp(pStr, "top_center"))
		pTextProp->justification = MFFontJustify_Top_Center;
	else if(!MFString_CaseCmp(pStr, "top_right"))
		pTextProp->justification = MFFontJustify_Top_Right;
	else if(!MFString_CaseCmp(pStr, "top_full"))
		pTextProp->justification = MFFontJustify_Top_Full;
	else if(!MFString_CaseCmp(pStr, "center_left"))
		pTextProp->justification = MFFontJustify_Center_Left;
	else if(!MFString_CaseCmp(pStr, "center"))
		pTextProp->justification = MFFontJustify_Center;
	else if(!MFString_CaseCmp(pStr, "center_right"))
		pTextProp->justification = MFFontJustify_Center_Right;
	else if(!MFString_CaseCmp(pStr, "center_full"))
		pTextProp->justification = MFFontJustify_Center_Full;
	else if(!MFString_CaseCmp(pStr, "bottom_Left"))
		pTextProp->justification = MFFontJustify_Bottom_Left;
	else if(!MFString_CaseCmp(pStr, "bottom_center"))
		pTextProp->justification = MFFontJustify_Bottom_Center;
	else if(!MFString_CaseCmp(pStr, "bottom_right"))
		pTextProp->justification = MFFontJustify_Bottom_Right;
	else if(!MFString_CaseCmp(pStr, "bottom_full"))
		pTextProp->justification = MFFontJustify_Bottom_Full;
}
