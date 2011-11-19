#include "Warlords.h"
#include "UI/HKUI.h"
#include "UI/HKWidgetStyle.h"
#include "UI/HKWidgetLoader-XML.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"

#include "Menu.h"
#include "Session.h"
#include "Editor.h"

extern Game *pGame;
extern Editor *pEditor;

extern FrontMenu *pFrontMenu;
FrontMenu *FrontMenu::Get()
{
	return pFrontMenu;
}

FrontMenu::FrontMenu()
{
	// init members
	pCurrentWindow = NULL;

	// load the styles
	HKWidgetStyle::LoadStylesFromXML("menu-style.xml");

	// load the menu
	pMenu = HKWidget_CreateFromXML("menu.xml");
	MFDebug_Assert(pMenu, "Failed to load front menu UI!");

	// configure main menu
	mainMenu.pMenu = pMenu->FindChild("mainmenu");
	if(mainMenu.pMenu)
	{
		mainMenu.pPlayButton = mainMenu.pMenu->FindChild<HKWidgetButton>("play_game");
		mainMenu.pResumeButton = mainMenu.pMenu->FindChild<HKWidgetButton>("resume_game");
		mainMenu.pLoginButton = mainMenu.pMenu->FindChild<HKWidgetButton>("login");
		mainMenu.pProfileButton = mainMenu.pMenu->FindChild<HKWidgetButton>("profile");
		mainMenu.pTitle = mainMenu.pMenu->FindChild<HKWidgetLabel>("welcome_text");

		mainMenu.pPlayButton->OnClicked += fastdelegate::MakeDelegate(&mainMenu, &FrontMenu::MainMenu::OnPlayClicked);
		mainMenu.pResumeButton->OnClicked += fastdelegate::MakeDelegate(&mainMenu, &FrontMenu::MainMenu::OnResumeClicked);
		mainMenu.pLoginButton->OnClicked += fastdelegate::MakeDelegate(&mainMenu, &FrontMenu::MainMenu::OnLoginClicked);
		mainMenu.pProfileButton->OnClicked += fastdelegate::MakeDelegate(&mainMenu, &FrontMenu::MainMenu::OnProfileClicked);
	}

	// configure new game menu
	playMenu.pMenu = pMenu->FindChild("playgame");
	if(playMenu.pMenu)
	{
		playMenu.pCreateButton = playMenu.pMenu->FindChild<HKWidgetButton>("create");
		playMenu.pJoinButton = playMenu.pMenu->FindChild<HKWidgetButton>("join");
		playMenu.pOfflineButton = playMenu.pMenu->FindChild<HKWidgetButton>("offline");
		playMenu.pReturnButton = playMenu.pMenu->FindChild<HKWidgetButton>("return");

		playMenu.pCreateButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &FrontMenu::PlayMenu::OnCreateClicked);
		playMenu.pJoinButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &FrontMenu::PlayMenu::OnJoinClicked);
		playMenu.pOfflineButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &FrontMenu::PlayMenu::OnOfflineClicked);
		playMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &FrontMenu::OnReturnClicked);
	}

	// configure game select menu
	listMenu.pMenu = pMenu->FindChild("resume");
	if(listMenu.pMenu)
	{
		listMenu.pActiveList = listMenu.pMenu->FindChild<HKWidgetListbox>("activegames");
		listMenu.pResumeButton = listMenu.pMenu->FindChild<HKWidgetButton>("resume");
		listMenu.pReturnButton = listMenu.pMenu->FindChild<HKWidgetButton>("return");
		listMenu.pTitle = listMenu.pMenu->FindChild<HKWidgetLabel>("resume_label");
		listMenu.pNamePanel = listMenu.pMenu->FindChild("gameNamePanel");
		listMenu.pName = listMenu.pMenu->FindChild<HKWidgetTextbox>("gameName");

		listMenu.pActiveList->Bind(listMenu.gameListAdapter);
		listMenu.pActiveList->OnSelChanged += fastdelegate::MakeDelegate(&listMenu, &FrontMenu::ListMenu::OnSelect);
		listMenu.pName->OnChanged += fastdelegate::MakeDelegate(&listMenu, &FrontMenu::ListMenu::OnNameChanged);
		listMenu.pResumeButton->OnClicked += fastdelegate::MakeDelegate(&listMenu, &FrontMenu::ListMenu::OnContinueClicked);
		listMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(&listMenu, &FrontMenu::ListMenu::OnReturnClicked);
	}

	// configure login window
	loginMenu.pMenu = pMenu->FindChild("login");
	if(loginMenu.pMenu)
	{
		loginMenu.pUsernameText = loginMenu.pMenu->FindChild<HKWidgetTextbox>("username");
		loginMenu.pPasswordText = loginMenu.pMenu->FindChild<HKWidgetTextbox>("password");
		loginMenu.pLoginButton = loginMenu.pMenu->FindChild<HKWidgetButton>("login");
		loginMenu.pReturnButton = loginMenu.pMenu->FindChild<HKWidgetButton>("return");

		loginMenu.pUsernameText->OnChanged += fastdelegate::MakeDelegate(&loginMenu, &FrontMenu::LoginMenu::OnUpdateLogin);
		loginMenu.pPasswordText->OnChanged += fastdelegate::MakeDelegate(&loginMenu, &FrontMenu::LoginMenu::OnUpdateLogin);
		loginMenu.pLoginButton->OnClicked += fastdelegate::MakeDelegate(&loginMenu, &FrontMenu::LoginMenu::OnLogin);
		loginMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &FrontMenu::OnReturnClicked);
	}

	// configure lobby window
	lobbyMenu.pMenu = pMenu->FindChild("lobby");
	if(lobbyMenu.pMenu)
	{
		// setup the UI
		lobbyMenu.pTitle = lobbyMenu.pMenu->FindChild<HKWidgetLabel>("lobby_label");
		lobbyMenu.pStartButton = lobbyMenu.pMenu->FindChild<HKWidgetButton>("start");
		lobbyMenu.pLeaveButton = lobbyMenu.pMenu->FindChild<HKWidgetButton>("leave");
		lobbyMenu.pReturnButton = lobbyMenu.pMenu->FindChild<HKWidgetButton>("return");

		lobbyMenu.pStartButton->OnClicked += fastdelegate::MakeDelegate(&lobbyMenu, &FrontMenu::LobbyMenu::OnStartClicked);
		lobbyMenu.pLeaveButton->OnClicked += fastdelegate::MakeDelegate(&lobbyMenu, &FrontMenu::LobbyMenu::OnLeaveClicked);
		lobbyMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &FrontMenu::OnReturnClicked);

		for(int a=0; a<8; ++a)
		{
			lobbyMenu.players[a].pPlayerRow = lobbyMenu.pMenu->FindChild(MFString::Format("player%d", a).CStr());

			lobbyMenu.players[a].pName = lobbyMenu.players[a].pPlayerRow->FindChild<HKWidgetLabel>("name");
			lobbyMenu.players[a].pPlayerConfig = lobbyMenu.players[a].pPlayerRow->FindChild("playerSettings");

			lobbyMenu.players[a].pRace = lobbyMenu.players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("race");
			lobbyMenu.players[a].pColour = lobbyMenu.players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("colour");
			lobbyMenu.players[a].pHero = lobbyMenu.players[a].pPlayerRow->FindChild<HKWidgetSelectbox>("hero");

			lobbyMenu.players[a].pRace->Bind(lobbyMenu.raceListAdapter);
			lobbyMenu.players[a].pRace->SetUserData((void*)a);
			lobbyMenu.players[a].pRace->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &FrontMenu::LobbyMenu::OnRaceChanged);
			lobbyMenu.players[a].pColour->Bind(lobbyMenu.colourListAdapter);
			lobbyMenu.players[a].pColour->SetUserData((void*)a);
			lobbyMenu.players[a].pColour->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &FrontMenu::LobbyMenu::OnColourChanged);
			lobbyMenu.players[a].pHero->Bind(lobbyMenu.players[a].heroListAdapter);
			lobbyMenu.players[a].pHero->SetUserData((void*)a);
			lobbyMenu.players[a].pHero->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &FrontMenu::LobbyMenu::OnHeroChanged);
		}
	}

	// configure profile window


	// and add the menu to the UI manager
	pMenu->SetVisible(HKWidget::Invisible);
	HKUserInterface::Get().AddTopLevelWidget(pMenu, false);
}

