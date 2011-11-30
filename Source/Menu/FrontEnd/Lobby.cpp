#include "Warlords.h"
#include "UI/HKUI.h"

#include "Menu/FrontEnd/Lobby.h"
#include "Menu/Menu.h"
#include "Editor.h"

extern Game *pGame;
extern Editor *pEditor;

void LobbyMenu::Load(HKWidget *pRoot, FrontMenu *pFrontMenu)
{
	pMenu = pRoot;

	if(pMenu)
	{
		// setup the UI
		pTitle = pMenu->FindChild<HKWidgetLabel>("lobby_label");
		pStartButton = pMenu->FindChild<HKWidgetButton>("start");
		pLeaveButton = pMenu->FindChild<HKWidgetButton>("leave");
		pReturnButton = pMenu->FindChild<HKWidgetButton>("return");

		pStartButton->OnClicked += fastdelegate::MakeDelegate(this, &LobbyMenu::OnStartClicked);
		pLeaveButton->OnClicked += fastdelegate::MakeDelegate(this, &LobbyMenu::OnLeaveClicked);
		pReturnButton->OnClicked += fastdelegate::MakeDelegate(pFrontMenu, &FrontMenu::OnReturnClicked);

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

void LobbyMenu::Show(GameDetails &_game)
{
	Session *pSession = Session::Get();
	game = _game;

	// make current and set the begin callback
	pSession->SetBeginDelegate(MakeDelegate(this, &LobbyMenu::OnBegin));
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

	// add random item
	ListItem item;
	item.name = "Random";
	item.id = 0;
	raceList.push(item);

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
			players[a].pRace->SetSelection(game.players[a].race);
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
	for(int b=0; b<game.mapDetails.unitSetDetails.numUnits; ++b)
	{
		UnitSetDetails::Unit &unit = game.mapDetails.unitSetDetails.units[b];
		if(unit.type == UT_Hero && (unit.race == race || unit.race == 0))
		{
			ListItem i;
			i.name = unit.name;
			i.id = b;
			players[player].heroList.push(i);
		}
	}

	bUpdatingHeroes = false;

	players[player].pHero->SetSelection(hero+1);
}

void LobbyMenu::OnStartClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	MFDebug_Assert(game.maxPlayers == game.numPlayers, "Something went wrong?!");

	for(int a=0; a<game.maxPlayers; ++a)
	{
		game.players[a].race = players[a].pRace->GetSelection();
		game.players[a].colour = players[a].pColour->GetSelection() + 1;
		game.players[a].hero = players[a].pHero->GetSelection() - 1;

		if(game.players[a].race == 0)
		{
			// select random race
			while(game.players[a].race == 0)
			{
				int r = (MFRand() % (game.mapDetails.unitSetDetails.numRaces - 1)) + 1;
				if(game.mapDetails.bRacePresent[r])
					game.players[a].race = r;
			}
		}

		if(game.players[a].hero == -1)
		{
			// count the number of heroes available
			int numHeroes = 0;
			for(int b=0; b<game.mapDetails.unitSetDetails.numUnits; ++b)
			{
				UnitSetDetails::Unit &unit = game.mapDetails.unitSetDetails.units[b];
				if(unit.type == UT_Hero && (unit.race == game.players[a].race || unit.race == 0))
					++numHeroes;
			}

			// select random hero
			game.players[a].hero = MFRand() % numHeroes;
		}
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

void LobbyMenu::OnLeaveClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Session *pSession = Session::Get();
	pSession->LeaveGame(game.id, MakeDelegate(this, &LobbyMenu::OnGameLeft));
}

void LobbyMenu::OnRaceChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
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

			pSession->SetRace(game.id, race, MakeDelegate(this, &LobbyMenu::CommitRace));

			// disable the race box until this completes
			players[player].pRace->SetEnabled(false);
		}
		else
		{
			game.players[player].race = race;
		}

		// update hero list
		RepopulateHeroes(player, race, -1);
	}
}

void LobbyMenu::OnColourChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
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

			pSession->SetColour(game.id, colour, MakeDelegate(this, &LobbyMenu::CommitColour));

			// disable the colour box until this completes
			players[player].pColour->SetEnabled(false);
		}
		else
		{
			game.players[player].colour = colour;
		}
	}
}

void LobbyMenu::OnHeroChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
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

			pSession->SetHero(game.id, hero, MakeDelegate(this, &LobbyMenu::CommitHero));

			// disable the hero box until this completes
			players[player].pHero->SetEnabled(false);
		}
		else
		{
			game.players[player].hero = hero;
		}
	}
}

void LobbyMenu::CommitRace(ServerError error, Session *pSession)
{
	players[setPlayer].pRace->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].race = setRace;
	else
		players[setPlayer].pRace->SetSelection(game.players[setPlayer].race - 1);
}

void LobbyMenu::CommitColour(ServerError error, Session *pSession)
{
	players[setPlayer].pColour->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].colour = setColour;
	else
		players[setPlayer].pHero->SetSelection(game.players[setPlayer].colour - 1);
}

void LobbyMenu::CommitHero(ServerError error, Session *pSession)
{
	players[setPlayer].pHero->SetEnabled(true);

	if(error == SE_NO_ERROR)
		game.players[setPlayer].hero = setHero;
	else
		players[setPlayer].pHero->SetSelection(game.players[setPlayer].hero);
}

void LobbyMenu::OnBegin(ServerError error, Session *pSession)
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

void LobbyMenu::OnGameLeft(ServerError error, Session *pSession)
{
	HKWidgetEventInfo ev(pReturnButton);
	FrontMenu::Get()->HideCurrent();
	FrontMenu::Get()->ShowMainMenu();
}

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
