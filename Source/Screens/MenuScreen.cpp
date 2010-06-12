#include "Warlords.h"
#include "Display.h"
#include "MenuScreen.h"
#include "HomeScreen.h"
#include "Game.h"
#include "Editor.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern HomeScreen *pHome;

extern Game *pGame;
extern Editor *pEditor;

MenuScreen::MenuScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	gameType = 0;
	pMaps = NULL;

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

	if(gameType == 0)
	{
		// we can't start internet games yet :(
		pStart->Enable(false);
	}
	else
	{
		pStart->Enable(true);
		pInputManager->PushReceiver(pEdit);
	}
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
	if(gameType == 0)
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Create Online Game");
	else
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "New Offline Game");

	pMapList->Draw();

	pStart->Draw();
	pReturn->Draw();

	if(gameType == 1)
		pEdit->Draw();

	int selection = pMapList->GetSelection();
	if(selection != -1)
	{
		MapData *pMap = (MapData*)pMapList->GetItemData(selection);

		MFFont_DrawText(pFont, MakeVector(270, 70), 30.f, MFVector::white, pMap->details.mapName);
		MFFont_DrawTextf(pFont, MakeVector(280, 100), 25.f, MFVector::white, "Players: %d", pMap->details.numPlayers);
		MFFont_DrawTextf(pFont, MakeVector(280, 125), 25.f, MFVector::white, "Size: %d, %d", pMap->details.width, pMap->details.height);
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

			// setup game parameters
			GameParams params;
			MFZeroMemory(&params, sizeof(params));
			params.pMap = pMap->name;
			params.bOnline = buttonID == 0;

			// set up the races...
			bool bColoursTaken[16];
			MFZeroMemory(bColoursTaken, sizeof(bColoursTaken));
			int numColours = pMap->details.unitSetDetails.numRaces - 1;

			params.numPlayers = pMap->details.numPlayers;
			for(int a=0; a<params.numPlayers; ++a)
			{
				// choose random race
				params.playerRaces[a] = 1 + (MFRand() % pMap->details.unitSetDetails.numRaces);
				while(!pMap->details.bRacePresent[params.playerRaces[a]])
					params.playerRaces[a] = 1 + (MFRand() % pMap->details.unitSetDetails.numRaces);

				// choose random colour
				int colour;
				do colour = 1 + (MFRand() % numColours);
				while(bColoursTaken[colour-1]);
				bColoursTaken[colour-1] = true;

				params.playerColours[a] = colour;
			}

			// create game
			if(buttonID == 0)
			{
				// start game
				pGame = new Game(&params);
				Game::SetCurrent(pGame);
				pGame->BeginGame();
			}
			else
			{
				params.bEditMap = true;

				// edit map
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
	if(item < 0)
		return;

	MenuScreen *pScreen = (MenuScreen*)pUserData;

	MapData *pMap = (MapData*)pScreen->pMapList->GetItemData(item);
	if(!pMap->bDetailsLoaded)
	{
		Map::GetMapDetails(pMap->name, &pMap->details);
		pMap->bDetailsLoaded = true;
	}
}
