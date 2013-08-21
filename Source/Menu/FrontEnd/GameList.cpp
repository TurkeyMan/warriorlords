#include "Warlords.h"
#include "Haku/UI/HKUI.h"

#include "Menu/FrontEnd/GameList.h"
#include "Menu/Menu.h"

#include "Editor.h"
#include "Screen.h"

#include "Profile.h"
#include "Lobby.h"

extern Editor *pEditor;

// ListMenu
void ListMenu::Load(HKWidget *pRoot, FrontMenu *pFrontMenu)
{
	pMenu = pRoot;

	if(pMenu)
	{
		pActiveList = pMenu->FindChild<HKWidgetListbox>("activegames");
		pResumeButton = pMenu->FindChild<HKWidgetButton>("resume");
		pReturnButton = pMenu->FindChild<HKWidgetButton>("return");
		pEditButton = pMenu->FindChild<HKWidgetButton>("edit");
		pTitle = pMenu->FindChild<HKWidgetLabel>("resume_label");
		pNamePanel = pMenu->FindChild("gameNamePanel");
		pName = pMenu->FindChild<HKWidgetTextbox>("gameName");

		pActiveList->Bind(gameListAdapter);
		pActiveList->OnSelChanged += fastdelegate::MakeDelegate(this, &ListMenu::OnSelect);
		pName->OnChanged += fastdelegate::MakeDelegate(this, &ListMenu::OnNameChanged);
		pResumeButton->OnClicked += fastdelegate::MakeDelegate(this, &ListMenu::OnContinueClicked);
		pReturnButton->OnClicked += fastdelegate::MakeDelegate(this, &ListMenu::OnReturnClicked);
		pEditButton->OnClicked += fastdelegate::MakeDelegate(this, &ListMenu::OnEditClicked);
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

	// list local games
	MFFindData fd;
	MFFind *pFind = MFFileSystem_FindFirst("home:*.txt", &fd);
	while(pFind)
	{
		fd.pFilename;

		ListItem i;
		i.id = 0;
		i.name = fd.pFilename;
		i.map = "";
		i.numPlayers = 0;
		gameList.push(i);

		if(!MFFileSystem_FindNext(pFind, &fd))
		{
			MFFileSystem_FindClose(pFind);
			pFind = NULL;
		}
	}

	if(Session::Get())
	{
		Profile *pUser = Session::Get()->User();

		// populate game list...
		MFArray<GameState*> &games = pUser->Games();
		for(size_t a=0; a<games.size(); ++a)
		{
			ListItem i;
			i.id = games[a]->ID();
			i.name = MFString::Format("Game %d", games[a]->ID());
			i.map = games[a]->Map().Name();
			i.numPlayers = games[a]->Players().size();

			gameList.push(i);
		}

		MFArray<Lobby*> &pending = pUser->Pending();
		for(size_t a=0; a<pending.size(); ++a)
		{
			ListItem i;
			i.id = pending[a]->ID();
			i.name = MFString::Format("Lobby %d", pending[a]->ID());
			i.map = pending[a]->Map().Name();
			i.numPlayers = pending[a]->NumPlayers();

			gameList.push(i);
		}
	}

	bReturnToMain = true;

	FrontMenu::Get()->ShowAsCurrent(pMenu);
}
/*
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
*/
void ListMenu::ShowCreate(bool bOnline)
{
	type = bOnline ? Create : Offline;

	// set the title
	pTitle->SetText(bOnline ? "Create Game" : "Offline Game");

	// disable resume button
	pResumeButton->SetEnabled(false);

	// show the edit button
	pEditButton->SetVisible(bOnline ? HKWidget::Gone : HKWidget::Visible);

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

void ListMenu::ShowLobby(Lobby *pLobby)
{
	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	menu.lobbyMenu.Show(pLobby);
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
			MapTemplate *pMapTemplate = MapTemplate::Create(map);

			GameCreateDetails details;
			details.pName = pName->GetString().CStr();
			details.pMap = map.CStr();
			details.turnTime = 3600; // TODO: fix me?
			details.numPlayers = pMapTemplate->NumPlayersPresent();

			pMapTemplate->Release();

			ServerRequest *pReq = new ServerRequest(MakeDelegate(this, &ListMenu::OnGameCreated));
			pReq->CreateGame(Session::Get(), &details);
//			pSession->CreateGame(details, MakeDelegate(this, &ListMenu::OnGameCreated));

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
			Lobby *pLobby = new Lobby(map);

			ShowLobby(pLobby);
			break;
		}
/*
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
*/
		case Resume:
		{
			ListItem &item = gameList[pActiveList->GetSelection()];
			uint32 id = item.id;

			if(id == 0)
			{
				MFDebug_Assert(false, "!");
//				GameState *pState = GameState::ResumeGame(item.name.CStr(), false);
//				Game *pGame = new Game(*pState);

				FrontMenu::Get()->Hide();
			}
			else
			{
				GameState *pGame = GameState::Get(id);
				if(pGame)
				{
					MFDebug_Assert(false, "!");
//					GameState *pState = GameState::ResumeGame(NULL, true);
//					Game *pGame = new Game(*pState);

					FrontMenu::Get()->Hide();
					return;
				}

				Lobby *pLobby = Lobby::Get(id);
				if(pLobby)
				{
					ShowLobby(pLobby);
					return;
				}
			}
			break;
		}

		default:
			break;
	}
}

void ListMenu::OnGameCreated(ServerRequest *pReq)
{
	// enable all the UI bits again
	pActiveList->SetEnabled(true);
	pName->SetEnabled(true);
	pResumeButton->SetEnabled(true);
	pReturnButton->SetEnabled(true);

	// check the pSession has this stuff remembered somehow...
	if(pReq->Status() == ServerRequest::SE_NO_ERROR)
	{
		Lobby *pLobby = Lobby::FromJson(pReq->Json());

		if(pLobby)
		{
			// TODO: we should add it to the user's profile

			ShowLobby(pLobby);
		}
	}

	delete pReq;
}
/*
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
*/
void ListMenu::OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	pEditButton->SetVisible(HKWidget::Gone);

	FrontMenu &menu = *FrontMenu::Get();
	menu.HideCurrent();

	if(bReturnToMain)
		menu.ShowMainMenu();
	else
		menu.playMenu.Show();
}

void ListMenu::OnEditClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	MFString map = gameList[pActiveList->GetSelection()].name;

	// create the editor
	pEditor = new Editor(map);
	Screen::SetNext(pEditor);

	// hide the menu
	FrontMenu::Get()->Hide();
}

/*
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
*/
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
