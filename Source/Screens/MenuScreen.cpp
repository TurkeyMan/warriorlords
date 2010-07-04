#include "Warlords.h"
#include "Display.h"
#include "MenuScreen.h"
#include "HomeScreen.h"
#include "LobbyScreen.h"
#include "Game.h"
#include "Editor.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern HomeScreen *pHome;
extern LobbyScreen *pLobby;

extern Game *pGame;
extern Editor *pEditor;

MenuScreen::MenuScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	gameType = 0;
	pMaps = NULL;
	pMessage = NULL;

	// populate map list
	MFRect rect = { 10.f, 64.f, 240.f, 200.f };
	pMapList = ListBox::Create(&rect, pFont);
	pMapList->SetSelectCallback(SelectMap, this);

	MFFindData fd;
	MFFind *pFind = MFFileSystem_FindFirst("game:Map*.ini", &fd);
	while(pFind)
	{
		// allocate map details
		MapData *pDetails = (MapData*)MFHeap_Alloc(sizeof(MapData));

		// copy the name and truncate extension
		MFString_Copy(pDetails->name, fd.pFilename);
		char *pExt = MFString_RChr(pDetails->name, '.');
		*pExt = 0;

		pDetails->bDetailsLoaded = false;

		// add item to the list box
		pMapList->AddItem(pDetails->name, -1, pDetails);

		// hook it up to map list
		pDetails->pNext = pMaps;
		pMaps = pDetails;

		if(!MFFileSystem_FindNext(pFind, &fd))
		{
			MFFileSystem_FindClose(pFind);
			pFind = NULL;
		}
	}

	// create the game name textbox
	MFRect stringPos = { 340, 70, 160, MFFont_GetFontHeight(pFont) };
	pGameName = StringBox::Create(pFont, &stringPos, NULL, NULL);

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
	pStart = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 0, false);

	// edit map button
	pos.x = 32.f + 64.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pEdit = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 1, false);

	// return button
	pos.x = 64.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 2, false);
}

MenuScreen::~MenuScreen()
{
	while(pMaps)
	{
		MapData *pNext = pMaps->pNext;
		MFHeap_Free(pNext);
		pMaps = pNext;
	}

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void MenuScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pMapList);
	pInputManager->PushReceiver(pStart);
	pInputManager->PushReceiver(pReturn);

	pMapList->SetSelection(-1);
	pStart->Enable(false);

	if(gameType == 0)
	{
		pInputManager->PushReceiver(pGameName);
	}
	else
	{
		pInputManager->PushReceiver(pEdit);
	}

	pMessage = NULL;
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
	if(gameType == 0)
		pGameName->Update();

	return 0;
}

void MenuScreen::Draw()
{
	if(gameType == 0)
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Create Online Game");
	else
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "New Offline Game");

	if(pMessage)
		MFFont_DrawText(pFont, MakeVector(270, 16), 32.f, MFVector::red, pMessage);

	pMapList->Draw();

	if(gameType == 0)
	{
		MFFont_DrawText(pFont, MakeVector(270, 66), 30.f, MFVector::white, "Game:");
		pGameName->Draw();
	}

	pStart->Draw();
	pReturn->Draw();

	if(gameType == 1)
		pEdit->Draw();

	int selection = pMapList->GetSelection();
	if(selection != -1)
	{
		MapData *pMap = (MapData*)pMapList->GetItemData(selection);

		float offset = 0.f;
		if(gameType == 0)
			offset += 32;

		MFFont_DrawText(pFont, MakeVector(270, 70 + offset), 30.f, MFVector::white, pMap->details.mapName);
		MFFont_DrawTextf(pFont, MakeVector(280, 100 + offset), 25.f, MFVector::white, "Players: %d", pMap->details.numPlayers);
		MFFont_DrawTextf(pFont, MakeVector(280, 125 + offset), 25.f, MFVector::white, "Size: %d, %d", pMap->details.width, pMap->details.height);
	}
}

void MenuScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void MenuScreen::Click(int button, void *pUserData, int buttonID)
{
	MenuScreen *pMenu = (MenuScreen*)pUserData;

	switch(buttonID)
	{
		case 0:
		case 1:
		{
			int selection = pMenu->pMapList->GetSelection();
			if(selection == -1)
				return;

			MapData *pMap = (MapData*)pMenu->pMapList->GetItemData(selection);

			// create game
			if(buttonID == 0)
			{
				if(pMenu->gameType == 0)
				{
					Session *pSession = Session::GetCurrent();
					if(!pSession)
					{
						pMenu->pMessage = "Not logged in!";
						break;
					}

					const char *pGameName = pMenu->pGameName->GetString();

					if(pGameName[0] == 0)
					{
						pMenu->pMessage = "Enter a game name!";
						break;
					}


					GameCreateDetails details;
					details.pName = pGameName;
					details.pMap = pMap->name;
					details.turnTime = 60*60;
					details.numPlayers = pMap->details.numPlayers;

					uint32 lobby;
					ServerError err = WLServ_CreateGame(pSession->GetUserID(), &details, &lobby);

					if(err != SE_NO_ERROR)
					{
						switch(err)
						{
							case SE_CONNECTION_FAILED:
								pMenu->pMessage = "Couldn't connect to server!";
								break;
							case SE_INVALID_GAME:
								pMenu->pMessage = "Invalid game!";
								break;
							default:
								pMenu->pMessage = "Unknown Error!";
								break;
						}
					}
					else
					{
						pLobby->ShowOnline(lobby);
					}
				}
				else
				{
					pLobby->ShowOffline(&pMap->details);
				}
			}
			else
			{
				// edit map
				GameParams params;
				MFZeroMemory(&params, sizeof(params));
				params.pMap = pMap->name;
				params.bOnline = false;
				params.bEditMap = true;

				pGame = new Game(&params);
				Game::SetCurrent(pGame);

				pEditor = new Editor(pGame);
				Screen::SetNext(pEditor);
			}
			break;
		}

		case 2:
			// return to the home screen
			Screen::SetNext(pHome);
			break;
	}
}

void MenuScreen::SelectMap(int item, void *pUserData)
{
	MenuScreen *pScreen = (MenuScreen*)pUserData;

	pScreen->pStart->Enable(item >= 0);

	if(item < 0)
		return;

	MapData *pMap = (MapData*)pScreen->pMapList->GetItemData(item);
	if(!pMap->bDetailsLoaded)
	{
		Map::GetMapDetails(pMap->name, &pMap->details);
		pMap->bDetailsLoaded = true;
	}
}
