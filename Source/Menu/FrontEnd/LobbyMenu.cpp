#include "Warlords.h"
#include "Haku/UI/HKUI.h"

#include "Menu/FrontEnd/LobbyMenu.h"
#include "Menu/Menu.h"
#include "Map.h"
#include "Lobby.h"
#include "Profile.h"

#include "Menu/Game/GameUI.h"

void LobbyMenu::Load(HKWidget *pRoot, FrontMenu *_pFrontMenu)
{
	pMenu = pRoot;

	pFrontMenu = _pFrontMenu;

	if(pMenu)
	{
		// setup the UI
		pTitle = pMenu->FindChild<HKWidgetLabel>("lobby_label");
		pStartButton = pMenu->FindChild<HKWidgetButton>("start");
		pLeaveButton = pMenu->FindChild<HKWidgetButton>("leave");
		pReturnButton = pMenu->FindChild<HKWidgetButton>("return");

		pStartButton->OnClicked += fastdelegate::MakeDelegate(this, &LobbyMenu::OnStartClicked);
		pLeaveButton->OnClicked += fastdelegate::MakeDelegate(this, &LobbyMenu::OnLeaveClicked);
		pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &LobbyMenu::OnReturnClicked);

		for(int a=0; a<8; ++a)
		{
			players[a].pPlayerRow = pMenu->FindChild(MFString::Format("player%d", a).CStr());

			players[a].pName = players[a].pPlayerRow->FindChild<HKWidgetLabel>("name");
			players[a].pPlayerConfig = players[a].pPlayerRow->FindChild("playerSettings");

			players[a].pRace = players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("race");
			players[a].pColour = players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("colour");
			players[a].pHero = players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("hero");

			players[a].pRace->Bind(raceListAdapter);
			players[a].pRace->SetUserData((void*)a);
			players[a].pRace->OnSelChanged += fastdelegate::MakeDelegate(this, &LobbyMenu::OnRaceChanged);
			players[a].pColour->Bind(colourListAdapter);
			players[a].pColour->SetUserData((void*)a);
			players[a].pColour->OnSelChanged += fastdelegate::MakeDelegate(this, &LobbyMenu::OnColourChanged);
			players[a].pHero->Bind(players[a].heroListAdapter);
			players[a].pHero->SetUserData((void*)a);
			players[a].pHero->OnSelChanged += fastdelegate::MakeDelegate(this, &LobbyMenu::OnHeroChanged);
		}
	}
}

void LobbyMenu::Show(Lobby *pGame)
{
	pLobby = pGame;

	// get some detaials
	bool bIsOnline = pLobby->Online();

	// load the map info
	pLobby->LoadMap();

	// populate lists
	bUpdatingLists = true;

	raceList.clear();
	colourList.clear();

	// add random item
	ListItem item;
	item.name = "Random";
	item.id = 0;
	raceList.push(item);

	const MapTemplate &map = pLobby->Map();
	const UnitDefinitions *pUnitDefs = map.UnitDefs();
	for(int a=1; a<pUnitDefs->GetNumRaces(); ++a)
	{
		if(map.IsRacePresent(a))
		{
			ListItem item;
			item.name = pUnitDefs->GetRaceName(a);
			item.id = a;
			raceList.push(item);
		}

		colourList.push(pUnitDefs->GetRaceColour(a));
	}

	// set the lobby title
	pTitle->SetText("REMOVE ME");

	// update the player UI
	UpdateUIState();

	bUpdatingLists = false;

	// show the start or leave button
	pStartButton->SetVisible(bIsOnline ? HKWidget::Gone : HKWidget::Visible);
	pLeaveButton->SetVisible(bIsOnline ? HKWidget::Visible : HKWidget::Gone);

	FrontMenu::Get()->ShowAsCurrent(pMenu);

	if(bIsOnline)
	{
		// TODO...
/*
		// connect to the message server
		pSession->SetMessageCallback(fastdelegate::MakeDelegate(this, &LobbyMenu::ReceivePeerMessage));
		pSession->EnableRealtimeConnection(true);

		// send join message
		int player;
		GameDetails::Player *pPlayer = pSession->GetLobbyPlayer(-1, &player);
		pSession->SendMessageToPeers(MFStr("JOIN:%d:%s:%d:%d:%d", player, pPlayer->name, pPlayer->race, pPlayer->colour, pPlayer->hero));
*/
	}
}

