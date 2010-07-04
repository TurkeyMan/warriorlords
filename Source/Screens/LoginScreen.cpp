#include "Warlords.h"
#include "Display.h"
#include "LoginScreen.h"
#include "HomeScreen.h"

#include "MFMaterial.h"
#include "MFRenderer.h"

extern HomeScreen *pHome;

LoginScreen::LoginScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	state = 0;

#if defined(_DEBUG)
	bAutoLogin = true;
#else
	bAutoLogin = false;
#endif

	pPrompt = "Login:";
	pMessage = NULL;

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	MFRect stringPos = { 140, 80, 256, MFFont_GetFontHeight(pFont) };
	pUsername = StringBox::Create(pFont, &stringPos, NULL, NULL);
	pUsername->RegisterTabCallback(TabUser, this);

	stringPos.y += 40.f;
	pPassword = StringBox::Create(pFont, &stringPos, NULL, NULL);
	pPassword->RegisterTabCallback(TabPass, this);

	stringPos.y += 40.f;
	pEmail = StringBox::Create(pFont, &stringPos, NULL, NULL);
	pEmail->RegisterTabCallback(TabEmail, this);

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pLogin = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 0, false);

	// offline button
	pos.x = 64.f + 96.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 1, false);

	// new account button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pNew = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 2, false);

	// create account button
	pos.x = 64.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pCreate = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 3, false);

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 4, false);
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
	state = 0;

	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pUsername);
	pInputManager->PushReceiver(pPassword);
	pInputManager->PushReceiver(pLogin);
	pInputManager->PushReceiver(pOffline);
	pInputManager->PushReceiver(pNew);
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
	if(bAutoLogin)
	{
		AutoLogin();
		bAutoLogin = false;
	}

	pUsername->Update();
	pPassword->Update();
	if(state == 1)
		pEmail->Update();

	return 0;
}

void LoginScreen::Draw()
{
	MFFont_DrawText(pFont, MakeVector(32, 32), 32.f, MFVector::yellow, pPrompt);

	MFFont_DrawText(pFont, MakeVector(48, 80), 24.f, MFVector::yellow, "Username:");
	pUsername->Draw();

	MFFont_DrawText(pFont, MakeVector(48, 120), 24.f, MFVector::yellow, "Password:");
	pPassword->Draw();

	if(state == 1)
	{
		MFFont_DrawText(pFont, MakeVector(48, 160), 24.f, MFVector::yellow, "Email:");
		pEmail->Draw();
	}

	if(state == 0)
	{
		pLogin->Draw();
		pOffline->Draw();
		pNew->Draw();
	}
	else if(state == 1)
	{
		pCreate->Draw();
		pReturn->Draw();
	}

	if(pMessage)
		MFFont_DrawText(pFont, MakeVector(160, 220), 32.f, MFVector::red, pMessage);
}

void LoginScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void LoginScreen::Click(int button, void *pUserData, int buttonID)
{
	LoginScreen *pScreen = (LoginScreen*)pUserData;

	switch(buttonID)
	{
		case 0:
		{
			Session *pSession = new Session();

			// try and login...
			ServerError err = pSession->Login(pScreen->pUsername->GetString(), pScreen->pPassword->GetString());
			if(err != SE_NO_ERROR)
			{
				delete pSession;

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

			if(pSession->IsActive())
			{
				pScreen->pMessage = NULL;

				// set the current session
				Session::SetCurrent(pSession);

				// and continue
				Screen::SetNext(pHome);
			}
			break;
		}

		case 1:
		{
			Session *pSession = new Session();
			pSession->BeginOffline();
			Session::SetCurrent(pSession);

			// begin offline
			Screen::SetNext(pHome);
			break;
		}

		case 2:
		{
			pScreen->state = 1;
			pInputManager->PopReceiver(pScreen);
			pInputManager->PushReceiver(pScreen);
			pInputManager->PushReceiver(pScreen->pUsername);
			pInputManager->PushReceiver(pScreen->pPassword);
			pInputManager->PushReceiver(pScreen->pEmail);
			pInputManager->PushReceiver(pScreen->pCreate);
			pInputManager->PushReceiver(pScreen->pReturn);
			pScreen->pPrompt = "Create Account:";
			pScreen->pMessage = NULL;
			break;
		}

		case 3:
		{
			Session *pSession = new Session();

			const char *pUsername = pScreen->pUsername->GetString();
			const char *pPassword = pScreen->pPassword->GetString();
			const char *pEmail = pScreen->pEmail->GetString();

			// create the account
			uint32 user;
			ServerError err = WLServ_CreateAccount(pUsername, pPassword, pEmail, &user);
			if(err == SE_NO_ERROR)
			{
				// try and login...
				err = pSession->Login(pUsername, pPassword);
			}

			if(err != SE_NO_ERROR)
			{
				delete pSession;

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

			if(pSession->IsActive())
			{
				pScreen->pMessage = NULL;

				// set the current session
				Session::SetCurrent(pSession);

				// and continue
				Screen::SetNext(pHome);
			}
			break;
		}

		case 4:
		{
			pScreen->state = 0;
			pInputManager->PopReceiver(pScreen);
			pInputManager->PushReceiver(pScreen);
			pInputManager->PushReceiver(pScreen->pUsername);
			pInputManager->PushReceiver(pScreen->pPassword);
			pInputManager->PushReceiver(pScreen->pLogin);
			pInputManager->PushReceiver(pScreen->pOffline);
			pInputManager->PushReceiver(pScreen->pNew);
			pScreen->pPrompt = "Login:";
			pScreen->pMessage = NULL;
			break;
		}
	}
}

void LoginScreen::TabUser(const char *pString, void *pUserData)
{
	LoginScreen *pThis = (LoginScreen*)pUserData;
	pThis->pPassword->Enable(true);
}

void LoginScreen::TabPass(const char *pString, void *pUserData)
{
	LoginScreen *pThis = (LoginScreen*)pUserData;
	if(pThis->state == 0)
		pThis->pUsername->Enable(true);
	else if(pThis->state == 1)
		pThis->pEmail->Enable(true);
}

void LoginScreen::TabEmail(const char *pString, void *pUserData)
{
	LoginScreen *pThis = (LoginScreen*)pUserData;
	pThis->pUsername->Enable(true);
}

void LoginScreen::AutoLogin()
{
	Session *pSession = new Session();

	// try and login...
	ServerError err = pSession->Login("Manu", "fred");
	if(err != SE_NO_ERROR)
	{
		delete pSession;

		switch(err)
		{
			case SE_CONNECTION_FAILED:
				pMessage = "Couldn't connect to server!";
				break;
			case SE_INVALID_LOGIN:
				pMessage = "Invalid login!";
				break;
			default:
				pMessage = "Unknown Error!";
				break;
		}
		return;
	}

	if(pSession->IsActive())
	{
		pMessage = NULL;

		// set the current session
		Session::SetCurrent(pSession);

		// and continue
		Screen::SetNext(pHome);
	}
}
