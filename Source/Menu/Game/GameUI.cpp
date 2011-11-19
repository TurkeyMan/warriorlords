#include "Warlords.h"
#include "UI/HKUI.h"
#include "UI/HKWidgetLoader-XML.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"

#include "GameUI.h"
#include "Session.h"
#include "Editor.h"

extern Game *pGame;

extern GameMenu *pGameMenu;
GameMenu *GameMenu::Get()
{
	return pGameMenu;
}

GameMenu::GameMenu()
{
	// init members
	pCurrentWindow = NULL;

	// load the menu
	pMenu = HKWidget_CreateFromXML("game.xml");
	MFDebug_Assert(pMenu, "Failed to load front menu UI!");

	// configure main menu
	gameScreen.pMenu = pMenu->FindChild("map");
	if(gameScreen.pMenu)
	{
		gameScreen.pUndoButton = gameScreen.pMenu->FindChild<HKWidgetButton>("undo");
		gameScreen.pEndTurnButton = gameScreen.pMenu->FindChild<HKWidgetButton>("end_turn");
		gameScreen.pMinimapButton = gameScreen.pMenu->FindChild<HKWidgetButton>("minimap");

		gameScreen.pUndoButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameMenu::GameScreen::OnUndoClicked);
		gameScreen.pEndTurnButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameMenu::GameScreen::OnEndTurnClicked);
		gameScreen.pMinimapButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameMenu::GameScreen::OnMinimapClicked);
	}
/*
	// configure new game menu
	playMenu.pMenu = pMenu->FindChild("playgame");
	if(playMenu.pMenu)
	{
		playMenu.pCreateButton = playMenu.pMenu->FindChild<HKWidgetButton>("create");
		playMenu.pJoinButton = playMenu.pMenu->FindChild<HKWidgetButton>("join");
		playMenu.pOfflineButton = playMenu.pMenu->FindChild<HKWidgetButton>("offline");
		playMenu.pReturnButton = playMenu.pMenu->FindChild<HKWidgetButton>("return");

		playMenu.pCreateButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &GameMenu::PlayMenu::OnCreateClicked);
		playMenu.pJoinButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &GameMenu::PlayMenu::OnJoinClicked);
		playMenu.pOfflineButton->OnClicked += fastdelegate::MakeDelegate(&playMenu, &GameMenu::PlayMenu::OnOfflineClicked);
		playMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &GameMenu::OnReturnClicked);
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
		listMenu.pActiveList->OnSelChanged += fastdelegate::MakeDelegate(&listMenu, &GameMenu::ListMenu::OnSelect);
		listMenu.pName->OnChanged += fastdelegate::MakeDelegate(&listMenu, &GameMenu::ListMenu::OnNameChanged);
		listMenu.pResumeButton->OnClicked += fastdelegate::MakeDelegate(&listMenu, &GameMenu::ListMenu::OnContinueClicked);
		listMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(&listMenu, &GameMenu::ListMenu::OnReturnClicked);
	}

	// configure login window
	loginMenu.pMenu = pMenu->FindChild("login");
	if(loginMenu.pMenu)
	{
		loginMenu.pUsernameText = loginMenu.pMenu->FindChild<HKWidgetTextbox>("username");
		loginMenu.pPasswordText = loginMenu.pMenu->FindChild<HKWidgetTextbox>("password");
		loginMenu.pLoginButton = loginMenu.pMenu->FindChild<HKWidgetButton>("login");
		loginMenu.pReturnButton = loginMenu.pMenu->FindChild<HKWidgetButton>("return");

		loginMenu.pUsernameText->OnChanged += fastdelegate::MakeDelegate(&loginMenu, &GameMenu::LoginMenu::OnUpdateLogin);
		loginMenu.pPasswordText->OnChanged += fastdelegate::MakeDelegate(&loginMenu, &GameMenu::LoginMenu::OnUpdateLogin);
		loginMenu.pLoginButton->OnClicked += fastdelegate::MakeDelegate(&loginMenu, &GameMenu::LoginMenu::OnLogin);
		loginMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &GameMenu::OnReturnClicked);
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

		lobbyMenu.pStartButton->OnClicked += fastdelegate::MakeDelegate(&lobbyMenu, &GameMenu::LobbyMenu::OnStartClicked);
		lobbyMenu.pLeaveButton->OnClicked += fastdelegate::MakeDelegate(&lobbyMenu, &GameMenu::LobbyMenu::OnLeaveClicked);
		lobbyMenu.pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &GameMenu::OnReturnClicked);

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
			lobbyMenu.players[a].pRace->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &GameMenu::LobbyMenu::OnRaceChanged);
			lobbyMenu.players[a].pColour->Bind(lobbyMenu.colourListAdapter);
			lobbyMenu.players[a].pColour->SetUserData((void*)a);
			lobbyMenu.players[a].pColour->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &GameMenu::LobbyMenu::OnColourChanged);
			lobbyMenu.players[a].pHero->Bind(lobbyMenu.players[a].heroListAdapter);
			lobbyMenu.players[a].pHero->SetUserData((void*)a);
			lobbyMenu.players[a].pHero->OnSelChanged += fastdelegate::MakeDelegate(&lobbyMenu, &GameMenu::LobbyMenu::OnHeroChanged);
		}
	}
*/
	// configure profile window


	// and add the menu to the UI manager
	pMenu->SetVisible(HKWidget::Invisible);
	HKUserInterface::Get().AddTopLevelWidget(pMenu, false);
}

GameMenu::~GameMenu()
{
	delete pMenu;
}

void GameMenu::Show()
{
	pMenu->SetVisible(HKWidget::Visible);
}

void GameMenu::Hide()
{
	HideCurrent();
	pMenu->SetVisible(HKWidget::Invisible);
}

void GameMenu::ShowAsCurrent(HKWidget *pMenu)
{
	pMenu->SetVisible(HKWidget::Visible);
	pCurrentWindow = pMenu;
}

void GameMenu::HideCurrent()
{
	if(pCurrentWindow)
	{
		pCurrentWindow->SetVisible(HKWidget::Invisible);
		pCurrentWindow = NULL;
	}
}

// event handlers
void GameMenu::OnCloseClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	GameMenu &menu = *GameMenu::Get();
	menu.HideCurrent();
}

// MainMenu
void GameMenu::GameScreen::Show()
{
	pMenu->SetVisible(HKWidget::Visible);
}

void GameMenu::GameScreen::OnEndTurnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
}

void GameMenu::GameScreen::OnUndoClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
}

void GameMenu::GameScreen::OnMinimapClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
}
