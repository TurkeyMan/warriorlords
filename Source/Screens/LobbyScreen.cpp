#include "Warlords.h"
#include "Display.h"
#include "LobbyScreen.h"
#include "HomeScreen.h"

#include "MFSystem.h"
#include "MFMaterial.h"
#include "MFRenderer.h"

extern HomeScreen *pHome;
extern Game *pGame;

LobbyScreen::LobbyScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	request.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::GetDetails));
	begin.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::BeginGame));
	enter.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::EnterGame));
	race.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::CommitRace));
	colour.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::CommitColour));
	hero.SetCompleteDelegate(MakeDelegate(this, &LobbyScreen::CommitHero));

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
	pBegin = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, false);
	pBegin->SetClickCallback(MakeDelegate(this, &LobbyScreen::Click));

	// leave button
	pos.x = 64.f + 96.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pLeave = Button::Create(pIcons, &pos, &uvs, MFVector::one, 1, false);
	pLeave->SetClickCallback(MakeDelegate(this, &LobbyScreen::Click));

	// return button
	pos.x = 64.f + 96.f + 128.f;
	uvs.x = 0.0f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pReturn = Button::Create(pIcons, &pos, &uvs, MFVector::one, 2, false);
	pReturn->SetClickCallback(MakeDelegate(this, &LobbyScreen::Click));

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
		pRaces[a]->SetSelectCallback(MakeDelegate(this, &LobbyScreen::SetRace), a);

		rect.x += 100.f;
		rect.width = 30.f;
		pColours[a] = SelectBox::Create(&rect, pFont);
		pColours[a]->SetSelectCallback(MakeDelegate(this, &LobbyScreen::SetColour), a);

		rect.x += 40.f;
		rect.width = 160.f;
		pHeroes[a] = SelectBox::Create(&rect, pFont);
		pHeroes[a]->SetSelectCallback(MakeDelegate(this, &LobbyScreen::SetHero), a);
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
		pHeroes[a]->Destroy();
	}

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void LobbyScreen::Select()
{
	// reset hero selection
	for(int a=0; a<details.maxPlayers; ++a)
		details.players[a].hero = 0;

	// refresh the lobby screen
	Refresh();
}

void LobbyScreen::Refresh()
{
	pInputManager->PopReceiver(this);
	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pReturn);

	if(!bOffline)
		pInputManager->PushReceiver(pLeave);

	// only push the start game button if we created the game
	Session *pSession = Session::Get();
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

		// populate the box with heroes
		SetHeroes(a);

		if(bOffline || (pSession && details.players[a].id == pSession->GetUserID()))
		{
			pInputManager->PushReceiver(pRaces[a]);
			pInputManager->PushReceiver(pColours[a]);
			pInputManager->PushReceiver(pHeroes[a]);
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

void LobbyScreen::GetDetails(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetGameDetails(request, &details);

	if(Screen::GetCurrent() != this)
	{
		if(err != SE_NO_ERROR)
			return;

		// load the map details and show the lobby screen
		Map::GetMapDetails(details.map, &map);
		Screen::SetNext(this);
	}
	else
	{
		if(err != SE_NO_ERROR)
		{
			// the game may have been started...
			WLServ_GameState(enter, details.id);
			return;
		}

		// update the lobby state
		Refresh();
	}
}

bool LobbyScreen::ShowOnline(uint32 lobbyID)
{
	bOffline = false;

	MFZeroMemory(&details, sizeof(details));

	WLServ_GetGameByID(request, lobbyID);

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
		details.players[a].hero = 0;
	}

	Map::GetMapDetails(pMap->filename, &map);

	Screen::SetNext(this);
}

void LobbyScreen::UpdateLobbyState()
{
	if(bOffline)
		return;

	static float timeout = 10.f;
	timeout -= MFSystem_TimeDelta();
	if(timeout <= 0.f)
	{
		if(!request.RequestPending())
			WLServ_GetGameByID(request, details.id);
		timeout = 10.f;
	}
}

int LobbyScreen::Update()
{
	UpdateLobbyState();

	return 0;
}

