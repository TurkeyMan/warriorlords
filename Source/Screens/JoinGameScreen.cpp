#include "Warlords.h"
#include "Display.h"
#include "JoinGameScreen.h"
#include "HomeScreen.h"
#include "LobbyScreen.h"

#include "MFMaterial.h"
#include "MFRenderer.h"

extern HomeScreen *pHome;
extern LobbyScreen *pLobby;

JoinGameScreen::JoinGameScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	find.SetCompleteDelegate(MakeDelegate(this, &JoinGameScreen::FindGame));
	join.SetCompleteDelegate(MakeDelegate(this, &JoinGameScreen::JoinGame));

	pMessage = NULL;

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	MFRect stringPos = { 140, 80, 256, MFFont_GetFontHeight(pFont) };
	pGame = StringBox::Create(pFont, &stringPos);

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pJoin = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, false);
	pJoin->SetClickCallback(MakeDelegate(this, &JoinGameScreen::Click));

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, 1, false);
	pReturn->SetClickCallback(MakeDelegate(this, &JoinGameScreen::Click));
}

JoinGameScreen::~JoinGameScreen()
{
	pGame->Destroy();
	pJoin->Destroy();
	pReturn->Destroy();

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void JoinGameScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pJoin);
	pInputManager->PushReceiver(pReturn);
	pInputManager->PushReceiver(pGame);

	pMessage = NULL;
}

bool JoinGameScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
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

int JoinGameScreen::Update()
{
	pGame->Update();

	return 0;
}

void JoinGameScreen::Draw()
{
	MFFont_DrawText(pFont, MakeVector(32, 32), 32.f, MFVector::yellow, "Join a game...");

	pGame->Draw();
	pJoin->Draw();
	pReturn->Draw();

	if(pMessage)
		MFFont_DrawText(pFont, MakeVector(160, 220), 32.f, MFVector::red, pMessage);
}

void JoinGameScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void JoinGameScreen::Click(int button, int buttonID)
{
	switch(buttonID)
	{
		case 0:
		{
			WLServ_GetGameByName(find, pGame->GetString());
			break;
		}

		case 1:
		{
			Screen::SetNext(pHome);
			break;
		}
	}
}

void JoinGameScreen::FindGame(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetGameDetails(find, &details);

	if(err != SE_NO_ERROR)
	{
		pMessage = "Invalid game!";
		return;
	}

	Session *pCurrent = Session::GetCurrent();
	if(!pCurrent)
	{
		pMessage = "Invalid session!";
		return;
	}

	WLServ_JoinGame(join, pCurrent->GetUserID(), details.id);
}

void JoinGameScreen::JoinGame(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetError(join);

	if(err == SE_NO_ERROR)
	{
		// enter lobby
		pLobby->ShowOnline(details.id);
	}
	else
	{
		// couldn't join game
		pMessage = "Couldn't join game!";
	}
}
