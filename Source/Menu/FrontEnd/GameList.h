#if !defined(_MENU_GAMELIST_H)
#define _MENU_GAMELIST_H

#include "Haku/UI/Widgets/HKWidgetButton.h"
#include "Haku/UI/Widgets/HKWidgetTextbox.h"
#include "Haku/UI/Widgets/HKWidgetListbox.h"

class FrontMenu;

class ListMenu
{
public:
	enum Type
	{
		Create,
		Offline,
		Join,
		Resume
	};

	ListMenu()	: gameListAdapter(gameList) {}

	void Load(HKWidget *pRoot, FrontMenu *pFrontMenu);

	void ShowResume();
	void ShowJoin();

	void ShowCreate(bool bOnline);

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

	HKWidgetListbox *pActiveList;
	HKWidgetButton *pResumeButton;
	HKWidgetButton *pReturnButton;
	HKWidgetButton *pEditButton;
	HKWidgetLabel *pTitle;
	HKWidget *pNamePanel;
	HKWidgetTextbox *pName;

	HKDynamicArray<ListItem> gameList;
	GameListAdapter gameListAdapter;

	Type type;
	bool bReturnToMain;

	void ShowLobby(GameDetails &game);

	void OnSelect(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnNameChanged(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnContinueClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnEditClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

	void OnUpdateResponse(ServerError err, Session *pSession);
	void OnFindResponse(ServerError err, Session *pSession, GameLobby *pGames, int numGames);
	void OnGameCreated(ServerError error, Session *pSession, GameDetails *pGame);
	void OnGameJoined(ServerError error, Session *pSession, GameDetails *pGame);
};

#endif
