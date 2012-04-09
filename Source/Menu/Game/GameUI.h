#if !defined(_GAMEMENU_H)
#define _GAMEMENU_H

#include "Game.h"

#include "UI/HKWidget.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"
#include "UI/Widgets/HKWidgetSelectbox.h"
#include "Menu/Widgets/UnitButton.h"

class GameUI
{
public:
	typedef FastDelegate1<int> MsgBoxDelegate;

	class Window
	{
	public:
		virtual void Hide();

		HKWidget *GetMenu() { return pMenu; }

		HKWidget *pMenu;
	};

	GameUI(Game *pGame);
	~GameUI();

	void Update();

	void Show();
	void Hide();

	HKWidget *GetMap() { return gameScreen.pMap; }

	void ShowAsCurrent(Window *pMenu, bool bShowAbove = false);
	void HideCurrent();

	void ShowGameScreen() { gameScreen.Show(); }

	void ShowCastleMenu(Castle *pCastle) { pShowCastle = pCastle; }
	void ShowMiniMap();

	void ShowMessageBox(const char *pMessage, MsgBoxDelegate selectCallback, bool bNotification);

	void ShowUndoButton(bool bShow);
	void ShowWindowContainer(bool bShow);

protected:
	HKWidget *pMenu;

	HKWidget *pWindowContainer;

	Window *pCurrentWindowStack[16];
	int stackDepth;

	bool NullEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);
	void OnCloseClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

	// game screen
	struct GameScreen
	{
		GameScreen(GameUI *pUI) : pUI(pUI) {}
		GameUI *pUI;

		void Show();

		HKWidget *pMenu;

		HKWidget *pMap;

		HKWidgetButton *pUndoButton;
		HKWidgetButton *pEndTurnButton;
		HKWidgetButton *pMinimapButton;

		void OnUndoClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnMinimapClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnEndTurnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnFinishTurn(int selection);
	} gameScreen;

	// battle screen
	struct BattleScreen
	{
		void Show();

		HKWidget *pMenu;

	} battleScreen;

	// castle config
	struct CastleMenu : public Window
	{
		CastleMenu(GameUI *pUI) : pUI(pUI) {}
		GameUI *pUI;

		void Show(Castle *pCastle);

		Castle *pCastle;

		HKWidgetLabel *pCastleName;
		HKWidgetButton *pClose;
		UnitButton *pUnits[8];

		HKWidgetLayoutFrame *pUnitDetails;
		HKWidgetLabel *pUnitName;
		HKWidgetLabel *pUnitType;
		HKWidgetLabel *pUnitAtk;
		HKWidgetLabel *pUnitMov;
		HKWidgetLabel *pUnitTurns;
		HKWidget *pTypeImage;

		int numBuildUnits;

		void UpdateUnitInfo();

		void OnSelectUnit(HKWidget &sender, const HKWidgetEventInfo &ev);
	} castleMenu;

	// group config
	struct GroupMenu : public Window
	{
		void Show();

	} groupMenu;

	struct UnitMenu : public Window
	{
		void Show();

	} unitMenu;

	struct InventoryMenu : public Window
	{
		void Show();

	} inventoryMenu;

	struct MiniMap : public Window
	{
		MiniMap(GameUI *pUI) : pUI(pUI) {}
		GameUI *pUI;

		void Show();

		HKWidget *pMap;
		HKWidgetButton *pClose;

		void OnFocusMap(HKWidget &sender, const HKWidgetEventInfo &ev);
	} miniMap;

	struct MsgBox : public Window
	{
		MsgBox(GameUI *pUI) : pUI(pUI) {}
		GameUI *pUI;

		void Show(const char *pMessage, MsgBoxDelegate selectCallback, bool bNotification);

		HKWidgetLabel *pMessage;
		HKWidgetButton *pAccept;
		HKWidgetButton *pCancel;

		MsgBoxDelegate selectCallback;

		void OnAcceptClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnCancelClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	} msgBox;

	Game *pGame;

	Castle *pShowCastle;
};

#endif
