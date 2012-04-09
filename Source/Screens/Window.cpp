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

	pIcons = MFMaterial_Create("Icons");

	if(bCloseButton)
	{
		float texelCenter = MFRenderer_GetTexelCenterOffset() / 256.f;
		MFRect closePos = { 0.f, 0.f, 32.f, 32.f };
		MFRect closeUVs = { 0.25f + texelCenter, 0.25f + texelCenter, 0.125f, 0.125f };

		pCloseButton = Button::Create(pIcons, &closePos, &closeUVs, MFVector::white);
		pCloseButton->SetClickCallback(MakeDelegate(this, &Window::CloseWindow));
	}

	SetWindowSize(480.f - margin*2.f, 320.f - margin*2.f);
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

//	MFPrimitive_DrawUntexturedQuad(win.x, win.y, win.width, win.height, MakeVector(0,0,1,0.3f));
//	MFPrimitive_DrawUntexturedQuad(window.x, window.y, window.width, window.height, MakeVector(0,0,1,0.3f));

	// setup ortho rect to target window?
	//...

	bool bDidDraw = DrawContent();

	if(bCloseButton)
		pCloseButton->Draw();

	return bDidDraw;
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

void Window::CloseWindow(int button, int buttonID)
{
	Hide();
}

void Window::SetWindowSize(float width, float height)
{
	GetDisplayRect(&window);
	window.x = window.width*0.5f - width*0.5f;
	window.y = window.height*0.5f - height*0.5f;
	window.width = width;
	window.height = height;

	if(bCloseButton)
	{
		MFRect closePos = { window.x + window.width + margin - 32.f, window.y - margin, 32.f, 32.f };
		pCloseButton->SetPos(&closePos);
	}
}
