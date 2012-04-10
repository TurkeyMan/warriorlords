#include "Warlords.h"
#include "UI/HKUI.h"
#include "UI/HKWidgetLoader-XML.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"
#include "Menu/Widgets/UnitButton.h"

#include "GameUI.h"

#pragma warning(disable: 4355)

GameUI::GameUI(Game *pGame)
: gameScreen(this)
, castleMenu(this)
, miniMap(this)
, msgBox(this)
{
	this->pGame = pGame;
	pShowCastle = NULL;

	// init members
	stackDepth = 0;

	// load the menu
	pMenu = HKWidget_CreateFromXML("game.xml");
	MFDebug_Assert(pMenu, "Failed to load front menu UI!");

	pWindowContainer = pMenu->FindChild("windows");
	pWindowContainer->RegisterInputEventHook(fastdelegate::MakeDelegate(this, &GameUI::NullEvent));

	// configure map icons
	gameScreen.pMenu = pMenu->FindChild("map");
	if(gameScreen.pMenu)
	{
		gameScreen.pMap = gameScreen.pMenu->FindChild("map");
		pGame->SetInputSource(gameScreen.pMap);

		gameScreen.pUndoButton = gameScreen.pMenu->FindChild<HKWidgetButton>("undo");
		gameScreen.pEndTurnButton = gameScreen.pMenu->FindChild<HKWidgetButton>("end_turn");
		gameScreen.pMinimapButton = gameScreen.pMenu->FindChild<HKWidgetButton>("show_minimap");

		gameScreen.pUndoButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameUI::GameScreen::OnUndoClicked);
		gameScreen.pEndTurnButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameUI::GameScreen::OnEndTurnClicked);
		gameScreen.pMinimapButton->OnClicked += fastdelegate::MakeDelegate(&gameScreen, &GameUI::GameScreen::OnMinimapClicked);
	}

	// configure castle menu
	castleMenu.pMenu = pMenu->FindChild("castle");
	if(castleMenu.pMenu)
	{
		castleMenu.pCastleName = pMenu->FindChild<HKWidgetLabel>("castleName");

		castleMenu.pClose = pMenu->FindChild<HKWidgetButton>("close");
		castleMenu.pClose->OnClicked += fastdelegate::MakeDelegate(this, &GameUI::OnCloseClicked);

		for(int a=0; a<4; ++a)
		{
			castleMenu.pUnits[a] = pMenu->FindChild<UnitButton>(MFString::Format("unit%d", a).CStr());
			castleMenu.pUnits[a]->SetUserData((void*)a);
			castleMenu.pUnits[a]->OnClicked += fastdelegate::MakeDelegate(&castleMenu, &GameUI::CastleMenu::OnSelectUnit);

			castleMenu.pUnits[a + 4] = pMenu->FindChild<UnitButton>(MFString::Format("hero%d", a).CStr());
			castleMenu.pUnits[a + 4]->SetUserData((void*)(a + 4));
			castleMenu.pUnits[a + 4]->OnClicked += fastdelegate::MakeDelegate(&castleMenu, &GameUI::CastleMenu::OnSelectUnit);
		}

		castleMenu.pUnitDetails = pMenu->FindChild<HKWidgetLayoutFrame>("unitDetails");

		castleMenu.pUnitName = pMenu->FindChild<HKWidgetLabel>("unitName");
		castleMenu.pUnitType = pMenu->FindChild<HKWidgetLabel>("unitType");
		castleMenu.pUnitAtk = pMenu->FindChild<HKWidgetLabel>("unitAtk");
		castleMenu.pUnitMov = pMenu->FindChild<HKWidgetLabel>("unitMov");
		castleMenu.pUnitTurns = pMenu->FindChild<HKWidgetLabel>("unitTurns");

		castleMenu.pTypeImage = pMenu->FindChild("typeImage");
	}

	// configure minimap
	miniMap.pMenu = pMenu->FindChild("minimap");
	if(miniMap.pMenu)
	{
		miniMap.pMap = miniMap.pMenu->FindChild("map");
		miniMap.pMap->OnDown += fastdelegate::MakeDelegate(&miniMap, &GameUI::MiniMap::OnFocusMap);
		miniMap.pMap->OnDrag += fastdelegate::MakeDelegate(&miniMap, &GameUI::MiniMap::OnFocusMap);
		miniMap.pMap->SetClickable(true);
		miniMap.pMap->SetHoverable(true);

		miniMap.pClose = miniMap.pMenu->FindChild<HKWidgetButton>("close");

		// subscribe events
		miniMap.pClose->OnClicked += fastdelegate::MakeDelegate(this, &GameUI::OnCloseClicked);
	}

	// configure message box
	msgBox.pMenu = pMenu->FindChild("requestbox");
	if(msgBox.pMenu)
	{
		msgBox.pMessage = msgBox.pMenu->FindChild<HKWidgetLabel>("message");
		msgBox.pAccept = msgBox.pMenu->FindChild<HKWidgetButton>("accept");
		msgBox.pCancel = msgBox.pMenu->FindChild<HKWidgetButton>("cancel");

		// subscribe events
		msgBox.pAccept->OnClicked += fastdelegate::MakeDelegate(&msgBox, &GameUI::MsgBox::OnAcceptClicked);
		msgBox.pCancel->OnClicked += fastdelegate::MakeDelegate(&msgBox, &GameUI::MsgBox::OnCancelClicked);
	}

	// and add the menu to the UI manager
	pMenu->SetVisible(HKWidget::Invisible);
	HKUserInterface::Get().AddTopLevelWidget(pMenu, false);
}

