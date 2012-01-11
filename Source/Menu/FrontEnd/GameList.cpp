#include "Warlords.h"
#include "UI/HKUI.h"

#include "Menu/FrontEnd/GameList.h"
#include "Menu/Menu.h"

extern Game *pGame;

// ListMenu
void ListMenu::Load(HKWidget *pRoot, FrontMenu *pFrontMenu)
{
	pMenu = pRoot;

	if(pMenu)
	{
		pActiveList = pMenu->FindChild<HKWidgetListbox>("activegames");
		pResumeButton = pMenu->FindChild<HKWidgetButton>("resume");
		pReturnButton = pMenu->FindChild<HKWidgetButton>("return");
		pTitle = pMenu->FindChild<HKWidgetLabel>("resume_label");
		pNamePanel = pMenu->FindChild("gameNamePanel");
		pName = pMenu->FindChild<HKWidgetTextbox>("gameName");

		pActiveList->Bind(gameListAdapter);
		pActiveList->OnSelChanged += fastdelegate::MakeDelegate(this, &ListMenu::OnSelect);
		pName->OnChanged += fastdelegate::MakeDelegate(this, &ListMenu::OnNameChanged);
		pResumeButton->OnClicked += fastdelegate::MakeDelegate(this, &ListMenu::OnContinueClicked);
		pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &ListMenu::OnReturnClicked);
	}
}

void ListMenu::ShowResume()
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
		pSession->SetUpdateDelegate(fastdelegate::MakeDelegate(this, &ListMenu::OnUpdateResponse));
		pSession->UpdateGames();
	}

	bReturnToMain = true;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void ListMenu::ShowJoin()
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
		pSession->FindGames(fastdelegate::MakeDelegate(this, &ListMenu::OnFindResponse));
	}

	bReturnToMain = false;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void ListMenu::ShowCreate(bool bOnline)
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
		ListItem i;
		i.name = pData->GetMapName(a);
		gameList.push(i);
	}

	bReturnToMain = false;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}

void ListMenu::ShowLobby(GameDetails &game)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	menu.lobbyMenu.Show(game);
}

void ListMenu::OnSelect(HKWidget &sender, const HKWidgetEventInfo &ev)
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

void ListMenu::OnNameChanged(HKWidget &sender, const HKWidgetEventInfo &ev)
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

void ListMenu::OnContinueClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
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

			pSession->CreateGame(details, MakeDelegate(this, &ListMenu::OnGameCreated));

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
				game.players[a].race = 0;
				game.players[a].hero = -1;
			}

			ShowLobby(game);
			break;
		}

		case Join:
		{
			// attempt to join the game...
			pSession->JoinGame(pName->GetString(), MakeDelegate(this, &ListMenu::OnGameJoined));

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

void ListMenu::OnGameCreated(ServerError error, Session *pSession, GameDetails *pGame)
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

void ListMenu::OnGameJoined(ServerError error, Session *pSession, GameDetails *pGame)
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

void ListMenu::OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	if(bReturnToMain)
		menu.ShowMainMenu();
	else
		menu.playMenu.Show();
}

void ListMenu::OnUpdateResponse(ServerError err, Session *pSession)
{
	// populate game list...
	int numGames = pSession->GetNumCurrentGames();
	for(int a=0; a<numGames; ++a)
	{
		GameState *pState = pSession->GetCurrentGame(a);
		if(!pState)
			continue;

		ListItem i;
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
		if(!pState)
			continue;

		ListItem i;
		i.id = pState->id;
		i.name = pState->name;
		i.map = pState->map;
		i.numPlayers = pState->numPlayers;

		gameList.push(i);
	}
}

void ListMenu::OnFindResponse(ServerError err, Session *pSession, GameLobby *pGames, int numGames)
{
	// populate game list...
	for(int a=0; a<numGames; ++a)
	{
		ListItem i;
		i.id = pGames[a].id;
		i.name = pGames[a].name;
		i.numPlayers = 0;

		gameList.push(i);
	}
}

// list adapter
HKWidget *ListMenu::GameListAdapter::GetItemView(int index, ListItem &item)
{
	HKWidgetLabel *pLabel = HKUserInterface::Get().CreateWidget<HKWidgetLabel>();
	pLabel->SetProperty("text_font", "FranklinGothic");
	pLabel->SetLayoutJustification(HKWidget::TopFill);
	pLabel->SetTextColour(MFVector::white);
	return pLabel;
}

void ListMenu::GameListAdapter::UpdateItemView(int index, ListItem &item, HKWidget *pLayout)
{
	HKWidgetLabel *pLabel = (HKWidgetLabel*)pLayout;
	pLabel->SetText(item.name);
}
