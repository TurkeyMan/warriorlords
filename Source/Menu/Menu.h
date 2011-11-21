#if !defined(_FRONTMENU_H)
#define _FRONTMENU_H

#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"
#include "UI/Widgets/HKWidgetSelectbox.h"

#include "Menu/FrontEnd/Lobby.h"
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
		void OnJoinClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
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
		void OnLoginResponse(ServerError err, Session *pSession);
	} loginMenu;

protected:
	HKWidget *pMenu;
	HKWidget *pCurrentWindow;
};

#endif
