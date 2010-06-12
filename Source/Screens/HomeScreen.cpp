#include "Warlords.h"
#include "Display.h"
#include "HomeScreen.h"
#include "MenuScreen.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern MenuScreen *pMenu;

HomeScreen::HomeScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	pSession = NULL;
	bOffline = true;

	// populate map list
	MFRect rect = { 10.f, 64.f, 240.f, 200.f };
	pMyTurn = ListBox::Create(&rect, pFont);

	rect.x += 260.f;
	pMyGames = ListBox::Create(&rect, pFont);

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	// create game button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 16.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pCreate = Button::Create(pIcons, &pos, &uvs, MFVector::one, CreateGame, this, 0, false);

	// join game button
	pos.x = 32.f + 64.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pJoin = Button::Create(pIcons, &pos, &uvs, MFVector::one, FindGame, this, 2, false);

	// find game button
	pos.x = 48.f + 128.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pFind = Button::Create(pIcons, &pos, &uvs, MFVector::one, FindGame, this, 3, false);

	// offline game button
	pos.x = 64.f + 192.f;
	uvs.x = 0.f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, CreateGame, this, 1, false);

	// continue game button
	pos.x = 80.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pContinue = Button::Create(pIcons, &pos, &uvs, MFVector::one, ContinueGame, this, 4, false);
}

HomeScreen::~HomeScreen()
{
	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void HomeScreen::Select()
{
	pSession = Session::GetCurrent();
	bOffline = pSession == NULL || pSession->IsOffline();

	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pMyTurn);
	pInputManager->PushReceiver(pMyGames);
	pInputManager->PushReceiver(pCreate);
	pInputManager->PushReceiver(pJoin);
	pInputManager->PushReceiver(pFind);
	pInputManager->PushReceiver(pOffline);
	pInputManager->PushReceiver(pContinue);

	pCreate->Enable(!bOffline);
	pJoin->Enable(!bOffline);
	pFind->Enable(!bOffline);

	pMyTurn->SetSelection(-1);
	pMyGames->SetSelection(-1);
	pContinue->Enable(false);
}

bool HomeScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
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

int HomeScreen::Update()
{

	return 0;
}

void HomeScreen::Draw()
{
	if(bOffline)
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Offline");
	else
		MFFont_DrawTextf(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Welcome %s!", pSession->GetUsername());

	pMyTurn->Draw();
	pMyGames->Draw();
	pCreate->Draw();
	pJoin->Draw();
	pFind->Draw();
	pOffline->Draw();
	pContinue->Draw();
}

void HomeScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void HomeScreen::CreateGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

	pMenu->SetGameType(buttonID);

	// go to game create screen
	Screen::SetNext(pMenu);
}

void HomeScreen::JoinGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}

void HomeScreen::FindGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}

void HomeScreen::ContinueGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}