FrontMenu::~FrontMenu()
{
	delete pMenu;
}

void FrontMenu::Show()
{
	pMenu->SetVisible(HKWidget::Visible);
}

void FrontMenu::Hide()
{
	HideCurrent();
	pMenu->SetVisible(HKWidget::Invisible);
}

void FrontMenu::ShowAsCurrent(HKWidget *pMenu)
{
	pMenu->SetVisible(HKWidget::Visible);
	pCurrentWindow = pMenu;
}

void FrontMenu::HideCurrent()
{
	if(pCurrentWindow)
	{
		pCurrentWindow->SetVisible(HKWidget::Invisible);
		pCurrentWindow = NULL;
	}
}

// event handlers
void FrontMenu::OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.ShowMainMenu();
}

// MainMenu
void FrontMenu::MainMenu::Show()
{
	Session *pSession = Session::Get();

	// configure for player signed in
	if(pSession->IsLoggedIn())
	{
		pTitle->SetText(MFString("Welcome ") + pSession->GetUsername());

		pLoginButton->SetVisible(HKWidget::Gone);
		pProfileButton->SetVisible(HKWidget::Visible);
	}
	else
	{
		pTitle->SetText("Offline");

		pLoginButton->SetVisible(HKWidget::Visible);
		pProfileButton->SetVisible(HKWidget::Gone);
	}

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::MainMenu::OnPlayClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.playMenu.Show();
}

