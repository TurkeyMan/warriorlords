#include "Warlords.h"
#include "Group.h"
#include "Haku/UI/HKUI.h"
#include "Haku/UI/HKWidgetLoader-XML.h"
#include "Haku/UI/Widgets/HKWidgetButton.h"
#include "Haku/UI/Widgets/HKWidgetTextbox.h"
#include "Haku/UI/Widgets/HKWidgetListbox.h"
#include "Menu/Widgets/UnitButton.h"

#include "GameUI.h"

#pragma warning(disable: 4355)

GameUI::GameUI(Game *pGame)
: gameScreen(this)
, castleMenu(this)
, recruitMenu(this)
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
		castleMenu.pCastleName = castleMenu.pMenu->FindChild<HKWidgetLabel>("castleName");

		castleMenu.pClose = castleMenu.pMenu->FindChild<HKWidgetButton>("close");
		castleMenu.pClose->OnClicked += fastdelegate::MakeDelegate(this, &GameUI::OnCloseClicked);

		for(int a=0; a<4; ++a)
		{
			castleMenu.pUnits[a] = castleMenu.pMenu->FindChild<UnitButton>(MFString::Format("unit%d", a).CStr());
			castleMenu.pUnits[a]->SetUserData((void*)a);
			castleMenu.pUnits[a]->OnClicked += fastdelegate::MakeDelegate(&castleMenu, &GameUI::CastleMenu::OnSelectUnit);

			castleMenu.pUnits[a + 4] = castleMenu.pMenu->FindChild<UnitButton>(MFString::Format("hero%d", a).CStr());
			castleMenu.pUnits[a + 4]->SetUserData((void*)(a + 4));
			castleMenu.pUnits[a + 4]->OnClicked += fastdelegate::MakeDelegate(&castleMenu, &GameUI::CastleMenu::OnSelectUnit);
		}

		castleMenu.pUnitDetails = castleMenu.pMenu->FindChild<HKWidgetLayoutFrame>("unitDetails");

		castleMenu.pUnitName = castleMenu.pMenu->FindChild<HKWidgetLabel>("unitName");
		castleMenu.pUnitType = castleMenu.pMenu->FindChild<HKWidgetLabel>("unitType");
		castleMenu.pUnitAtk = castleMenu.pMenu->FindChild<HKWidgetLabel>("unitAtk");
		castleMenu.pUnitMov = castleMenu.pMenu->FindChild<HKWidgetLabel>("unitMov");
		castleMenu.pUnitTurns = castleMenu.pMenu->FindChild<HKWidgetLabel>("unitTurns");

		castleMenu.pTypeImage = castleMenu.pMenu->FindChild("typeImage");
	}

	// configure recruit menu
	recruitMenu.pMenu = pMenu->FindChild("recruit");
	if(recruitMenu.pMenu)
	{
		recruitMenu.pClose = recruitMenu.pMenu->FindChild<HKWidgetButton>("close");
		recruitMenu.pClose->OnClicked += fastdelegate::MakeDelegate(this, &GameUI::OnCloseClicked);

		for(int a=0; a<8; ++a)
		{
			recruitMenu.pHeroes[a] = recruitMenu.pMenu->FindChild<UnitButton>(MFString::Format("hero%d", a).CStr());
			recruitMenu.pHeroes[a]->SetUserData((void*)a);
			recruitMenu.pHeroes[a]->OnClicked += fastdelegate::MakeDelegate(&recruitMenu, &GameUI::RecruitMenu::OnSelectHero);
		}

		recruitMenu.pHeroDetails = recruitMenu.pMenu->FindChild<HKWidgetLayoutFrame>("heroDetails");

		recruitMenu.pHeroName = recruitMenu.pMenu->FindChild<HKWidgetLabel>("heroName");
		recruitMenu.pHeroAtk = recruitMenu.pMenu->FindChild<HKWidgetLabel>("heroAtk");
		recruitMenu.pHeroMov = recruitMenu.pMenu->FindChild<HKWidgetLabel>("heroMov");
		recruitMenu.pHeroTurns = recruitMenu.pMenu->FindChild<HKWidgetLabel>("heroTurns");

		recruitMenu.pTypeImage = recruitMenu.pMenu->FindChild("typeImage");
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

void GameUI::ShowRecruitMenu(Place *pPlace, Unit *pHero)
{
	recruitMenu.Show(pPlace, pHero);
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
	if(pUI->pGame->IsMyTurn())
		pUI->pGame->ShowRequest("End Turn?", fastdelegate::MakeDelegate(this, &GameUI::GameScreen::OnFinishTurn), false);
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
		pGame->mapView.CenterView((float)pTile->GetX(), (float)pTile->GetY());
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

// castle menu

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
		bool bBuildHero = pGame->State().CanCastleBuildHero(pCastle, a);
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
		const UnitDefinitions &unitDefs = pCastle->UnitDefs();
		const UnitDetails &unit = unitDefs.GetUnitDetails(buildUnit);

		pUnitName->SetText(unit.name);

		if(unit.type == UT_Vehicle)
		{
			pUnitType->SetVisible(HKWidget::Gone);
			pUnitAtk->SetVisible(HKWidget::Gone);
			pUnitMov->SetText(MFString::Format("Mov: %d%s", unit.movement, unit.movementClass > 0 ? MFStr(" (%s)", unitDefs.GetMovementClassName(unit.movementClass).CStr()) : ""));
			pUnitTurns->SetText(MFString::Format("Turns: %d", pCastle->BuildTimeRemaining()));

			pTypeImage->SetVisible(HKWidget::Gone);
		}
		else
		{
			pUnitType->SetVisible(HKWidget::Visible);
			pUnitAtk->SetVisible(HKWidget::Visible);
			pUnitType->SetText(MFString::Format("Type: %s", unitDefs.GetArmourClassName(unit.armour).CStr()));
			pUnitAtk->SetText(MFString::Format("Atk: %d - %d (%s%s)", unit.attackMin, unit.attackMax, unit.AttackSpeedDescription(), unitDefs.GetAttackTypeName(unit.atkType).CStr()));
			pUnitMov->SetText(MFString::Format("Mov: %d%s", unit.movement, unit.movementClass > 0 ? MFStr(" (%s)", unitDefs.GetMovementClassName(unit.movementClass).CStr()) : ""));
			pUnitTurns->SetText(MFString::Format("Turns: %d", pCastle->BuildTimeRemaining()));

			pTypeImage->SetVisible(HKWidget::Visible);
			pTypeImage->SetProperty("background_image", unit.atkType == 0 ? "Melee" : "Ranged");
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

// recruit menu

void GameUI::RecruitMenu::Show(Place *pPlace, Unit *pHero)
{
	Game *pGame = pUI->pGame;

	this->pPlace = pPlace;

	int player = pGame->CurrentPlayer();
	MFVector colour = pGame->GetPlayerColour(player);

	if(pPlace->recruit.recruiting < 0 || pPlace->recruit.pRecruitHero != pHero)
	{
		pPlace->recruit.pRecruitHero = pHero;
		pPlace->recruit.recruiting = -1;
		pPlace->recruit.turnsRemaining = 0;
	}

	const UnitDefinitions &unitDefs = pGame->Map().UnitDefs();

	int numUnits = unitDefs.GetNumUnitTypes();
	int numHeroes = 0;
	for(int a=0; a<numUnits; ++a)
	{
		const UnitDetails &unit = unitDefs.GetUnitDetails(a);

		if(unit.type == UT_Hero && (!pGame->PlayerHasHero(player, a) && (unit.race == 0 || unit.race == pGame->GetPlayerRace(player))))
		{
			heroes[numHeroes] = a;

			pHeroes[numHeroes]->SetUnit(a);
			pHeroes[numHeroes]->SetUnitColour(colour);

			pHeroes[numHeroes]->SetProperty("background_colour", pPlace->recruit.recruiting == a ? "blue" : "white");

			pHeroes[numHeroes]->SetVisible(HKWidget::Visible);
			++numHeroes;
		}
	}

	for(int a=numHeroes; a<8; ++a)
		pHeroes[a]->SetVisible(HKWidget::Gone);

	UpdateHeroInfo();

	pUI->ShowAsCurrent(this);
}

void GameUI::RecruitMenu::UpdateHeroInfo()
{
	int selected = pPlace->recruit.recruiting;

	pHeroDetails->SetVisible(selected >= 0 ? HKWidget::Visible : HKWidget::Invisible);

	if(selected < 0)
		return;

	const UnitDefinitions &unitDefs = pUI->pGame->Map().UnitDefs();
	const UnitDetails &unit = unitDefs.GetUnitDetails(selected);

	pHeroName->SetText(unit.name);
	pHeroAtk->SetText(MFString::Format("Atk: %d - %d (%s%s)", unit.attackMin, unit.attackMax, unit.AttackSpeedDescription(), unitDefs.GetAttackTypeName(unit.atkType)));
	pHeroMov->SetText(MFString::Format("Mov: %d%s", unit.movement, unit.movementClass > 0 ? MFStr(" (%s)", unitDefs.GetMovementClassName(unit.movementClass)) : ""));
	pHeroTurns->SetText(MFString::Format("Turns: %d", unit.buildTime));

	pTypeImage->SetProperty("background_image", unit.atkType == 0 ? "Melee" : "Ranged");
}

void GameUI::RecruitMenu::OnSelectHero(HKWidget &sender, const HKWidgetEventInfo &ev)
{
	Game *pGame = pUI->pGame;

	int button = (int)(size_t)sender.GetUserData();

//	MFVector colour = pGame->GetPlayerColour(pGame->CurrentPlayer());
	for(int a=0; a<8; ++a)
		pHeroes[a]->SetProperty("background_colour", button == a ? "blue" : "white");

	// mark current selection
	pPlace->recruit.recruiting = heroes[button];
	pPlace->recruit.turnsRemaining = 2;

	UpdateHeroInfo();
}

// minimap
void GameUI::MiniMap::Show()
{
	int width, height;
	pUI->pGame->mapView.GetMinimap(&width, &height);

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

// message box

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
