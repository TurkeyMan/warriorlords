#include "Warlords.h"
#include "Display.h"
#include "HomeScreen.h"
#include "Game.h"
#include "Editor.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern Game *pGame;
extern Editor *pEditor;

HomeScreen::HomeScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	// populate map list
	MFRect rect = { 16.f, 16.f, 256.f, 128.f };
	pMyTurn = ListBox::Create(&rect, pFont);

	rect.y += 128.f;
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
	pFind = Button::Create(pIcons, &pos, &uvs, MFVector::one, FindGame, this, 1, false);

	// find game button
	pos.x = 48.f + 128.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pFind = Button::Create(pIcons, &pos, &uvs, MFVector::one, FindGame, this, 1, false);

	// offline game button
	pos.x = 64.f + 192.f;
	uvs.x = 0.f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, OfflineGame, this, 1, false);

	// continue game button
	pos.x = 80.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pContinue = Button::Create(pIcons, &pos, &uvs, MFVector::one, ContinueGame, this, 1, false);
}

HomeScreen::~HomeScreen()
{
	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void HomeScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pMyTurn);
	pInputManager->PushReceiver(pMyGames);
	pInputManager->PushReceiver(pJoin);
	pInputManager->PushReceiver(pFind);
	pInputManager->PushReceiver(pOffline);
	pInputManager->PushReceiver(pContinue);
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

}

void HomeScreen::JoinGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}

void HomeScreen::FindGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}

void HomeScreen::OfflineGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}

void HomeScreen::ContinueGame(int button, void *pUserData, int buttonID)
{
	HomeScreen *pScreen = (HomeScreen*)pUserData;

}