void LobbyScreen::Draw()
{
	Session *pSession = Session::Get();
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
			pHeroes[a]->Draw();
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

void LobbyScreen::Click(int button, int buttonID)
{
	switch(buttonID)
	{
		case 0:
		{
			// begin game...

			// setup game parameters
			MFZeroMemory(&params, sizeof(params));
			params.pMap = details.map;
			params.bOnline = !bOffline;

			// set up the players
			bool bAssigned[16];
			MFZeroMemory(bAssigned, sizeof(bAssigned));
			params.numPlayers = details.numPlayers;
			for(int a=0; a<params.numPlayers; ++a)
			{
				// select a random starting position for the player
				int p = MFRand() % params.numPlayers;
				while(bAssigned[p])
					p = MFRand() % params.numPlayers;
				bAssigned[p] = true;

				// assign the player details
				params.players[p].id = details.players[a].id;
				params.players[p].race = details.players[a].race;
				params.players[p].colour = details.players[a].colour;
				params.players[p].hero = details.players[a].hero;
			}

			if(bOffline)
			{
				// start game
				pGame = new Game(&params);
				Game::SetCurrent(pGame);
				pGame->BeginGame();
				break;
			}

			// create the game
			uint32 players[16];
			for(int a=0; a<params.numPlayers; ++a)
				players[a] = params.players[a].id;

			WLServ_BeginGame(begin, details.id, players, params.numPlayers);
			break;
		}
		case 1:
		{
			// leave the game
			Session *pSession = Session::Get();
			if(pSession)
				WLServ_LeaveGame(leave, pSession->GetUserID(), details.id);
		}
		case 2:
		{
			// return to the home screen
			Screen::SetNext(pHome);
			break;
		}
	}
}

void LobbyScreen::BeginGame(HTTPRequest::Status status)
{
	WLServResult_GetGame(begin, &params.gameID);

	// start game
	pGame = new Game(&params);
	Game::SetCurrent(pGame);
	pGame->BeginGame();
}

void LobbyScreen::EnterGame(HTTPRequest::Status status)
{
	GameState state;
	ServerError err = WLServResult_GetGameState(enter, &state);

	if(err == SE_NO_ERROR)
	{
		// enter game
		pGame = new Game(&state);
		Game::SetCurrent(pGame);
		return;
	}

	// the host must have left the game
	//... display a dialog box?

	Screen::SetNext(pHome);
}

void LobbyScreen::SetHeroes(int player)
{
	// populate the box with heroes
	pHeroes[player]->Clear();

	for(int b=0; b<map.unitSetDetails.numUnits; ++b)
	{
		if(map.unitSetDetails.units[b].race == details.players[player].race && map.unitSetDetails.units[b].type == UT_Hero)
			pHeroes[player]->AddItem(map.unitSetDetails.units[b].name);
	}

	pHeroes[player]->SetSelection(details.players[player].hero);
}

void LobbyScreen::SetRace(int item, int id)
{
	++item;
	ServerError err = SE_NO_ERROR;

	raceID = id;
	newRace = item;

	if(!bOffline)
	{
		WLServ_SetRace(race, details.id, details.players[id].id, item);
	}
	else
	{
		details.players[id].race = item;
		SetHeroes(id);
	}
}

void LobbyScreen::CommitRace(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetError(race);

	if(err == SE_NO_ERROR)
	{
		details.players[raceID].race = newRace;
		SetHeroes(raceID);
	}
	else
	{
		pRaces[raceID]->SetSelection(details.players[raceID].race - 1);
	}
}

void LobbyScreen::SetColour(int item, int id)
{
	++item;

	for(int a=0; a<details.numPlayers; ++a)
	{
		if(item == details.players[a].colour)
			return;
	}

	colourID = id;
	newColour = item;

	if(!bOffline)
		WLServ_SetColour(colour, details.id, details.players[id].id, item);
	else
		details.players[id].colour = item;
}

void LobbyScreen::CommitColour(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetError(colour);

	if(err == SE_NO_ERROR)
		details.players[colourID].colour = newColour;
	else
		pColours[colourID]->SetSelection(details.players[colourID].colour - 1);
}

void LobbyScreen::SetHero(int item, int id)
{
	if(item == details.players[id].hero)
		return;

	heroID = id;
	newHero = item;

	if(!bOffline)
		WLServ_SetHero(hero, details.id, details.players[id].id, item);
	else
		details.players[id].hero = item;
}

void LobbyScreen::CommitHero(HTTPRequest::Status status)
{
	ServerError err = WLServResult_GetError(hero);

	if(err == SE_NO_ERROR)
		details.players[heroID].hero = newHero;
	else
		pHeroes[heroID]->SetSelection(details.players[heroID].hero);
}
