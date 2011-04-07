#include "Warlords.h"
#include "Display.h"
#include "HomeScreen.h"
#include "MenuScreen.h"
#include "LobbyScreen.h"
#include "JoinGameScreen.h"

#include "MFPrimitive.h"
#include "MFMaterial.h"
#include "MFRenderer.h"
#include "MFFileSystem.h"

extern MenuScreen *pMenu;
extern LobbyScreen *pLobby;
extern JoinGameScreen *pJoinGame;

extern Game *pGame;

HomeScreen::HomeScreen()
{
	pIcons = MFMaterial_Create("Icons");
	pFont = MFFont_Create("FranklinGothic");

	request.SetCompleteDelegate(MakeDelegate(this, &HomeScreen::Resume));

	pSession = NULL;
	bOffline = true;

	// populate map list
	MFRect rect = { 10.f, 64.f, 240.f, 200.f };
	pWaiting = ListBox::Create(&rect, pFont);
	pWaiting->SetSelectCallback(MakeDelegate(this, &HomeScreen::SelectPending));
	pWaiting->SetDblClickCallback(MakeDelegate(this, &HomeScreen::EnterPending));

	rect.x += 260.f;
	pMyGames = ListBox::Create(&rect, pFont);
	pMyGames->SetSelectCallback(MakeDelegate(this, &HomeScreen::SelectGame));
	pMyGames->SetDblClickCallback(MakeDelegate(this, &HomeScreen::EnterGame));

	// start buttons
	MFRect uvs, pos = { 0, 0, 64.f, 64.f };
	float texelCenterOffset = MFRenderer_GetTexelCenterOffset() / 256.f;

	// create game button
	MFRect display;
	GetDisplayRect(&display);
	pos.x = 16.f;
	pos.y = 32.f + 256.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	uvs.width = 0.25f; uvs.height = 0.25f;
	pCreate = Button::Create(pIcons, &pos, &uvs, MFVector::one, 0, false);
	pCreate->SetClickCallback(MakeDelegate(this, &HomeScreen::CreateGame));

	// join game button
	pos.x = 32.f + 64.f;
	uvs.x = 0.5f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pJoin = Button::Create(pIcons, &pos, &uvs, MFVector::one, 2, false);
	pJoin->SetClickCallback(MakeDelegate(this, &HomeScreen::JoinGame));

	// find game button
	pos.x = 48.f + 128.f;
	uvs.x = 0.75f + texelCenterOffset; uvs.y = 0.f + texelCenterOffset;
	pFind = Button::Create(pIcons, &pos, &uvs, MFVector::one, 3, false);
	pFind->SetClickCallback(MakeDelegate(this, &HomeScreen::FindGame));

	// offline game button
	pos.x = 64.f + 192.f;
	uvs.x = 0.f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pOffline = Button::Create(pIcons, &pos, &uvs, MFVector::one, 1, false);
	pOffline->SetClickCallback(MakeDelegate(this, &HomeScreen::CreateGame));

	// continue game button
	pos.x = 80.f + 256.f;
	uvs.x = 0.25f + texelCenterOffset; uvs.y = 0.75f + texelCenterOffset;
	pContinue = Button::Create(pIcons, &pos, &uvs, MFVector::one, 4, false);
	pContinue->SetClickCallback(MakeDelegate(this, &HomeScreen::ContinueGame));
}

HomeScreen::~HomeScreen()
{
	pWaiting->Destroy();
	pMyGames->Destroy();

	pCreate->Destroy();
	pJoin->Destroy();
	pFind->Destroy();
	pOffline->Destroy();
	pContinue->Destroy();

	MFMaterial_Destroy(pIcons);
	MFFont_Destroy(pFont);
}

