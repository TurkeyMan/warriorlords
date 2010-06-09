#include "Warlords.h"
#include "Display.h"
#include "LoginScreen.h"
#include "MenuScreen.h"
#include "ServerRequest.h"

#include "MFMaterial.h"
#include "MFRenderer.h"

extern MenuScreen *pMenu;

LoginScreen::LoginScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	pMessage = NULL;

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	MFRect stringPos = { 128, 128, 256, MFFont_GetFontHeight(pFont) };
	pUsername = StringBox::Create(pFont, &stringPos, NULL, NULL);
	pUsername->RegisterTabCallback(TabUser, this);

	stringPos.y += 40.f;
	pPassword = StringBox::Create(pFont, &stringPos, NULL, NULL);
	pPassword->RegisterTabCallback(TabPass, this);

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 16.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pLogin = Button::Create(pIcons, &pos, &uvs, MFVector::one, Login, this, 0, false);

	// offline button
	pos.x = 32.f + 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, Offline, this, 1, false);
}

LoginScreen::~LoginScreen()
{
	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);

	pUsername->Destroy();
	pPassword->Destroy();
	pLogin->Destroy();
	pOffline->Destroy();
}

void LoginScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pUsername);
	pInputManager->PushReceiver(pPassword);
	pInputManager->PushReceiver(pLogin);
	pInputManager->PushReceiver(pOffline);
}

bool LoginScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	switch(ev)
	{
		case IE_Tap:
		{
			break;
		}
	}

	return false;
}

int LoginScreen::Update()
{
	pUsername->Update();
	pPassword->Update();

	return 0;
}

void LoginScreen::Draw()
{
	pUsername->Draw();
	pPassword->Draw();
	pLogin->Draw();
	pOffline->Draw();

	if(pMessage)
		MFFont_DrawText(pFont, MakeVector(32, 32), 32.f, MFVector::red, pMessage);
}

void LoginScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void LoginScreen::Login(int button, void *pUserData, int buttonID)
{
	LoginScreen *pScreen = (LoginScreen*)pUserData;

	// try and login...
	uint32 user;
	ServerError err = WLServ_Login(pScreen->pUsername->GetString(), pScreen->pPassword->GetString(), &user);

	if(err != SE_NO_ERROR)
	{
		switch(err)
		{
			case SE_CONNECTION_FAILED:
				pScreen->pMessage = "Couldn't connect to server!";
				break;
			case SE_INVALID_LOGIN:
				pScreen->pMessage = "Invalid login!";
				break;
			default:
				pScreen->pMessage = "Unknown Error!";
				break;
		}
		return;
	}

	pScreen->pMessage = NULL;

	// login and continue
	Screen::SetNext(pMenu);
}

void LoginScreen::Offline(int button, void *pUserData, int buttonID)
{
	LoginScreen *pScreen = (LoginScreen*)pUserData;

	// begin offline
	Screen::SetNext(pMenu);
}

void LoginScreen::TabUser(const char *pString, void *pUserData)
{
	LoginScreen *pThis = (LoginScreen*)pUserData;
	pThis->pPassword->Enable(true);
}

void LoginScreen::TabPass(const char *pString, void *pUserData)
{
	LoginScreen *pThis = (LoginScreen*)pUserData;
	pThis->pUsername->Enable(true);
}
