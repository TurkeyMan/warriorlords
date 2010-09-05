#include "Warlords.h"
#include "Display.h"
#include "LobbyScreen.h"
#include "HomeScreen.h"

#include "MFMaterial.h"
#include "MFRenderer.h"

extern HomeScreen *pHome;
extern Game *pGame;

LobbyScreen::LobbyScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	bOffline = false;

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	// login button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 64.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pBegin = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 0, false);

	// leave button
	pos.x = 64.f + 96.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pLeave = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 1, false);

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, Click, this, 2, false);

	// create the select boxes
	MFRect rect = { 10.f, 10.f, 100.f, 20.f };
	for(int a=0; a<8; ++a)
	{
//		rect.x = (float)(a & 1) * 240.f + 100.f;
//		rect.y = (float)(a >> 1) * 30.f + 10.f;
		rect.y = (float)a * 30.f + 10.f;

		rect.x = 300.f;
		rect.width = 90.f;
		pRaces[a] = SelectBox::Create(&rect, pFont);
		pRaces[a]->SetSelectCallback(SetRace, this, a);

		rect.x += 100.f;
		rect.width = 30.f;
		pColours[a] = SelectBox::Create(&rect, pFont);
		pColours[a]->SetSelectCallback(SetColour, this, a);
	}
}

LobbyScreen::~LobbyScreen()
{
	pBegin->Destroy();
	pLeave->Destroy();
	pReturn->Destroy();

	for(int a=0; a<8; ++a)
	{
		pRaces[a]->Destroy();
		pColours[a]->Destroy();
	}

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void LobbyScreen::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pReturn);

	if(!bOffline)
		pInputManager->PushReceiver(pLeave);

	// only push the start game button if we created the game
	Session *pSession = Session::GetCurrent();
	if(bOffline || (pSession && pSession->GetUserID() == details.players[0].id))
		pInputManager->PushReceiver(pBegin);

	for(int a=0; a<details.maxPlayers; ++a)
	{
		pRaces[a]->Clear();
		pColours[a]->Clear();

		for(int b=1; b<map.unitSetDetails.numRaces; ++b)
			pRaces[a]->AddItem(map.unitSetDetails.races[b]);
		pRaces[a]->SetSelection(details.players[a].race - 1);

		for(int b=1; b<map.unitSetDetails.numRaces; ++b)
		{
			MFVector colour;
			colour.FromPackedColour(map.unitSetDetails.colours[b]);
			pColours[a]->AddItem("X", -1, NULL, colour);
		}
		pColours[a]->SetSelection(details.players[a].colour - 1);

		if(bOffline || (pSession && details.players[a].id == pSession->GetUserID()))
		{
			pInputManager->PushReceiver(pRaces[a]);
			pInputManager->PushReceiver(pColours[a]);
		}
	}
}

bool LobbyScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
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

bool LobbyScreen::ShowOnline(uint32 lobbyID)
{
	bOffline = false;

	MFZeroMemory(&details, sizeof(details));

	ServerError err = WLServ_GetGameByID(lobbyID, &details);
	if(err != SE_NO_ERROR)
		return false;

	Map::GetMapDetails(details.map, &map);

	Screen::SetNext(this);
	return true;
}

void LobbyScreen::ShowOffline(const MapDetails *pMap)
{
	bOffline = true;

	MFZeroMemory(&details, sizeof(details));
	MFString_Copy(details.map, pMap->filename);
	details.maxPlayers = pMap->numPlayers;
	details.turnTime = 60*60;

	details.numPlayers = details.maxPlayers;
	for(int a=0; a<details.numPlayers; ++a)
	{
		details.players[a].race = 1;
		details.players[a].colour = 1 + a;
	}

	Map::GetMapDetails(pMap->filename, &map);

	Screen::SetNext(this);
}

void LobbyScreen::UpdateLobbyState()
{
	if(bOffline)
		return;

	ServerError err = WLServ_GetGameByID(details.id, &details);
	//...
}

int LobbyScreen::Update()
{

	return 0;
}

