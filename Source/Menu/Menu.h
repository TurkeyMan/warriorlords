#if !defined(_FRONTMENU_H)
#define _FRONTMENU_H

#include "Haku/UI/Widgets/HKWidgetButton.h"
#include "Haku/UI/Widgets/HKWidgetTextbox.h"
#include "Haku/UI/Widgets/HKWidgetListbox.h"
#include "Haku/UI/Widgets/HKWidgetSelectbox.h"

#include "Menu/FrontEnd/LobbyMenu.h"
#include "Menu/FrontEnd/GameList.h"

class FrontMenu
{
public:
	static FrontMenu *Get();

	FrontMenu();
	~FrontMenu();

	void Show();
	void Hide();

	void ShowAsCurrent(HKWidget *pMenu);
	void HideCurrent();

	void ShowMainMenu() { mainMenu.Show(); }

	void OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

	ListMenu listMenu;
	LobbyMenu lobbyMenu;

	struct MainMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetButton *pPlayButton;
		HKWidgetButton *pResumeButton;
		HKWidgetButton *pLoginButton;
		HKWidgetButton *pProfileButton;
		HKWidgetLabel *pTitle;

		void OnPlayClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnResumeClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnLoginClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnProfileClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	} mainMenu;

	struct PlayMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetButton *pCreateButton;
		HKWidgetButton *pJoinButton;
		HKWidgetButton *pOfflineButton;
		HKWidgetButton *pReturnButton;

		void OnCreateClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
//		void OnJoinClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnOfflineClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	} playMenu;

	struct LoginMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetTextbox *pUsernameText;
		HKWidgetTextbox *pPasswordText;
		HKWidgetButton *pLoginButton;
		HKWidgetButton *pReturnButton;

		void OnUpdateLogin(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnLogin(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnLoginResponse(ServerRequest *pReq);
	} loginMenu;

protected:
	HKWidget *pMenu;
	HKWidget *pCurrentWindow;
};

#endif
