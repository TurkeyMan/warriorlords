#include "Warlords.h"
#include "Window.h"
#include "Display.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"
#include "MFRenderer.h"

Window::Window(bool _bCloseButton)
{
	bVisible = false;
	bCloseButton = _bCloseButton;

	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;

	pIcons = MFMaterial_Create("Icons");
	MFRect closePos = { window.x + window.width - 32.f, window.y, 32.f, 32.f };
	MFRect closeUVs = { 0.25f + (.5f/256.f), 0.5f + (.5f/256.f), 0.125f, 0.125f };

	pCloseButton = Button::Create(pIcons, &closePos, &closeUVs, MFVector::white, CloseWindow, this);
}

Window::~Window()
{
	pCloseButton->Destroy();
	pCloseButton = NULL;

	MFMaterial_Destroy(pIcons);
}

bool Window::Draw()
{
	if(!bVisible)
		return false;

	Game::GetCurrent()->DrawWindow(window);

	if(bCloseButton)
		pCloseButton->Draw();

	return true;
}

bool Window::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	return true;
}

void Window::Show()
{
	bVisible = true;

	pInputManager->PushReceiver(this);
	if(bCloseButton)
		pInputManager->PushReceiver(pCloseButton);
}

void Window::Hide()
{
	bVisible = false;

	pInputManager->PopReceiver(this);
}

void Window::CloseWindow(int button, void *pUserData, int buttonID)
{
	Window *pWindow = (Window*)pUserData;
	pWindow->Hide();
}
