#include "Warlords.h"
#include "UI/HKUI.h"
#include "UI/HKWidgetLoader-XML.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"

#include "Menu.h"
#include "Session.h"

void HideMenu(HKWidget &w, const HKWidgetEventInfo &)
{
	w.GetParent()->SetVisible(HKWidget::Invisible);
}

extern FrontMenu *pFrontMenu;
FrontMenu *FrontMenu::Get()
{
	return pFrontMenu;
}

FrontMenu::FrontMenu()
{
	// init members
	pCurrentWindow = NULL;

	// load the menu
	pMenu = HKWidget_CreateFromXML("menu.xml");
	MFDebug_Assert(pMenu, "Failed to load front menu UI!");

	// *** TEMP - hide button ***
	HKWidgetButton *pHideButton = (HKWidgetButton*)pMenu->FindChild("hide");
	pHideButton->OnClicked += HideMenu;

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

	// configure create game window

	// configure find game window

	// configure lobby window

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
	switch(type)
	{
		case Join:
			break;
		case Create:
			break;
		case Offline:
			break;
		case Resume:
			break;
		default:
			break;
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
	pLabel->SetTextColour(MFVector::white);
	return pLabel;
}

void FrontMenu::GameListAdapter::UpdateItemView(int index, FrontMenu::ListItem &item, HKWidget *pLayout)
{
	HKWidgetLabel *pLabel = (HKWidgetLabel*)pLayout;
	pLabel->SetText(item.name);
}