void LobbyMenu::UpdateUIState()
{
	bool bIsOnline = pLobby->Online();

	// update the player UI
	int maxPlayers = pLobby->Players().size();
	for(int a=0; a<8; ++a)
	{
		bool bPlayerVisible = a < maxPlayers && (!bIsOnline || pLobby->Players(a).pUser != NULL);

		players[a].pPlayerRow->SetVisible(a < maxPlayers ? HKWidget::Visible : HKWidget::Gone); // show the row if the map has enough players
		players[a].pPlayerConfig->SetVisible(bPlayerVisible ? HKWidget::Visible : HKWidget::Gone); // hide the settings if the player has not yet joined

		if(bPlayerVisible)
		{
			Lobby::Player &player = pLobby->Players(a);

			// set player name
			players[a].pName->SetText(bIsOnline ? player.pUser->Name() : MFString::Format("Player %d", a + 1));

			// set the players selection
			players[a].pRace->SetSelection(player.race);
			players[a].pColour->SetSelection(player.colour - 1);

			RepopulateHeroes(a, player.race, player.hero);

			// disable selectboxes that aren't yours
			if(bIsOnline && Session::Get()->User() != player.pUser)
			{
				players[a].pRace->SetEnabled(false);
				players[a].pColour->SetEnabled(false);
				players[a].pHero->SetEnabled(false);
			}
		}
		else if(a < maxPlayers)
		{
			// set player name
			players[a].pName->SetText("Waiting for player...");
		}
	}
}

Lobby::Player *LobbyMenu::GetLobbyPlayer(uint32 id, int *pPlayer)
{
	for(int a=0; a<pLobby->NumPlayers(); ++a)
	{
		if(pLobby->Players(a).pUser->ID() == id)
		{
			if(pPlayer)
				*pPlayer = a;
			return &pLobby->Players(a);
		}
	}
	return NULL;
}

