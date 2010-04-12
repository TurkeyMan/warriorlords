#include "Warlords.h"
#include "Display.h"
#include "MenuScreen.h"
#include "Game.h"
#include "Editor.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern Game *pGame;
extern Editor *pEditor;

MenuScreen::MenuScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	// populate map list
	MFRect rect = { 16.f, 16.f, 256.f, 256.f };
	pListBox = ListBox::Create(&rect, pFont);

	MFFindData fd;
	MFFind *pFind = MFFileSystem_FindFirst("game:Map*.ini", &fd);
	while(pFind)
	{
		char name[256];
		MFString_Copy(name, fd.pFilename);
		char *pExt = MFString_RChr(name, '.');
		*pExt = 0;

		pListBox->AddItem(name);

		if(!MFFileSystem_FindNext(pFind, &fd))
		{
			MFFileSystem_FindClose(pFind);
			pFind = NULL;
		}
	}

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	// start game button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 16.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pStart = Button::Create(pIcons, &pos, &uvs, MFVector::one, StartGame, this, 0, false);

	// edit map button
	pos.x = 32.f + 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pEdit = Button::Create(pIcons, &pos, &uvs, MFVector::one, StartGame, this, 1, false);
}

MenuScreen::~MenuScreen()
{
	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void MenuScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pListBox);
	pInputManager->PushReceiver(pStart);
	pInputManager->PushReceiver(pEdit);
}

bool MenuScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
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

int MenuScreen::Update()
{

	return 0;
}

void MenuScreen::Draw()
{
	pListBox->Draw();

	pStart->Draw();
	pEdit->Draw();
}

void MenuScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void MenuScreen::StartGame(int button, void *pUserData, int buttonID)
{
	MenuScreen *pMenu = (MenuScreen*)pUserData;

	int selection = pMenu->pListBox->GetSelection();
	if(selection == -1)
		return;

	const char *pMap = pMenu->pListBox->GetItemText(selection);

	if(buttonID == 0)
	{
		pGame = new Game(pMap);
		Game::SetCurrent(pGame);
		pGame->BeginGame();
	}
	else
	{
		pGame = new Game(pMap, true);
		Game::SetCurrent(pGame);

		pEditor = new Editor(pGame);
		Screen::SetNext(pEditor);
	}
}