void FrontMenu::MainMenu::OnResumeClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.listMenu.ShowResume();
}

void FrontMenu::MainMenu::OnLoginClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.loginMenu.Show();
}

void FrontMenu::MainMenu::OnProfileClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
}

// PlayMenu
void FrontMenu::PlayMenu::Show()
{
	Session *pSession = Session::Get();

	// disable online menus
	bool bOnline = pSession->IsLoggedIn();
	pCreateButton->SetEnabled(bOnline);
	pJoinButton->SetEnabled(bOnline);

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::PlayMenu::OnCreateClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.listMenu.ShowCreate(true);
}

void FrontMenu::PlayMenu::OnJoinClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.listMenu.ShowJoin();
}

void FrontMenu::PlayMenu::OnOfflineClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();
	menu.listMenu.ShowCreate(false);
}

// LoginMenu
void FrontMenu::LoginMenu::Show()
{
	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::LoginMenu::OnUpdateLogin(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	// enable the login button only when both login and password contain something
	pLoginButton->SetEnabled(!pUsernameText->IsEmpty() && !pPasswordText->IsEmpty());
}

void FrontMenu::LoginMenu::OnLogin(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	pLoginButton->SetEnabled(false);
	pReturnButton->SetEnabled(false);

	Session *pSession = Session::Get();
	pSession->SetLoginDelegate(fastdelegate::MakeDelegate(this, &FrontMenu::LoginMenu::OnLoginResponse));
	pSession->Login(pUsernameText->GetString().CStr(), pPasswordText->GetString().CStr());
}

void FrontMenu::LoginMenu::OnLoginResponse(ServerError err, Session *pSession)
{
	pLoginButton->SetEnabled(true);
	pReturnButton->SetEnabled(true);

	if(err == SE_NO_ERROR)
	{
		// save the login details...
		MFString loginInfo = MFString::Format("%s\n%08X", pSession->GetUsername(), pSession->GetUserID());

		MFFileSystem_Save("home:login_info.ini", loginInfo.CStr(), loginInfo.NumBytes());

		// we are logged in, return to main menu...
		HKWidgetEventInfo ev(pReturnButton);
		FrontMenu::Get()->OnReturnClicked(*pReturnButton, ev);
	}
}

// ListMenu
void FrontMenu::ListMenu::ShowResume()
{
	type = Resume;

	// set the title
	pTitle->SetText("Resume Game");

	// disable resume button
	pResumeButton->SetEnabled(false);

	// hide string box
	pNamePanel->SetVisible(HKWidget::Gone);

	// update game list
	gameList.clear();

	Session *pSession = Session::Get();
	if(pSession->IsLoggedIn())
	{
		pSession->SetUpdateDelegate(fastdelegate::MakeDelegate(this, &FrontMenu::ListMenu::OnUpdateResponse));
		pSession->UpdateGames();
	}

	bReturnToMain = true;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::ListMenu::ShowJoin()
{
	type = Join;

	// set the title
	pTitle->SetText("Join Game");

	// disable resume button
	pResumeButton->SetEnabled(false);

	// show string box
	pNamePanel->SetVisible(HKWidget::Visible);
	pName->SetString(NULL);

	// collect game list
	gameList.clear();

	Session *pSession = Session::Get();
	if(pSession->IsLoggedIn())
	{
		pSession->FindGames(fastdelegate::MakeDelegate(this, &FrontMenu::ListMenu::OnFindResponse), NULL);
	}

	bReturnToMain = false;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::ListMenu::ShowCreate(bool bOnline)
{
	type = bOnline ? Create : Offline;

	// set the title
	pTitle->SetText(bOnline ? "Create Game" : "Offline Game");

	// disable resume button
	pResumeButton->SetEnabled(false);

	// show string box
	pNamePanel->SetVisible(bOnline ? HKWidget::Visible : HKWidget::Gone);
	pName->SetString(NULL);

	// collect map list
	gameList.clear();

	GameData *pData = GameData::Get();
	int numMaps = pData->GetNumMaps();
	for(int a=0; a<numMaps; ++a)
	{
		FrontMenu::ListItem i;
		i.name = pData->GetMapName(a);
		gameList.push(i);
	}

	bReturnToMain = false;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::ListMenu::ShowLobby(GameDetails &game)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	menu.lobbyMenu.Show(game);
}

void FrontMenu::ListMenu::OnSelect(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	int sel = pActiveList->GetSelection();

	switch(type)
	{
		case Join:
		{
			if(sel >= 0)
				pName->SetString(gameList[sel].name);
			break;
		}
		case Create:
		{
			pResumeButton->SetEnabled(sel >= 0 && !pName->IsEmpty());
			break;
		}
		case Offline:
		case Resume:
			pResumeButton->SetEnabled(sel >= 0);
			break;
		default:
			break;
	}
}

void FrontMenu::ListMenu::OnNameChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	switch(type)
	{
		case Join:
			pResumeButton->SetEnabled(!pName->IsEmpty());
			break;
		case Create:
		{
			int sel = pActiveList->GetSelection();
			pResumeButton->SetEnabled(sel >= 0 && !pName->IsEmpty());
			break;
		}
		default:
			break;
	}
}

void FrontMenu::ListMenu::OnContinueClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Session *pSession = Session::Get();

	switch(type)
	{
		case Create:
		{
			// create online game
			MFString map = gameList[pActiveList->GetSelection()].name;

			// create the game
			MapDetails mapDetails;
			Map::GetMapDetails(map.CStr(), &mapDetails);

			GameCreateDetails details;
			details.pName = pName->GetString().CStr();
			details.pMap = map.CStr();
			details.turnTime = 3600; // TODO: fix me?
			details.numPlayers = mapDetails.numPlayers;

			pSession->CreateGame(details, MakeDelegate(this, &FrontMenu::ListMenu::OnGameCreated));

			// disable the UI while it talks to the server...
			pActiveList->SetEnabled(false);
			pName->SetEnabled(false);
			pResumeButton->SetEnabled(false);
			pReturnButton->SetEnabled(false);
			break;
		}

		case Offline:
		{
			MFString map = gameList[pActiveList->GetSelection()].name;

			// create offline game
			GameDetails game;
			game.id = 0;
			MFString_Copy(game.name, "Offline Game");
			MFString_Copy(game.map, map.CStr());
			Map::GetMapDetails(game.map, &game.mapDetails);
			game.bMapDetailsLoaded = true;
			game.maxPlayers = game.mapDetails.numPlayers;
			game.numPlayers = game.maxPlayers;
			game.turnTime = 0;

			for(int a=0; a<game.numPlayers; ++a)
			{
				game.players[a].id = 0;
				MFString_Copy(game.players[a].name, MFStr("Player %d", a+1));
				game.players[a].colour = 1 + a;
				game.players[a].race = 1;
				game.players[a].hero = 0;
			}

			ShowLobby(game);
			break;
		}

		case Join:
		{
			// attempt to join the game...
			pSession->JoinGame(pName->GetString(), MakeDelegate(this, &FrontMenu::ListMenu::OnGameJoined));

			// disable the UI while it talks to the server...
			pActiveList->SetEnabled(false);
			pName->SetEnabled(false);
			pResumeButton->SetEnabled(false);
			pReturnButton->SetEnabled(false);
			break;
		}

		case Resume:
		{
			uint32 id = gameList[pActiveList->GetSelection()].id;

			int numGames = pSession->GetNumPendingGames();
			for(int a=0; a<numGames; ++a)
			{
				GameDetails *pGame = pSession->GetPendingGame(a);
				if(pGame->id == id)
				{
					ShowLobby(*pGame);
					return;
				}
			}

			numGames = pSession->GetNumCurrentGames();
			for(int a=0; a<numGames; ++a)
			{
				GameState *pState = pSession->GetCurrentGame(a);
				if(pState->id == id)
				{
					pGame = new Game(pState);
					Game::SetCurrent(pGame);

					FrontMenu::Get()->Hide();
					return;
				}
			}
			break;
		}

		default:
			break;
	}
}

void FrontMenu::ListMenu::OnGameCreated(ServerError error, Session *pSession, GameDetails *pGame)
{
	// enable all the UI bits again
	pActiveList->SetEnabled(true);
	pName->SetEnabled(true);
	pResumeButton->SetEnabled(true);
	pReturnButton->SetEnabled(true);

	// check the pSession has this stuff remembered somehow...
	if(error == SE_NO_ERROR)
	{
		ShowLobby(*pGame);
	}
}

void FrontMenu::ListMenu::OnGameJoined(ServerError error, Session *pSession, GameDetails *pGame)
{
	// enable all the UI bits again
	pActiveList->SetEnabled(true);
	pName->SetEnabled(true);
	pResumeButton->SetEnabled(true);
	pReturnButton->SetEnabled(true);

	if(error == SE_NO_ERROR)
	{
		ShowLobby(*pGame);
	}
}

void FrontMenu::ListMenu::OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	if(bReturnToMain)
		menu.ShowMainMenu();
	else
		menu.playMenu.Show();
}