void HomeScreen::Select()
{
	pSession = Session::Get();
	bOffline = pSession == NULL || !pSession->IsLoggedIn();

	pWaiting->Clear();
	pMyGames->Clear();

	if(!bOffline)
	{
		pSession->SetUpdateDelegate(MakeDelegate(this, &HomeScreen::UpdateSession));
		pSession->UpdateState();
	}

	pInputManager->PushReceiver(this);
	pInputManager->PushReceiver(pWaiting);
	pInputManager->PushReceiver(pMyGames);
	pInputManager->PushReceiver(pCreate);
	pInputManager->PushReceiver(pJoin);
	pInputManager->PushReceiver(pFind);
	pInputManager->PushReceiver(pOffline);
	pInputManager->PushReceiver(pContinue);

	pCreate->Enable(!bOffline);
	pJoin->Enable(!bOffline);
	pFind->Enable(!bOffline);

	pWaiting->SetSelection(-1);
	pMyGames->SetSelection(-1);
	pContinue->Enable(false);
}

bool HomeScreen::HandleInputEvent(InputEvent ev, InputInfo &info)
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

int HomeScreen::Update()
{
	return 0;
}

void HomeScreen::Draw()
{
	if(bOffline)
		MFFont_DrawText(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Offline");
	else
		MFFont_DrawTextf(pFont, MakeVector(16, 16), 32.f, MFVector::yellow, "Welcome %s!", pSession->GetUsername());

	pWaiting->Draw();
	pMyGames->Draw();
	pCreate->Draw();
	pJoin->Draw();
	pFind->Draw();
	pOffline->Draw();
	pContinue->Draw();

	DrawTicker(10, 0);
}

void HomeScreen::Deselect()
{
	pInputManager->PopReceiver(this);
}

void HomeScreen::ResumeGame(uint32 game)
{
	WLServ_GameState(request, game);
}

void HomeScreen::CreateGame(int button, int buttonID)
{
	// go to game create screen
	pMenu->SetGameType(buttonID);
	Screen::SetNext(pMenu);
}

void HomeScreen::JoinGame(int button, int buttonID)
{
	Screen::SetNext(pJoinGame);
}

void HomeScreen::FindGame(int button, int buttonID)
{
}

void HomeScreen::ContinueGame(int button, int buttonID)
{
	ResumeGame(continueGame);
}

void HomeScreen::SelectPending(int item)
{
	pMyGames->SetSelection(-1);

	if(item < 0)
	{
		pContinue->Enable(false);
		return;
	}

	// select game
	GameDetails *pGame = (GameDetails*)pWaiting->GetItemData(item);
	continueGame = pGame->id;

	pContinue->Enable(true);
}

void HomeScreen::SelectGame(int item)
{
	pWaiting->SetSelection(-1);

	if(item < 0)
	{
		pContinue->Enable(false);
		return;
	}

	// select game
	GameDetails *pGame = (GameDetails*)pMyGames->GetItemData(item);
	continueGame = pGame->id;

	pContinue->Enable(true);
}

void HomeScreen::EnterPending(int item)
{
	if(item < 0)
		return;

	GameDetails *pGame = (GameDetails*)pWaiting->GetItemData(item);
	ResumeGame(pGame->id);
}

void HomeScreen::EnterGame(int item)
{
	if(item < 0)
		return;

	GameDetails *pGame = (GameDetails*)pMyGames->GetItemData(item);
	ResumeGame(pGame->id);
}

void HomeScreen::Resume(HTTPRequest::Status status)
{
	GameState state;
	ServerError err = WLServResult_GetGameState(request, &state);

	if(err == SE_NO_ERROR)
	{
		// resume game
		pGame = new Game(&state);
		Game::SetCurrent(pGame);
		return;
	}

	// enter lobby if it still exists
	if(pLobby->ShowOnline(continueGame))
		return;

	// game has disappeared >_<
	//... error string?
}

void HomeScreen::UpdateSession(ServerError error, Session *pSession)
{
	for(int a=0; a<pSession->GetNumPendingGames(); ++a)
	{
		GameDetails *pGame = pSession->GetPendingGame(a);
		pWaiting->AddItem(pGame->name, -1, pGame);
	}

	for(int a=0; a<pSession->GetNumCurrentGames(); ++a)
	{
		GameState *pGame = pSession->GetCurrentGame(a);
		pMyGames->AddItem(pGame->name, -1, pGame);
	}
}
