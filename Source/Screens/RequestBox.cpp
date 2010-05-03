#include "Warlords.h"
#include "RequestBox.h"

#include "MFFont.h"
#include "MFRenderer.h"

RequestBox::RequestBox()
: Window(false)
{
	window.x += window.width*0.125f;
	window.y += window.height*0.125f;
	window.width *= 0.75f;
	window.height *= 0.75f;

	MFRect pos1 = { window.x + 16.f, window.y + window.height - 80.f, 64.f, 64.f };
	MFRect uvs1 = { 0.25f + (.5f/256.f), 0.f + (.5f/256.f), 0.25f, 0.25f };
	pYes = Button::Create(pIcons, &pos1, &uvs1, MFVector::white, Select, this, 0);

	MFRect pos2 = { window.x + window.width - 80.f, window.y + window.height - 80.f, 64.f, 64.f };
	MFRect uvs2 = { 0.5f + (.5f/256.f), 0.f + (.5f/256.f), 0.25f, 0.25f };
	pNo = Button::Create(pIcons, &pos2, &uvs2, MFVector::white, Select, this, 1);
}

RequestBox::~RequestBox()
{
	pYes->Destroy();
	pNo->Destroy();
}

bool RequestBox::DrawContent()
{
	MFFont_DrawTextJustified(pFont, message, MakeVector(window.x + 16.f, window.y + 32.f), window.width - 32.f, window.height - 128.f, MFFontJustify_Top_Center, MFFont_GetFontHeight(pFont) * 1.5f, MFVector::white);

	pYes->Draw();
	if(!bNotification)
		pNo->Draw();

	return true;
}

bool RequestBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	return Window::HandleInputEvent(ev, info);
}

void RequestBox::Show(const char *pMessage, SelectCallback *_pSelectCallback, bool _bNotification, void *_pUserData)
{
	Window::Show();

	MFString_Copy(message, pMessage);
	pSelectCallback = _pSelectCallback;
	bNotification = _bNotification;
	pUserData = _pUserData;

	pInputManager->PushReceiver(pYes);
	if(!bNotification)
		pInputManager->PushReceiver(pNo);
}

void RequestBox::Hide()
{
	Window::Hide();
}

void RequestBox::Select(int button, void *_pUserData, int buttonID)
{
	RequestBox *pThis = (RequestBox*)_pUserData;

	pThis->Hide();

	if(pThis->pSelectCallback)
		pThis->pSelectCallback(buttonID, pThis->pUserData);
}