void FrontMenu::ListMenu::OnUpdateResponse(ServerError err, Session *pSession)
{
	// populate game list...
	int numGames = pSession->GetNumCurrentGames();
	for(int a=0; a<numGames; ++a)
	{
		GameState *pState = pSession->GetCurrentGame(a);

		FrontMenu::ListItem i;
		i.id = pState->id;
		i.name = pState->name;
		i.map = pState->map;
		i.numPlayers = pState->numPlayers;

		gameList.push(i);
	}

	numGames = pSession->GetNumPendingGames();
	for(int a=0; a<numGames; ++a)
	{
		GameDetails *pState = pSession->GetPendingGame(a);

		FrontMenu::ListItem i;
		i.id = pState->id;
		i.name = pState->name;
		i.map = pState->map;
		i.numPlayers = pState->numPlayers;

		gameList.push(i);
	}
}

void FrontMenu::ListMenu::OnFindResponse(ServerError err, Session *pSession, GameLobby *pGames, int numGames)
{
	// populate game list...
	for(int a=0; a<numGames; ++a)
	{
		FrontMenu::ListItem i;
		i.id = pGames[a].id;
		i.name = pGames[a].name;
		i.numPlayers = 0;

		gameList.push(i);
	}
}

HKWidget *FrontMenu::GameListAdapter::GetItemView(int index, FrontMenu::ListItem &item)
{
	HKWidgetLabel *pLabel = HKUserInterface::Get().CreateWidget<HKWidgetLabel>();
	pLabel->SetProperty("text_font", "FranklinGothic");
	pLabel->SetLayoutJustification(HKWidget::TopFill);
	pLabel->SetTextColour(MFVector::white);
	return pLabel;
}

