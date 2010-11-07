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
			GameDetails details;
			ServerError err = WLServ_GetGameByName(pGame->GetString(), &details);

			if(err == SE_NO_ERROR)
			{
				Session *pCurrent = Session::GetCurrent();
				if(pCurrent)
				{
					err = WLServ_JoinGame(pCurrent->GetUserID(), details.id);
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
				else
				{
					pMessage = "Invalid session!";
				}
			}
			else
			{
				pMessage = "Invalid game!";
			}
			break;
		}

		case 1:
		{
			Screen::SetNext(pHome);
			break;
		}
	}
}