void LobbyMenu::RepopulateHeroes(int player, int race, int hero)
{
	bUpdatingHeroes = true;

	players[player].heroList.clear();

	// add random hero item
	ListItem i;
	i.name = "Random";
	i.id = -1;
	players[player].heroList.push(i);

	// find and populate heroes
	const UnitDefinitions *pUnitDefs = pLobby->Map().UnitDefs();
	for(int b=0; b<pUnitDefs->GetNumUnitTypes(); ++b)
	{
		const UnitDetails &unit = pUnitDefs->GetUnitDetails(b);
		if(unit.type == UT_Hero && (unit.race == race || unit.race == 0))
		{
			ListItem i;
			i.name = unit.name;
			i.id = b;
			players[player].heroList.push(i);
		}
	}

	bUpdatingHeroes = false;

	players[player].pHero->SetSelection(hero + 1);
}
/*
void LobbyMenu::ReceivePeerMessage(uint32 user, const char *pMessage)
{
	Session *pSession = Session::Get();

	int player;
	Lobby::Player *pPlayer = GetLobbyPlayer(user, &player);

	do
	{
		char *pNextLine = MFString_Chr(pMessage, '\n');
		if(pNextLine)
			*pNextLine++ = 0;

		// parse args
		char *pArgs[32];
		int numArgs = 0;

		char *pArg = (char*)pMessage;
		for(int a=0; a<32; ++a)
		{
			pArg = MFString_Chr(pArg, ':');
			if(pArg)
			{
				*pArg++ = 0;
				pArgs[a] = pArg;

				++numArgs;
			}
			else
			{
				break;
			}
		}

		if(!MFString_CaseCmp(pMessage, "JOIN"))
		{
			int pos = MFString_AsciiToInteger(pArgs[0]);
			GameDetails::Player &player = game.players[pos];

			if(player.id != user)
			{
				MFDebug_Assert(player.id == 0, "Player already present!?");

				// parse the args
				char *pName = pArgs[1];
				int race = MFString_AsciiToInteger(pArgs[2]);
				int colour = MFString_AsciiToInteger(pArgs[3]);
				int hero = MFString_AsciiToInteger(pArgs[4]);

				// set the player details
				player.id = user;
				MFString_Copy(player.name, pName);
				player.race = race;
				player.colour = colour;
				player.hero = hero;

				// make the player visible
				players[pos].pPlayerRow->SetVisible(HKWidget::Visible);
				players[pos].pPlayerConfig->SetVisible(HKWidget::Visible);

				// and set up the players UI
				players[pos].pName->SetText(pName);

				// set the players selection
				players[pos].pRace->SetSelection(race);
				players[pos].pColour->SetSelection(colour - 1);
				RepopulateHeroes(pos, race, hero);

				// disable the UI
				players[pos].pRace->SetEnabled(false);
				players[pos].pColour->SetEnabled(false);
				players[pos].pHero->SetEnabled(false);

				++game.numPlayers;

				pStartButton->SetEnabled(game.numPlayers == game.maxPlayers);
			}
		}
		else if(!MFString_CaseCmp(pMessage, "LEAVE"))
		{
			if(pPlayer)
			{
				// shuffle the players up one slot.
				for(int a = player; a < game.numPlayers; ++a)
				{
					if(a < game.numPlayers - 1)
					{
						game.players[a] = game.players[a + 1];

						// and set up the players UI
						players[a].pName->SetText(game.players[a].name);

						// set the players selection
						players[a].pRace->SetSelection(game.players[a].race);
						players[a].pColour->SetSelection(game.players[a].colour - 1);
						RepopulateHeroes(a, game.players[a].race, game.players[a].hero);

						// disable the UI
						bool bIsMe = game.players[a].id == pSession->GetUserID();
						players[a].pRace->SetEnabled(bIsMe);
						players[a].pColour->SetEnabled(bIsMe);
						players[a].pHero->SetEnabled(bIsMe);
					}
					else
					{
						game.players[a].id = 0;
						players[a].pPlayerConfig->SetVisible(HKWidget::Gone);
						players[a].pName->SetText("Waiting for player...");
					}
				}

				--game.numPlayers;

				pStartButton->SetEnabled(false);
			}
		}
		else if(!MFString_CaseCmp(pMessage, "SETRACE"))
		{
			if(pPlayer)
			{
				int race = MFString_AsciiToInteger(pArgs[0]);
				pPlayer->race = race;
				pPlayer->hero = -1;
				players[player].pRace->SetSelection(race);
				RepopulateHeroes(player, race, -1);
			}
		}
		else if(!MFString_CaseCmp(pMessage, "SETCOLOUR"))
		{
			if(pPlayer)
			{
				int colour = MFString_AsciiToInteger(pArgs[0]);
				pPlayer->colour = colour;
				players[player].pColour->SetSelection(colour - 1);
			}
		}
		else if(!MFString_CaseCmp(pMessage, "SETHERO"))
		{
			if(pPlayer)
			{
				int hero = MFString_AsciiToInteger(pArgs[0]);
				pPlayer->hero = hero;
				players[player].pHero->SetSelection(hero + 1);
			}
		}
		else if(!MFString_CaseCmp(pMessage, "START"))
		{
			// need to load game state from server...
			pSession->EnterGame(game.id, MakeDelegate(this, &LobbyMenu::StartGame));

			// TODO: disable ALL UI...
		}

		pMessage = pNextLine;
	}
	while(pMessage);
}
*/