void FrontMenu::GameListAdapter::UpdateItemView(int index, FrontMenu::ListItem &item, HKWidget *pLayout)
{
	HKWidgetLabel *pLabel = (HKWidgetLabel*)pLayout;
	pLabel->SetText(item.name);
}

// LobbyMenu
void FrontMenu::LobbyMenu::Show(GameDetails &_game)
{
	Session *pSession = Session::Get();
	game = _game;

	// make current and set the begin callback
	pSession->SetBeginDelegate(MakeDelegate(this, &FrontMenu::LobbyMenu::OnBegin));
	pSession->MakeCurrent(game.id);

	// load the map info
	if(!game.bMapDetailsLoaded)
	{
		Map::GetMapDetails(game.map, &game.mapDetails);
		game.bMapDetailsLoaded = true;
	}

	// populate lists
	bUpdatingLists = true;

	raceList.clear();
	colourList.clear();

	for(int a=1; a<game.mapDetails.unitSetDetails.numRaces; ++a)
	{
		if(game.mapDetails.bRacePresent[a])
		{
			ListItem item;
			item.name = game.mapDetails.unitSetDetails.races[a];
			item.id = a;
			raceList.push(item);
		}

		MFVector v;
		v.FromPackedColour(game.mapDetails.unitSetDetails.colours[a]);
		colourList.push(v);
	}

	// set the lobby title
	pTitle->SetText(game.name);

	// update the players
	for(int a=0; a<8; ++a)
	{
		players[a].pPlayerRow->SetVisible(a < game.maxPlayers ? HKWidget::Visible : HKWidget::Gone); // show the row if the map has enough players
		players[a].pPlayerConfig->SetVisible(a < game.numPlayers ? HKWidget::Visible : HKWidget::Gone); // hide the settings if the player has not yet joined

		if(a < game.numPlayers)
		{
			// set player name
			players[a].pName->SetText(game.players[a].name);

			// set the players selection
			players[a].pRace->SetSelection(game.players[a].race - 1);
			players[a].pColour->SetSelection(game.players[a].colour - 1);

			RepopulateHeroes(a, game.players[a].race, game.players[a].hero);
		}
		else if(a < game.maxPlayers)
		{
			// set player name
			players[a].pName->SetText("Waiting for player...");
		}
	}

	bUpdatingLists = false;

	// show the start or leave button
	bool bCreator = pSession->IsCreator();
	pStartButton->SetVisible(bCreator ? HKWidget::Visible : HKWidget::Gone);
	pStartButton->SetEnabled(game.numPlayers == game.maxPlayers);

	pLeaveButton->SetVisible(!bCreator ? HKWidget::Visible : HKWidget::Gone);

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void FrontMenu::LobbyMenu::RepopulateHeroes(int player, int race, int hero)
{
	bUpdatingHeroes = true;

	players[player].heroList.clear();
	for(int b=0; b<game.mapDetails.unitSetDetails.numUnits; ++b)
	{
		if(game.mapDetails.unitSetDetails.units[b].type == UT_Hero && game.mapDetails.unitSetDetails.units[b].race == race)
		{
			ListItem i;
			i.name = game.mapDetails.unitSetDetails.units[b].name;
			i.id = b;
			players[player].heroList.push(i);
		}
	}

	bUpdatingHeroes = false;

	players[player].pHero->SetSelection(hero);
}

void FrontMenu::LobbyMenu::OnStartClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	MFDebug_Assert(game.maxPlayers == game.numPlayers, "Something went wrong?!");

	for(int a=0; a<game.maxPlayers; ++a)
	{
		game.players[a].race = players[a].pRace->GetSelection() + 1;
		game.players[a].colour = players[a].pColour->GetSelection() + 1;
		game.players[a].hero = players[a].pHero->GetSelection();
	}

	// begin game...

	// setup game parameters
	MFZeroMemory(&params, sizeof(params));
	params.bOnline = game.id != 0;
	params.bEditMap = false;
	params.gameID = game.id;
	params.pMap = game.map;

	// set up the players
	bool bAssigned[16];
	MFZeroMemory(bAssigned, sizeof(bAssigned));
	params.numPlayers = game.numPlayers;
	for(int a=0; a<game.numPlayers; ++a)
	{
		// select a random starting position for the player
		int p = MFRand() % params.numPlayers;
		while(bAssigned[p])
			p = MFRand() % params.numPlayers;
		bAssigned[p] = true;

		params.players[p].id = game.players[a].id;
		params.players[p].race = game.players[a].race;
		params.players[p].colour = game.players[a].colour;
		params.players[p].hero = game.players[a].hero;
	}

	if(game.id == 0)
	{
		// start game
		pGame = new Game(&params);
		Game::SetCurrent(pGame);

		if(params.bEditMap)
		{
			pEditor = new Editor(pGame);
			Screen::SetNext(pEditor);
		}
		else
			pGame->BeginGame();

		FrontMenu::Get()->Hide();
	}
	else
	{
		// create the game
		uint32 players[16];
		for(int a=0; a<params.numPlayers; ++a)
			players[a] = params.players[a].id;

		Session *pSession = Session::Get();
		pSession->BeginGame(game.id, players, params.numPlayers);
	}
}

