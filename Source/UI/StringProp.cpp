#include "Warlords.h"
#include "StringProp.h"
#include "Action.h"
#include "../Tools/ResourceCache.h"

#include "MFRenderer.h"
#include "MFTexture.h"
#include "MFPrimitive.h"
#include "MFSystem.h"

static const char *gTypeStrings[MFFontJustify_Max] =
{
	"regular",
	"multiline",
	"numeric",
	"password"
};

int GetJustificationFromString(const char *pString);

float uiStringProp::blinkTime = 0.4f;

void uiStringProp::RegisterEntity()
{
	FactoryType *pType = uiEntityManager::RegisterEntityType("String", Create, "Entity");

	uiActionManager::RegisterProperty("text", GetText, SetText, pType);
	uiActionManager::RegisterProperty("empty", GetEmpty, NULL, pType);
	uiActionManager::RegisterProperty("font", NULL, SetFont, pType);
	uiActionManager::RegisterProperty("textheight", NULL, SetTextHeight, pType);
	uiActionManager::RegisterProperty("type", NULL, SetType, pType);
}

uiStringProp::uiStringProp()
{
	stringLogic.SetChangeCallback(MakeDelegate(this, &uiStringProp::StringChangeCallback));

	pFont = MFFont_GetDebugFont();
	textHeight = MFFont_GetFontHeight(pFont);
	size.y = textHeight + 4;
	size.x = size.y * 5.f;

	changeCallback.clear();
	tabCallback.clear();

	bHandleTab = true;
	bFirstFrame = true;
}

uiStringProp::~uiStringProp()
{
}

bool uiStringProp::HandleInputEvent(InputEvent ev, const InputInfo &info)
{
	if(!bEnabled)
		return false;

	switch(ev)
	{
		case IE_Down:
		{
			// set the cursor pos
			bool bUpdateSelection = MFInput_Read(Key_LShift, IDD_Keyboard) || MFInput_Read(Key_RShift, IDD_Keyboard);
			UpdateCursorPos(info.down.x, bUpdateSelection);

			// allow proper dragging
			GetEntityManager()->SetExclusiveContactReceiver(info.contact, this);

			blinkTime = 0.4f;
			break;
		}

		case IE_Drag:
			// drag text selection
			UpdateCursorPos(info.drag.x, true);
			blinkTime = 0.4f;
			return true;
	}

	return uiEntity::HandleInputEvent(ev, info);
}

bool uiStringProp::ChangeFocus(bool bGainFocus)
{
	if(bGainFocus)
		bFirstFrame = true;
	return true;
}

void uiStringProp::UpdateCursorPos(float x, bool bUpdateSelection)
{
	MFString text = stringLogic.GetRenderString();
	const char *pString = text.CStr();

	float magnitude = 10000000.f, downPos = x - 2;
	int offset, len = text.NumChars();
	for(int a=0; a<=len; ++a)
	{
		float x = MFFont_GetStringWidth(pFont, pString, textHeight, 100000, a);
		float m = MFAbs(x - downPos);
		if(m < magnitude)
		{
			magnitude = m;
			offset = a;
		}
		else
			break;
	}

	stringLogic.SetCursorPos(offset, bUpdateSelection);
}

void uiStringProp::Update()
{
	uiEntity::Update();

	if(bHasFocus)
	{
		if(MFInput_WasPressed(Key_Tab, IDD_Keyboard) && bHandleTab)
		{
			if(!bFirstFrame)
			{
				if(tabCallback)
					tabCallback(stringLogic.GetString().CStr());

				SignalEvent("ontab", NULL);
			}
		}
		else
		{
			stringLogic.Update();
		}
	}

	bFirstFrame = false;
}

