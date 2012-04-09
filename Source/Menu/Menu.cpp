#include "Warlords.h"
#include "UI/HKUI.h"
#include "UI/HKWidgetStyle.h"
#include "UI/HKWidgetLoader-XML.h"

#include "Menu.h"

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
	HKWidgetStyle::LoadStylesFromXML("game-style.xml");

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
	HKWidget *pResume = pMenu->FindChild("resume");
	if(pResume)
		listMenu.Load(pResume, this);

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
	HKWidget *pLobby = pMenu->FindChild("lobby");
	if(pLobby)
		lobbyMenu.Load(pLobby, this);

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
