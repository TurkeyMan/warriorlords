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

const float Window::margin = 5.f;

Window::Window(bool _bCloseButton)
{
	bVisible = false;
	bCloseButton = _bCloseButton;
	pCloseButton = NULL;

	pFont = Game::GetCurrent()->GetTextFont();
	pIcons = MFMaterial_Create("Icons");

	GetDisplayRect(&window);
	window.x = window.width*0.5f - 240.f;
	window.y = window.height*0.5f - 160.f;
	window.width = 480.f;
	window.height = 320.f;

	if(bCloseButton)
	{
		MFRect closePos = { window.x + window.width - 32.f, window.y, 32.f, 32.f };
		MFRect closeUVs = { 0.25f + (.5f/256.f), 0.5f + (.5f/256.f), 0.125f, 0.125f };

		pCloseButton = Button::Create(pIcons, &closePos, &closeUVs, MFVector::white, CloseWindow, this);
	}

	AdjustRect_Margin(&window, margin*2.f);
}

Window::~Window()
{
	if(pCloseButton)
		pCloseButton->Destroy();

	MFMaterial_Destroy(pIcons);
}

bool Window::Draw()
{
	if(!bVisible)
		return false;

	MFRect win = window;
	win.x -= margin;
	win.y -= margin;
	win.width += margin*2;
	win.height += margin*2;

	Game::GetCurrent()->DrawWindow(win);

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
