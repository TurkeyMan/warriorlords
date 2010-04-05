#include "Warlords.h"
#include "RequestBox.h"

#include "MFFont.h"
#include "MFRenderer.h"

RequestBox::RequestBox()
: Window(false)
{
	MFRect pos1 = { window.x + 32.f, window.y + window.height - 96.f, 64.f, 64.f };
	MFRect uvs1 = { 0.25f + (.5f/256.f), 0.f + (.5f/256.f), 0.25f, 0.25f };
	pYes = Button::Create(pIcons, &pos1, &uvs1, MFVector::white, Select, this, 0);

	MFRect pos2 = { window.x + window.width - 96.f, window.y + window.height - 96.f, 64.f, 64.f };
	MFRect uvs2 = { 0.5f + (.5f/256.f), 0.f + (.5f/256.f), 0.25f, 0.25f };
	pNo = Button::Create(pIcons, &pos2, &uvs2, MFVector::white, Select, this, 1);
}

RequestBox::~RequestBox()
{
}

bool RequestBox::Draw()
{
	if(!Window::Draw())
		return false;

	pYes->Draw();
	pNo->Draw();

	return true;
}

bool RequestBox::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	return Window::HandleInputEvent(ev, info);
}

void RequestBox::Show(const char *pMessage, SelectCallback *_pSelectCallback)
{
	Window::Show();

	MFString_Copy(message, pMessage);
	pSelectCallback = _pSelectCallback;

	pInputManager->PushReceiver(pYes);
	pInputManager->PushReceiver(pNo);
}

void RequestBox::Hide()
{
	Window::Hide();
}

void RequestBox::Select(int button, void *pUserData, int buttonID)
{
	RequestBox *pThis = (RequestBox*)pUserData;
	pThis->Hide();
	pThis->pSelectCallback(buttonID);
}