void uiStringProp::Draw(const uiDrawState &state)
{
	uiEntity::Draw(state);

	MFString text = stringLogic.GetRenderString();
	const char *pString = text.CStr();

	int cursorPos = stringLogic.GetCursorPos();

	int selectionStart, selectionEnd;
	stringLogic.GetSelection(&selectionStart, &selectionEnd);

	bool bDrawSelection = selectionStart != selectionEnd;

	// draw the frame and selection
	MFPrimitive(PT_TriStrip | PT_Prelit | PT_Untextured);
	MFSetMatrix(state.mat);

	MFBegin(bDrawSelection ? 16 : 10);

	MFSetColour(MFVector::white);
	MFSetPosition(0.f, 0.f, 0);
	MFSetPosition(size.x, 0.f, 0);
	MFSetPosition(0.f, size.y, 0);
	MFSetPosition(size.x, size.y, 0);

	// degen
	MFSetPosition(0.f+size.x, 0.f+size.y, 0);
	MFSetPosition(2.f, 2.f, 0);

	MFSetColour(MFVector::black);
	MFSetPosition(2.f, 2.f, 0);
	MFSetPosition(size.x-2.f, 2.f, 0);
	MFSetPosition(2.f, size.y-2.f, 0);
	MFSetPosition(size.x-2.f, size.y-2.f, 0);

	if(bDrawSelection)
	{
		// draw selection (if selected)
		int selMin = MFMin(selectionStart, selectionEnd);
		int selMax = MFMax(selectionStart, selectionEnd);

		float selMinX = MFFont_GetStringWidth(pFont, pString, textHeight, 10000, selMin);
		float selMaxX = MFFont_GetStringWidth(pFont, pString, textHeight, 10000, selMax);

		// degen
		MFSetPosition(size.x-2.f, size.y-2.f, 0);
		MFSetPosition(2.f+selMinX, 2.f, 0);

		if(bHasFocus)
			MFSetColour(0, 0, 0.6f, 1);
		else
			MFSetColour(0.4f, 0.4f, 0.4f, 1);
		MFSetPosition(2.f+selMinX, 2.f, 0);
		MFSetPosition(2.f+selMaxX, 2.f, 0);
		MFSetPosition(2.f+selMinX, 2.f+textHeight, 0);
		MFSetPosition(2.f+selMaxX, 2.f+textHeight, 0);
	}

	MFEnd();

	// draw text
	MFFont_DrawText(pFont, 2.f, 2.f, textHeight, bEnabled ? state.colour : MFVector::grey, pString, -1, state.mat);

	if(bHasFocus)
	{
		// blink cursor
		blinkTime -= MFSystem_TimeDelta();
		if(blinkTime < -0.4f) blinkTime += 0.8f;
		bool bCursor = blinkTime > 0.0f;

		// draw cursor
		if(bCursor)
		{
			float cursorX = MFFont_GetStringWidth(pFont, pString, textHeight, 10000, cursorPos);

			// render cursor
			MFPrimitive(PT_TriStrip | PT_Prelit | PT_Untextured);
			MFSetMatrix(state.mat);

			MFBegin(4);

			MFSetColour(MFVector::white);
			MFSetPosition(2.f+cursorX, 3.f, 0);
			MFSetPosition(4.f+cursorX, 3.f, 0);
			MFSetPosition(2.f+cursorX, 1.f+textHeight, 0);
			MFSetPosition(4.f+cursorX, 1.f+textHeight, 0);

			MFEnd();
		}
	}
}

void uiStringProp::StringChangeCallback(const char *pString)
{
	blinkTime = 0.4f;

	if(changeCallback)
		changeCallback(stringLogic.GetString().CStr());

	SignalEvent("onchange", MFString::Format("\"%s\"", stringLogic.GetString().CStr()).CStr());
}

void uiStringProp::SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiStringProp *pStringProp = (uiStringProp*)pEntity;
	pStringProp->SetString(pArguments->GetString(0));
}

MFString uiStringProp::GetEmpty(uiEntity *pEntity)
{
	uiStringProp *pText = (uiStringProp*)pEntity;
	return pText->stringLogic.GetString().IsEmpty() ? "true" : "false";
}

MFString uiStringProp::GetText(uiEntity *pEntity)
{
	uiStringProp *pStringProp = (uiStringProp*)pEntity;
	return pStringProp->stringLogic.GetString();
}

void uiStringProp::SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiStringProp *pStringProp = (uiStringProp*)pEntity;
	pStringProp->pFont = GameData::Get()->GetResourceCache()->FindFont(pArguments->GetString(0).CStr());
	pStringProp->textHeight = MFFont_GetFontHeight(pStringProp->pFont);
	pStringProp->size.y = pStringProp->textHeight + 4.f;
}

void uiStringProp::SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiStringProp *pStringProp = (uiStringProp*)pEntity;
	pStringProp->textHeight = pArguments->GetFloat(0);
}

void uiStringProp::SetType(uiEntity *pEntity, uiRuntimeArgs *pArguments)
{
	uiStringProp *pStringProp = (uiStringProp*)pEntity;

	int type = LookupString(pArguments->GetString(0).CStr(), gTypeStrings);
	pStringProp->SetType((StringEntryLogic::StringType)type);
	pStringProp->bHandleTab = type != 1;
}