GameUI::~GameUI()
{
	delete pMenu;
}

void GameUI::Update()
{
	if(pShowCastle)
	{
		castleMenu.Show(pShowCastle);
		pShowCastle = NULL;
	}
}

void GameUI::Show()
{
	pMenu->SetVisible(HKWidget::Visible);
}

void GameUI::Hide()
{
	HideCurrent();
	pMenu->SetVisible(HKWidget::Invisible);
}

void GameUI::ShowAsCurrent(Window *pMenu, bool bShowAbove)
{
	if(stackDepth == 0)
		pWindowContainer->SetVisible(HKWidget::Visible);

	// if we don't want to show it above prior windows
	if(!bShowAbove && stackDepth > 0)
		pCurrentWindowStack[stackDepth - 1]->GetMenu()->SetVisible(HKWidget::Invisible);

	pCurrentWindowStack[stackDepth++] = pMenu;
	pMenu->GetMenu()->SetVisible(HKWidget::Visible);
}

void GameUI::HideCurrent()
{
	if(stackDepth == 0)
		return;

	pCurrentWindowStack[--stackDepth]->Hide();

	if(stackDepth > 0)
		pCurrentWindowStack[stackDepth - 1]->GetMenu()->SetVisible(HKWidget::Visible);
	else
		pWindowContainer->SetVisible(HKWidget::Invisible);
}

void GameUI::ShowMiniMap()
{
	miniMap.Show();
}

void GameUI::ShowMessageBox(const char *pMessage, MsgBoxDelegate selectCallback, bool bNotification)
{
	msgBox.Show(pMessage, selectCallback, bNotification);
}

void GameUI::ShowUndoButton(bool bShow)
{
	gameScreen.pUndoButton->SetVisible(bShow ? HKWidget::Visible : HKWidget::Invisible);
}

void GameUI::ShowWindowContainer(bool bShow)
{
	if(bShow)
		pWindowContainer->SetVisible(HKWidget::Visible);
	else
		pWindowContainer->SetVisible(HKWidget::Invisible);
}

void GameUI::Window::Hide()
{
	pMenu->SetVisible(HKWidget::Invisible);
}

// event handlers
bool GameUI::NullEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev)
{
	if(ev.pSource->device == IDD_Mouse && ev.pSource->deviceID != 0)
		return false;

	// HACK: right click returns
	if(ev.buttonID == 1 && ev.ev == HKInputManager::IE_Tap)
		HideCurrent();

	// return handled
	return true;
}

void GameUI::OnCloseClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	HideCurrent();
}

// map icons
void GameUI::GameScreen::Show()
{
	pMenu->SetVisible(HKWidget::Visible);

}

void GameUI::GameScreen::OnEndTurnClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Game *pGame = Game::GetCurrent();

	if(pGame->NumPendingActions() > 0)
		pGame->ReplayNextAction();
	else if(pGame->IsMyTurn())
		pGame->ShowRequest("End Turn?", fastdelegate::MakeDelegate(this, &GameUI::GameScreen::OnFinishTurn), false);
}

void GameUI::GameScreen::OnUndoClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Game *pGame = pUI->pGame;

	if(pGame->bMoving)
		return;

	Group *pGroup = pGame->RevertAction(pGame->pSelection);
	if(pGroup)
	{
		MapTile *pTile = pGroup->GetTile();
		pGame->pMap->CenterView((float)pTile->GetX(), (float)pTile->GetY());
	}

	pGame->SelectGroup(pGroup);
	pGame->UpdateUndoButton();
}

void GameUI::GameScreen::OnMinimapClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	pUI->miniMap.Show();
}

void GameUI::GameScreen::OnFinishTurn(int selection)
{
	Game *pGame = pUI->pGame;

	if(selection == 0)
		pGame->EndTurn();
}

// castle Menu