void FrontMenu::LobbyMenu::OnLeaveClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Session *pSession = Session::Get();
	pSession->LeaveGame(game.id, MakeDelegate(this, &FrontMenu::LobbyMenu::OnGameLeft));
}

void FrontMenu::LobbyMenu::OnRaceChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingLists)
		return;

	Session *pSession = Session::Get();

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int player = (int)(uintp)sender.GetUserData();

	// change race
	int race = raceList[selectEv.selection].id;
	if(game.players[player].race != race)
	{
		if(game.id != 0)
		{
			setPlayer = player;
			setRace = race;

			pSession->SetRace(game.id, race, MakeDelegate(this, &FrontMenu::LobbyMenu::CommitRace));

			// disable the race box until this completes
			players[player].pRace->SetEnabled(false);
		}
		else
		{
			game.players[player].race = race;
		}

		// update hero list
		RepopulateHeroes(player, race, 0);
	}
}

void FrontMenu::LobbyMenu::OnColourChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingLists)
		return;

	Session *pSession = Session::Get();

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int player = (int)(uintp)sender.GetUserData();

	// change colour
	int colour = selectEv.selection + 1; // we exclude the merc colour
	if(game.players[player].colour != colour)
	{
		if(game.id != 0)
		{
			setPlayer = player;
			setColour = colour;

			pSession->SetColour(game.id, colour, MakeDelegate(this, &FrontMenu::LobbyMenu::CommitColour));

			// disable the colour box until this completes
			players[player].pColour->SetEnabled(false);
		}
		else
		{
			game.players[player].colour = colour;
		}
	}
}

