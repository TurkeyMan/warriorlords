#include "Warlords.h"
#include "StringBox.h"

#include "MFPrimitive.h"
#include "MFView.h"
#include "MFFont.h"
#include "Display.h"
#include "MFSystem.h"

static float gBlinkTime = 0.4f;

StringBox::StringBox(MFRect &rect, StringEntryLogic::StringType type)
: InputReceiver(rect), stringLogic(256, type)
{
	stringLogic.SetChangeCallback(StringChangeCallback, this);

	pFont = NULL;
	pCallback = NULL;
	pUserData = NULL;

	bVisible = true;
	bEnabled = false;
}

StringBox::~StringBox()
{
}

StringBox *StringBox::Create(MFFont *pFont, MFRect *pPos, ChangeCallback *pCallback, void *pUserData, StringEntryLogic::StringType type, const char *pDefaultString)
{
	StringBox *pNew = (StringBox*)MFHeap_Alloc(sizeof(StringBox));
	pNew = new(pNew) StringBox(*pPos, type);

	if(pDefaultString)
		pNew->stringLogic.SetString(pDefaultString);

	pNew->pFont = pFont;
	pNew->pCallback = pCallback;
	pNew->pUserData = pUserData;

	return pNew;
}

void StringBox::Destroy()
{
	delete this;
}

bool StringBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
			Enable(true);

			pInputManager->SetExclusiveContactReceiver(info.contact, this);

			// set cursor pos
			return true;

		case IE_Drag:
			// drag text selection
			return true;
	}

	return false;
}

void StringBox::Update()
{
	if(bEnabled)
		stringLogic.Update();
}

void StringBox::Draw()
{
	if(!bVisible)
		return;

	const char *pString = stringLogic.GetString();
	float height = MFFont_GetFontHeight(pFont);
	int cursorPos = stringLogic.GetCursorPos();
	int selectionStart, selectionEnd;
	stringLogic.GetSelection(&selectionStart, &selectionEnd);

	MFPrimitive_DrawUntexturedQuad(rect.x - 1, rect.y - 1, rect.width + 2, rect.height + 2, MFVector::white);
	MFPrimitive_DrawUntexturedQuad(rect.x + 1, rect.y + 1, rect.width - 2, rect.height - 2, MFVector::black);

	if(bEnabled)
	{
		// draw selection (if selected)
		if(selectionStart != selectionEnd)
		{
			int selMin = MFMin(selectionStart, selectionEnd);
			int selMax = MFMax(selectionStart, selectionEnd);

			float selMinX = MFFont_GetStringWidth(pFont, pString, height, 10000, selMin);
			float selMaxX = MFFont_GetStringWidth(pFont, pString, height, 10000, selMax);
			MFPrimitive_DrawUntexturedQuad(rect.x+selMinX, rect.y, selMaxX-selMinX, height, MakeVector(0,0,0.6f,1));
		}
	}

	// draw text
	MFFont_BlitText(pFont, (int)rect.x, (int)rect.y, MFVector::white, pString);

	if(bEnabled)
	{
		// blink cursor
		gBlinkTime -= MFSystem_TimeDelta();
		if(gBlinkTime < -0.4f) gBlinkTime += 0.8f;
		bool bCursor = gBlinkTime > 0.0f;

		// draw cursor
		if(bCursor)
		{
			// render cursor
			float cursorX = MFFont_GetStringWidth(pFont, pString, height, 10000, cursorPos);
			MFPrimitive_DrawUntexturedQuad(rect.x+cursorX, rect.y, 2, height, MFVector::white);
		}
	}
}

void StringBox::StringChangeCallback(const char *pString, void *pUserData)
{
	StringBox *pStringBox = (StringBox*)pUserData;

	gBlinkTime = 0.4f;

	if(pStringBox->pCallback)
		pStringBox->pCallback(pStringBox->stringLogic.GetString(), pStringBox->pUserData);
}
