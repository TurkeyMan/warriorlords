#include "Warlords.h"
#include "TextProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

const char *uiTextProp::gJustifyStrings[MFFontJustify_Max] =
{
	"topleft",
	"topcenter",
	"topright",
	"topfull",
	"centerleft",
	"center",
	"centerright",
	"centerfull",
	"bottomleft",
	"bottomcenter",
	"bottomright",
	"bottomfull"
};

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
	pFont = MFFont_GetDebugFont();
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
	pTextProp->textHeight = MFFont_GetFontHeight(pTextProp->pFont);

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
	pTextProp->justification = (MFFontJustify)LookupString(pArguments->GetString(0).CStr(), gJustifyStrings);
}