void LobbyMenu::OnStartClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	MFDebug_Assert(!pLobby->Online(), "Something went wrong?!");

	// create a new game from the lobby
	GameState *pGameState = GameState::NewGame(pLobby);

	// destroy the lobby
	delete pLobby;

	// create the game object
	Game *pGame = new Game(*pGameState);

	// begin the first turn
	pGame->BeginTurn(0);

	// show the game
	pGame->Show();

	// and hide the menu
	FrontMenu::Get()->Hide();
}

void LobbyMenu::OnLeaveClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	ServerRequest *pReq = new ServerRequest(MakeDelegate(this, &LobbyMenu::OnGameLeft));
	pReq->LeaveGame(Session::Get(), pLobby->ID());
}

void LobbyMenu::OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
/*
	Session *pSession = Session::Get();
	pSession->EnableRealtimeConnection(false);
*/
	pFrontMenu->OnReturnClicked(sender, ev);
}

void LobbyMenu::OnRaceChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingLists)
		return;

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int playerIndex = (int)(uintp)sender.GetUserData();
	Lobby::Player &player = pLobby->Players(playerIndex);

	// change race
	int race = raceList[selectEv.selection].id;
	if(player.race != race)
	{
		if(pLobby->Online())
		{
			setPlayer = playerIndex;
			setRace = race;

			ServerRequest *pReq = new ServerRequest(MakeDelegate(this, &LobbyMenu::Commit));
			pReq->ConfigureGame(Session::Get(), pLobby->ID(), race, player.colour, player.hero, player.bReady);
//			pSession->SetRace(game.id, race, MakeDelegate(this, &LobbyMenu::CommitRace));

			// disable the race box until this completes
			players[playerIndex].pRace->SetEnabled(false);
			players[playerIndex].pColour->SetEnabled(false);
			players[playerIndex].pHero->SetEnabled(false);
		}
		else
			player.race = race;

		// update hero list
		RepopulateHeroes(playerIndex, race, -1);
	}
}

void LobbyMenu::OnColourChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingLists)
		return;

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int playerIndex = (int)(uintp)sender.GetUserData();
	Lobby::Player &player = pLobby->Players(playerIndex);

	// change colour
	int colour = selectEv.selection + 1; // we exclude the merc colour
	if(player.colour != colour)
	{
		if(pLobby->Online())
		{
			setPlayer = playerIndex;
			setColour = colour;

			ServerRequest *pReq = new ServerRequest(MakeDelegate(this, &LobbyMenu::Commit));
			pReq->ConfigureGame(Session::Get(), pLobby->ID(), player.race, colour, player.hero, player.bReady);
//			pSession->SetColour(game.id, colour, MakeDelegate(this, &LobbyMenu::CommitColour));

			// disable the colour box until this completes
			players[playerIndex].pRace->SetEnabled(false);
			players[playerIndex].pColour->SetEnabled(false);
			players[playerIndex].pHero->SetEnabled(false);
		}
		else
			player.colour = colour;
	}
}

void LobbyMenu::OnHeroChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingHeroes)
		return;

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int playerIndex = (int)(uintp)sender.GetUserData();
	Lobby::Player &player = pLobby->Players(playerIndex);

	// change hero
//	int hero = players[player].heroList[selectEv.selection].id;
	int hero = selectEv.selection - 1;
	if(player.hero != hero)
	{
		if(pLobby->Online())
		{
			setPlayer = playerIndex;
			setHero = hero;

			ServerRequest *pReq = new ServerRequest(MakeDelegate(this, &LobbyMenu::Commit));
			pReq->ConfigureGame(Session::Get(), pLobby->ID(), player.race, player.colour, hero, player.bReady);
//			pSession->SetHero(game.id, hero, MakeDelegate(this, &LobbyMenu::CommitHero));

			// disable the hero box until this completes
			players[playerIndex].pRace->SetEnabled(false);
			players[playerIndex].pColour->SetEnabled(false);
			players[playerIndex].pHero->SetEnabled(false);
		}
		else
			player.hero = hero;
	}
}