void LobbyScreen::Draw()
{
	Session *pSession = Session::GetCurrent();
	pReturn->Draw();

	if(!bOffline)
		pLeave->Draw();

	// if we are the creator, render the begin game button
	if(bOffline || (pSession && pSession->GetUserID() == details.players[0].id))
		pBegin->Draw();

	int a = 0;
	for(; a<details.numPlayers; ++a)
	{
		if(!bOffline)
			MFFont_BlitText(pFont, 10, a*30 + 10, MFVector::white, details.players[a].name);
		else
			MFFont_BlitTextf(pFont, 10, a*30 + 10, MFVector::white, "Player %d", a);

		if(bOffline || (pSession && details.players[a].id == pSession->GetUserID()))
		{
			pRaces[a]->Draw();
			pColours[a]->Draw();
		}
		else
		{
			const char *pRace = map.unitSetDetails.races[details.players[a].race];
			MFFont_BlitText(pFont, 310, a*30 + 10, MFVector::white, pRace);

			MFVector colour;
			colour.FromPackedColour(map.unitSetDetails.colours[details.players[a].colour]);
			MFFont_BlitText(pFont, 410, a*30 + 10, colour, "X");
		}
	}
	for(; a<details.maxPlayers; ++a)
	{
		MFFont_BlitText(pFont, 10, a*30 + 10, MFVector::white, "Waiting...");
	}
}

void LobbyScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void LobbyScreen::Click(int button, void *pUserData, int buttonID)
{
	LobbyScreen *pScreen = (LobbyScreen*)pUserData;

	switch(buttonID)
	{
		case 0:
		{
			// begin game...

			// setup game parameters
			GameParams params;
			MFZeroMemory(&params, sizeof(params));
			params.pMap = pScreen->details.map;
			params.bOnline = !pScreen->bOffline;

			// set up the players
			bool bAssigned[16];
			MFZeroMemory(bAssigned, sizeof(bAssigned));
			params.numPlayers = pScreen->details.numPlayers;
			for(int a=0; a<params.numPlayers; ++a)
			{
				// select a random starting position for the player
				int p = MFRand() % params.numPlayers;
				while(bAssigned[p])
					p = MFRand() % params.numPlayers;
				bAssigned[p] = true;

				// assign the player details
				params.players[p].id = pScreen->details.players[a].id;
				params.players[p].race = pScreen->details.players[a].race;
				params.players[p].colour = pScreen->details.players[a].colour;
			}

			// create the game
			if(!pScreen->bOffline)
			{
				uint32 players[16];
				for(int a=0; a<params.numPlayers; ++a)
					players[a] = params.players[a].id;

				ServerError err = WLServ_BeginGame(pScreen->details.id, players, params.numPlayers, &params.gameID);

				if(err != SE_NO_ERROR)
				{
					// set some error message?
					break;
				}
			}

			// start game
			pGame = new Game(&params);
			Game::SetCurrent(pGame);
			pGame->BeginGame();
			break;
		}
		case 1:
		{
			// leave the game
			Session *pSession = Session::GetCurrent();
			if(pSession)
				WLServ_LeaveGame(pSession->GetUserID(), pScreen->details.id);
		}
		case 2:
		{
			// return to the home screen
			Screen::SetNext(pHome);
			break;
		}
	}
}

void LobbyScreen::SetRace(int item, void *pUserData, int id)
{
	LobbyScreen *pThis = (LobbyScreen*)pUserData;

	++item;
	ServerError err = SE_NO_ERROR;
	
	if(!pThis->bOffline)
		err = WLServ_SetRace(pThis->details.id, pThis->details.players[id].id, item);

	if(err == SE_NO_ERROR)
		pThis->details.players[id].race = item;
	else
		pThis->pRaces[id]->SetSelection(pThis->details.players[id].race - 1);
}

void LobbyScreen::SetColour(int item, void *pUserData, int id)
{
	LobbyScreen *pThis = (LobbyScreen*)pUserData;

	++item;
	ServerError err = SE_NO_ERROR;

	if(!pThis->bOffline)
		err = WLServ_SetColour(pThis->details.id, pThis->details.players[id].id, item);

	for(int a=0; a<pThis->details.numPlayers; ++a)
	{
		if(item == pThis->details.players[a].colour)
		{
			err = SE_ALREADY_PRESENT;
			break;
		}
	}

	if(err == SE_NO_ERROR)
		pThis->details.players[id].colour = item;
	else
		pThis->pColours[id]->SetSelection(pThis->details.players[id].colour - 1);
}
