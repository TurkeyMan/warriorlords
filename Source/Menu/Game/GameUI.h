#if !defined(_GAMEMENU_H)
#define _GAMEMENU_H

#include "UI/HKWidget.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"
#include "UI/Widgets/HKWidgetSelectbox.h"

class GameMenu
{
public:
	static GameMenu *Get();

	GameMenu();
	~GameMenu();

	void Show();
	void Hide();

	void ShowAsCurrent(HKWidget *pMenu);
	void HideCurrent();

	void ShowGameScreen() { gameScreen.Show(); }

protected:
	HKWidget *pMenu;
	HKWidget *pCurrentWindow;

	void OnCloseClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

	struct GameScreen
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetButton *pUndoButton;
		HKWidgetButton *pEndTurnButton;
		HKWidgetButton *pMinimapButton;

		void OnEndTurnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnUndoClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnMinimapClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	} gameScreen;

	struct BattleScreen
	{
		void Show();

		HKWidget *pMenu;

	} battleScreen;

	struct CastleMenu
	{
		void Show();

		HKWidget *pMenu;

	} castleMenu;

	struct GroupMenu
	{
		void Show();

		HKWidget *pMenu;

	} groupMenu;

	struct UnitMenu
	{
		void Show();

		HKWidget *pMenu;

	} unitMenu;

	struct InventoryMenu
	{
		void Show();

		HKWidget *pMenu;

	} inventoryMenu;

	struct MiniMap
	{
		void Show();

		HKWidget *pMenu;

	} miniMap;
};

#endif
