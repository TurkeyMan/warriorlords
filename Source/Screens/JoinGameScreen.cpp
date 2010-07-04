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
	pGame = StringBox::Create(pFont, &stringPos, NULL, NULL);

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pJoin = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 0, false);

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 1, false);
}

JoinGameScreen::~JoinGameScreen()
{
	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);

	pGame->Destroy();
	pJoin->Destroy();
	pReturn->Destroy();
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

void JoinGameScreen::Click(int button, void *pUserData, int buttonID)
{
	JoinGameScreen *pScreen = (JoinGameScreen*)pUserData;

	switch(buttonID)
	{
		case 0:
		{
			GameDetails details;
			ServerError err = WLServ_GetGameByName(pScreen->pGame->GetString(), &details);

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
						pScreen->pMessage = "Couldn't join game!";
					}
				}
				else
				{
					pScreen->pMessage = "Invalid session!";
				}
			}
			else
			{
				pScreen->pMessage = "Invalid game!";
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
