#if !defined(_FRONTMENU_H)
#define _FRONTMENU_H

#include "UI/HKWidget.h"
#include "UI/Widgets/HKWidgetButton.h"
#include "UI/Widgets/HKWidgetTextbox.h"
#include "UI/Widgets/HKWidgetListbox.h"

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

protected:
	struct ListItem
	{
		uint32 id;
		MFString name;
		MFString map;
		int numPlayers;
	};

	class GameListAdapter : public HKArrayAdapter<ListItem>
	{
	public:
		GameListAdapter(HKDynamicArray<ListItem> &gameList) : HKArrayAdapter<ListItem>(gameList) { }

		HKWidget *GetItemView(int index, ListItem &item);
		void UpdateItemView(int index, ListItem &item, HKWidget *pLayout);
	};

	HKWidget *pMenu;
	HKWidget *pCurrentWindow;

	void OnReturnClicked(HKWidget &sender, HKWidgetEventInfo &ev);

	struct MainMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetButton *pPlayButton;
		HKWidgetButton *pResumeButton;
		HKWidgetButton *pLoginButton;
		HKWidgetButton *pProfileButton;
		HKWidgetLabel *pTitle;

		void OnPlayClicked(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnResumeClicked(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnLoginClicked(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnProfileClicked(HKWidget &sender, HKWidgetEventInfo &ev);
	} mainMenu;

	struct PlayMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetButton *pCreateButton;
		HKWidgetButton *pJoinButton;
		HKWidgetButton *pOfflineButton;
		HKWidgetButton *pReturnButton;

		void OnCreateClicked(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnJoinClicked(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnOfflineClicked(HKWidget &sender, HKWidgetEventInfo &ev);
	} playMenu;

	struct ListMenu
	{
		ListMenu()	: gameListAdapter(gameList) {}

		void ShowResume();
		void ShowJoin();

		HKWidget *pMenu;

		HKWidgetListbox *pActiveList;
		HKWidgetButton *pResumeButton;
		HKWidgetButton *pReturnButton;
		HKWidgetLabel *pTitle;
		HKWidget *pNamePanel;

		HKDynamicArray<ListItem> gameList;
		GameListAdapter gameListAdapter;

		void OnUpdateResponse(ServerError err, Session *pSession);
		void OnFindResponse(ServerError err, Session *pSession, GameLobby *pGames, int numGames);
	} listMenu;

	struct LoginMenu
	{
		void Show();

		HKWidget *pMenu;

		HKWidgetTextbox *pUsernameText;
		HKWidgetTextbox *pPasswordText;
		HKWidgetButton *pLoginButton;
		HKWidgetButton *pReturnButton;

		void OnUpdateLogin(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnLogin(HKWidget &sender, HKWidgetEventInfo &ev);
		void OnLoginResponse(ServerError err, Session *pSession);
	} loginMenu;
};

#endif