void LobbyMenu::Commit(ServerRequest *pReq)
{
	players[setPlayer].pRace->SetEnabled(true);
	players[setPlayer].pColour->SetEnabled(true);
	players[setPlayer].pHero->SetEnabled(true);

	if(pReq->Status() == ServerRequest::SE_NO_ERROR)
	{
		Lobby::FromJson(pReq->Json());
		UpdateUIState();
	}
	else
		players[setPlayer].pRace->SetSelection(pLobby->Players(setPlayer).race);

	delete pReq;
}
/*
void LobbyMenu::CommitRace(ServerError error, Session *pSession)
{
	players[setPlayer].pRace->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].race = setRace;
	else
		players[setPlayer].pRace->SetSelection(game.players[setPlayer].race);
}

void LobbyMenu::CommitColour(ServerError error, Session *pSession)
{
	players[setPlayer].pColour->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].colour = setColour;
	else
		players[setPlayer].pColour->SetSelection(game.players[setPlayer].colour - 1);
}

void LobbyMenu::CommitHero(ServerError error, Session *pSession)
{
	players[setPlayer].pHero->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].hero = setHero;
	else
		players[setPlayer].pHero->SetSelection(game.players[setPlayer].hero + 1);
}

void LobbyMenu::OnBegin(ServerError error, Session *pSession)
{
	if(error != SE_NO_ERROR)
		return;

	// start game
	params.gameID = game.id;
	Game *pGame = Game::NewGame(&params);
	Game::SetCurrent(pGame);

	pSession->SendMessageToPeers("START");

	FrontMenu::Get()->Hide();
}
*/
void LobbyMenu::OnGameLeft(ServerRequest *pReq)
{
	// TODO: remove lobby from profile

	HKWidgetEventInfo ev(pReturnButton);
	FrontMenu::Get()->HideCurrent();
	FrontMenu::Get()->ShowMainMenu();

//	pSession->EnableRealtimeConnection(false);

	delete pReq;
}
/*
void LobbyMenu::StartGame(ServerError error, Session *pSession, GameState *pState)
{
	if(error != SE_NO_ERROR)
		return;

	// start game
//	Game *pGame = new Game(pState);
	Game *pGame = Game::ResumeGame(NULL, false);
	Game::SetCurrent(pGame);

	FrontMenu::Get()->Hide();	
}
*/
// list adapter
HKWidget *LobbyMenu::GameListAdapter::GetItemView(int index, LobbyMenu::ListItem &item)
{
	HKWidgetLabel *pLabel = HKUserInterface::Get().CreateWidget<HKWidgetLabel>();
	pLabel->SetProperty("text_font", "FranklinGothic");
	pLabel->SetLayoutJustification(HKWidget::TopFill);
	pLabel->SetTextColour(MFVector::white);
	return pLabel;
}

void LobbyMenu::GameListAdapter::UpdateItemView(int index, LobbyMenu::ListItem &item, HKWidget *pLayout)
{
	HKWidgetLabel *pLabel = (HKWidgetLabel*)pLayout;
	pLabel->SetText(item.name);
}

// colour list adapter
HKWidget *LobbyMenu::ColourListAdapter::GetItemView(int index, MFVector &item)
{
	HKWidget *pWidget = HKUserInterface::Get().CreateWidget<HKWidget>();
	pWidget->SetSize(MakeVector(20, 20));
	return pWidget;
}

void LobbyMenu::ColourListAdapter::UpdateItemView(int index, MFVector &item, HKWidget *pLayout)
{
	pLayout->SetProperty("background_colour", item.ToString4());
}
