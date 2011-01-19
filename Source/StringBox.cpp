#include "Warlords.h"
#include "StringBox.h"
#include "Display.h"

#include "MFPrimitive.h"
#include "MFView.h"
#include "MFFont.h"
#include "MFSystem.h"

static float gBlinkTime = 0.4f;

StringBox::StringBox(MFRect &rect, StringEntryLogic::StringType type)
: InputReceiver(rect)
{
	stringLogic.SetType(type);
	stringLogic.SetChangeCallback(MakeDelegate(this, &StringBox::StringChangeCallback));

	pFont = NULL;
	changeCallback.clear();
	tabCallback.clear();

	bVisible = true;
	bEnabled = false;

	bFirstFrame = true;
}

StringBox::~StringBox()
{
}

StringBox *StringBox::Create(MFFont *pFont, MFRect *pPos, StringEntryLogic::StringType type, const char *pDefaultString)
{
	StringBox *pNew = (StringBox*)MFHeap_Alloc(sizeof(StringBox));
	pNew = new(pNew) StringBox(*pPos, type);

	if(pDefaultString)
		pNew->stringLogic.SetString(pDefaultString);

	pNew->pFont = pFont;

	return pNew;
}

void StringBox::Destroy()
{
	this->~StringBox();
	MFHeap_Free(this);
}

bool StringBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Down:
		{
			MFRect client = { 0, 0, rect.width, rect.height };
			if(MFTypes_PointInRect(info.down.x, info.down.y, &client))
			{
				Enable(true);
				return true;
			}
			else
			{
				Enable(false);
				return false;
			}
		}

		case IE_Drag:
			// drag text selection
			return true;
	}

	return false;
}

void StringBox::Update()
{
	if(bEnabled)
	{
		if(MFInput_WasPressed(Key_Tab, IDD_Keyboard))
		{
			if(!bFirstFrame)
			{
				Enable(false);

				if(tabCallback)
					tabCallback(stringLogic.GetString().CStr());
			}
		}
		else
		{
			stringLogic.Update();
		}
	}

	bFirstFrame = false;
}

void StringBox::Draw()
{
	if(!bVisible)
		return;

	const char *pString = stringLogic.GetString().CStr();
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

void StringBox::Enable(bool bEnable)
{
	bEnabled = bEnable;

	bFirstFrame = bEnable;
	gBlinkTime = 0.4f;

	pInputManager->SetExclusiveReceiver(bEnable ? this : NULL);
}

void StringBox::StringChangeCallback(const char *pString)
{
	gBlinkTime = 0.4f;

	if(changeCallback)
		changeCallback(stringLogic.GetString().CStr());
}
