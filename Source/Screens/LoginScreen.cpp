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

	createRequest.SetCompleteDelegate(MakeDelegate(this, &LoginScreen::CreateComplete));

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
	pUsername = StringBox::Create(pFont, &stringPos);
	pUsername->SetTabCallback(MakeDelegate(this, &LoginScreen::TabUser));

	stringPos.y += 40.f;
	pPassword = StringBox::Create(pFont, &stringPos);
	pPassword->SetTabCallback(MakeDelegate(this, &LoginScreen::TabPass));

	stringPos.y += 40.f;
	pEmail = StringBox::Create(pFont, &stringPos);
	pEmail->SetTabCallback(MakeDelegate(this, &LoginScreen::TabEmail));

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pLogin = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, false);
	pLogin->SetClickCallback(MakeDelegate(this, &LoginScreen::Click));

	// offline button
	pos.x = 64.f + 96.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, 1, false);
	pOffline->SetClickCallback(MakeDelegate(this, &LoginScreen::Click));

	// new account button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pNew = Button::Create(pIcons, &pos, &uvs, MFVector::one, 2, false);
	pNew->SetClickCallback(MakeDelegate(this, &LoginScreen::Click));

	// create account button
	pos.x = 64.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pCreate = Button::Create(pIcons, &pos, &uvs, MFVector::one, 3, false);
	pCreate->SetClickCallback(MakeDelegate(this, &LoginScreen::Click));

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, 4, false);
	pReturn->SetClickCallback(MakeDelegate(this, &LoginScreen::Click));
}

LoginScreen::~LoginScreen()
{
	pUsername->Destroy();
	pPassword->Destroy();
	pEmail->Destroy();

	pLogin->Destroy();
	pOffline->Destroy();
	pNew->Destroy();
	pCreate->Destroy();
	pReturn->Destroy();

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
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

void LoginScreen::Click(int button, int buttonID)
{
	switch(buttonID)
	{
		case 0:
		{
			// try and login...
			Session *pSession = new Session();
			pSession->SetLoginDelegate(MakeDelegate(this, &LoginScreen::OnLogin));
			pSession->Login(pUsername->GetString(), pPassword->GetString());
			Session::SetCurrent(pSession);
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
			state = 1;
			pInputManager->PopReceiver(this);
			pInputManager->PushReceiver(this);
			pInputManager->PushReceiver(pUsername);
			pInputManager->PushReceiver(pPassword);
			pInputManager->PushReceiver(pEmail);
			pInputManager->PushReceiver(pCreate);
			pInputManager->PushReceiver(pReturn);
			pPrompt = "Create Account:";
			pMessage = NULL;
			break;
		}

		case 3:
		{
			const char *pUsername = this->pUsername->GetString();
			const char *pPassword = this->pPassword->GetString();
			const char *pEmail = this->pEmail->GetString();

			// create the account
			WLServ_CreateAccount(createRequest, pUsername, pPassword, pEmail);
			break;
		}

		case 4:
		{
			state = 0;
			pInputManager->PopReceiver(this);
			pInputManager->PushReceiver(this);
			pInputManager->PushReceiver(pUsername);
			pInputManager->PushReceiver(pPassword);
			pInputManager->PushReceiver(pLogin);
			pInputManager->PushReceiver(pOffline);
			pInputManager->PushReceiver(pNew);
			pPrompt = "Login:";
			pMessage = NULL;
			break;
		}
	}
}

void LoginScreen::TabUser(const char *pString)
{
	pPassword->Enable(true);
}

void LoginScreen::TabPass(const char *pString)
{
	if(state == 0)
		pUsername->Enable(true);
	else if(state == 1)
		pEmail->Enable(true);
}

void LoginScreen::TabEmail(const char *pString)
{
	pUsername->Enable(true);
}

void LoginScreen::CreateComplete(HTTPRequest::Status status)
{
	uint32 user;
	ServerError err = WLServResult_GetUser(createRequest, &user);
	if(err != SE_NO_ERROR)
	{
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

	// try and login...
	Session *pSession = new Session();
	pSession->SetLoginDelegate(MakeDelegate(this, &LoginScreen::OnLogin));
	pSession->Login(pUsername->GetString(), pPassword->GetString());
	Session::SetCurrent(pSession);
}

void LoginScreen::AutoLogin()
{
	// try and login...
	Session *pSession = new Session();
	pSession->SetLoginDelegate(MakeDelegate(this, &LoginScreen::OnLogin));
	pSession->Login("TurkeyMan", "terceS");
	Session::SetCurrent(pSession);
}

void LoginScreen::OnLogin(ServerError err, Session *pSession)
{
	if(err != SE_NO_ERROR)
	{
		Session::SetCurrent(NULL);
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

	pMessage = NULL;

	// and continue
	Screen::SetNext(pHome);
}
