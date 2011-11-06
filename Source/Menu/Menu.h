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
		ListItem() { id = 0; numPlayers = 0; }
		ListItem(ListItem &i) { *this = i; }
		ListItem &operator=(ListItem &i)
		{
			id = i.id;
			name = i.name;
			map = i.map;
			numPlayers = i.numPlayers;
			return *this;
		}

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

	void OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

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

	struct ListMenu
	{
		ListMenu()	: gameListAdapter(gameList) {}

		enum Type
		{
			Create,
			Offline,
			Join,
			Resume
		};

		void ShowResume();
		void ShowJoin();

		void ShowCreate(bool bOnline);

		HKWidget *pMenu;

		HKWidgetListbox *pActiveList;
		HKWidgetButton *pResumeButton;
		HKWidgetButton *pReturnButton;
		HKWidgetLabel *pTitle;
		HKWidget *pNamePanel;
		HKWidgetTextbox *pName;

		HKDynamicArray<ListItem> gameList;
		GameListAdapter gameListAdapter;

		Type type;
		bool bReturnToMain;

		void OnSelect(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnNameChanged(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnContinueClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

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

		void OnUpdateLogin(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnLogin(HKWidget &sender, const HKWidgetEventInfo &ev);
		void OnLoginResponse(ServerError err, Session *pSession);
	} loginMenu;
};

#endif