void FrontMenu::LobbyMenu::OnHeroChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(bUpdatingHeroes)
		return;

	Session *pSession = Session::Get();

	HKWidgetSelectEvent &selectEv = (HKWidgetSelectEvent&)ev;
	int player = (int)(uintp)sender.GetUserData();

	// change hero
//	int hero = players[player].heroList[selectEv.selection].id;
	int hero = selectEv.selection;
	if(game.players[player].hero != hero)
	{
		if(game.id != 0)
		{
			setPlayer = player;
			setHero = hero;

			pSession->SetHero(game.id, hero, MakeDelegate(this, &FrontMenu::LobbyMenu::CommitHero));

			// disable the hero box until this completes
			players[player].pHero->SetEnabled(false);
		}
		else
		{
			game.players[player].hero = hero;
		}
	}
}

void FrontMenu::LobbyMenu::CommitRace(ServerError error, Session *pSession)
{
	players[setPlayer].pRace->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].race = setRace;
	else
		players[setPlayer].pRace->SetSelection(game.players[setPlayer].race - 1);
}

void FrontMenu::LobbyMenu::CommitColour(ServerError error, Session *pSession)
{
	players[setPlayer].pColour->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].colour = setColour;
	else
		players[setPlayer].pHero->SetSelection(game.players[setPlayer].colour - 1);
}

void FrontMenu::LobbyMenu::CommitHero(ServerError error, Session *pSession)
{
	players[setPlayer].pHero->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].hero = setHero;
	else
		players[setPlayer].pHero->SetSelection(game.players[setPlayer].hero);
}

void FrontMenu::LobbyMenu::OnBegin(ServerError error, Session *pSession)
{
	if(error == SE_NO_ERROR)
	{
		// start game
		params.gameID = game.id;
		pGame = new Game(&params);
		Game::SetCurrent(pGame);
		pGame->BeginGame();

		FrontMenu::Get()->Hide();
	}
}

void FrontMenu::LobbyMenu::OnGameLeft(ServerError error, Session *pSession)
{
	HKWidgetEventInfo ev(pReturnButton);
	FrontMenu::Get()->OnReturnClicked(*pReturnButton, ev);
}

HKWidget *FrontMenu::LobbyMenu::ColourListAdapter::GetItemView(int index, MFVector &item)
{
	HKWidget *pWidget = HKUserInterface::Get().CreateWidget<HKWidget>();
	pWidget->SetSize(MakeVector(20, 20));
	return pWidget;
}

void FrontMenu::LobbyMenu::ColourListAdapter::UpdateItemView(int index, MFVector &item, HKWidget *pLayout)
{
	pLayout->SetProperty("background_colour", item.ToString4());
}