void GameUI::CastleMenu::Show(Castle *_pCastle)
{
	Game *pGame = pUI->pGame;

	pCastle = _pCastle;

	pCastleName->SetText(pCastle->GetName());

	numBuildUnits = pCastle->details.numBuildUnits;

	MFVector colour = pGame->GetPlayerColour(pCastle->player);

	int a;
	for(a=0; a<4; ++a)
	{
		// unit buttons
		pUnits[a]->SetVisible(a < numBuildUnits ? HKWidget::Visible : HKWidget::Gone);

		if(a < numBuildUnits)
		{
			pUnits[a]->SetUnit(pCastle->details.buildUnits[a].unit);
			pUnits[a]->SetUnitColour(colour);

			pUnits[a]->SetProperty("background_colour", a == pCastle->nextBuild ? "blue" : "white");
		}

		// hero buttons
		bool bBuildHero = pGame->CanCastleBuildHero(pCastle, a);
		pUnits[4 + a]->SetVisible(bBuildHero ? HKWidget::Visible : HKWidget::Gone);

		if(bBuildHero)
		{
			Unit *pHero = pGame->GetPlayerHero(pCastle->player, a);
			pUnits[4 + a]->SetUnit(pHero->GetType());
			pUnits[4 + a]->SetUnitColour(colour);

			pUnits[4 + a]->SetProperty("background_colour", 4 + a == pCastle->nextBuild ? "blue" : "white");
		}
	}

	UpdateUnitInfo();

	pUI->ShowAsCurrent(this);
}

void GameUI::CastleMenu::UpdateUnitInfo()
{
	int buildUnit = pCastle->GetBuildUnit();

	pUnitDetails->SetVisible(buildUnit < 0 ? HKWidget::Invisible : HKWidget::Visible);

	if(buildUnit >= 0)
	{
		UnitDefinitions *pUnitDefs = pCastle->pUnitDefs;
		UnitDetails *pDetails = pUnitDefs->GetUnitDetails(buildUnit);

		pUnitName->SetText(pDetails->pName);

		if(pDetails->type == UT_Vehicle)
		{
			pUnitType->SetVisible(HKWidget::Gone);
			pUnitAtk->SetVisible(HKWidget::Gone);
			pUnitMov->SetText(MFString::Format("Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : ""));
			pUnitTurns->SetText(MFString::Format("Turns: %d", pCastle->BuildTimeRemaining()));

			pTypeImage->SetVisible(HKWidget::Gone);
		}
		else
		{
			pUnitType->SetVisible(HKWidget::Visible);
			pUnitAtk->SetVisible(HKWidget::Visible);
			pUnitType->SetText(MFString::Format("Type: %s", pUnitDefs->GetArmourClassName(pDetails->armour)));
			pUnitAtk->SetText(MFString::Format("Atk: %d - %d (%s%s)", pDetails->attackMin, pDetails->attackMax, pDetails->AttackSpeedDescription(), pUnitDefs->GetAttackTypeName(pDetails->atkType)));
			pUnitMov->SetText(MFString::Format("Mov: %d%s", pDetails->movement, pDetails->movementClass > 0 ? MFStr(" (%s)", pUnitDefs->GetMovementClassName(pDetails->movementClass)) : ""));
			pUnitTurns->SetText(MFString::Format("Turns: %d", pCastle->BuildTimeRemaining()));

			pTypeImage->SetVisible(HKWidget::Visible);
			pTypeImage->SetProperty("background_image", pDetails->atkType == 0 ? "Melee" : "Ranged");
		}
	}
}

void GameUI::CastleMenu::OnSelectUnit(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Game *pGame = pUI->pGame;

	int button = (int)(size_t)sender.GetUserData();

	MFVector colour = pGame->GetPlayerColour(pCastle->player);
	for(int a=0; a<8; ++a)
		pUnits[a]->SetProperty("background_colour", button == a ? "blue" : "white");

	pCastle->SetBuildUnit(button);

	UpdateUnitInfo();
}

void GameUI::MiniMap::Show()
{
	int width, height;
	pUI->pGame->GetMap()->GetMinimap(&width, &height);

	pMap->SetSize(MakeVector((float)width, (float)height));
	pMap->SetProperty("background_image", "MiniMap");

	pUI->ShowAsCurrent(this);

/*
	MFMaterial_SetMaterial(pMiniMap);

	float tx = (float)MFUtil_NextPowerOf2(width);
	float ty = (float)MFUtil_NextPowerOf2(height);

	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float texelCenterX = texelCenter / tx;
	float texelCenterY = texelCenter / ty;
*/
}

void GameUI::MiniMap::OnFocusMap(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	int x = 0;

	// on the down-signal set bDragging, and then track hover messages...
/*
				int mapWidth, mapHeight;
				pMap->GetMapSize(&mapWidth, &mapHeight);

				float x = (info.down.x - window.x) / width * (float)mapWidth;
				float y = (info.down.y - window.y) / height * (float)mapHeight;

				pMap->CenterView(x, y);
*/
}

void GameUI::MsgBox::Show(const char *pMessage, MsgBoxDelegate selectCallback, bool bNotification)
{
	this->selectCallback = selectCallback;

	this->pMessage->SetText(pMessage);
	pCancel->SetVisible(bNotification ? HKWidget::Invisible : HKWidget::Visible);

	pUI->ShowAsCurrent(this);
}

void GameUI::MsgBox::OnAcceptClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(selectCallback)
		selectCallback(0);

	pUI->HideCurrent();
}

void GameUI::MsgBox::OnCancelClicked(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	if(selectCallback)
		selectCallback(-1);

	pUI->HideCurrent();
}
