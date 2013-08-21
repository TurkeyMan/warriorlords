#if !defined(_MENU_LOBBY_H)
#define _MENU_LOBBY_H

#include "Haku/UI/Widgets/HKWidgetButton.h"
#include "Haku/UI/Widgets/HKWidgetTextbox.h"
#include "Haku/UI/Widgets/HKWidgetListbox.h"
#include "Haku/UI/Widgets/HKWidgetSelectbox.h"

#include "Lobby.h"

class FrontMenu;

class LobbyMenu
{
public:
	LobbyMenu()	: raceListAdapter(raceList), colourListAdapter(colourList) {}

	void Load(HKWidget *pRoot, FrontMenu *pFrontMenu);
	void Show(Lobby *pGame);

	void UpdateUIState();

	Lobby::Player *GetLobbyPlayer(uint32 id, int *pPlayer);

protected:
	struct ListItem
	{
		ListItem() { id = 0; }
		ListItem(ListItem &i) { *this = i; }
		ListItem &operator=(ListItem &i)
		{
			name = i.name;
			id = i.id;
			return *this;
		}

		MFString name;
		int id;
	};

	class GameListAdapter : public HKArrayAdapter<ListItem>
	{
	public:
		GameListAdapter(HKDynamicArray<ListItem> &gameList) : HKArrayAdapter<ListItem>(gameList) { }

		HKWidget *GetItemView(int index, ListItem &item);
		void UpdateItemView(int index, ListItem &item, HKWidget *pLayout);
	};

	class ColourListAdapter : public HKArrayAdapter<MFVector>
	{
	public:
		ColourListAdapter(HKDynamicArray<MFVector> &gameList) : HKArrayAdapter<MFVector>(gameList) { }

		HKWidget *GetItemView(int index, MFVector &item);
		void UpdateItemView(int index, MFVector &item, HKWidget *pLayout);
	};

	struct Player
	{
		Player() : heroListAdapter(heroList) {}

		HKWidget *pPlayerRow;

		HKWidgetLabel *pName;

		HKWidget *pPlayerConfig;
		HKWidgetSelectbox *pRace;
		HKWidgetSelectbox *pColour;
		HKWidgetSelectbox *pHero;

		HKDynamicArray<ListItem> heroList;
		GameListAdapter heroListAdapter;
	};

	Player players[8];
	int numPlayers;

	FrontMenu *pFrontMenu;

	HKWidget *pMenu;

	HKWidgetLabel *pTitle;
	HKWidgetButton *pStartButton;
	HKWidgetButton *pLeaveButton;
	HKWidgetButton *pReturnButton;

	HKDynamicArray<ListItem> raceList;
	GameListAdapter raceListAdapter;

	HKDynamicArray<MFVector> colourList;
	ColourListAdapter colourListAdapter;

	Lobby *pLobby;

	int setPlayer;
	int setRace, setColour, setHero;

	bool bUpdatingLists, bUpdatingHeroes;

	void RepopulateHeroes(int player, int race, int hero);

	void ReceivePeerMessage(uint32 user, const char *pMessage);

	void OnReturnClicked(HKWidget &sender, const HKWidgetEventInfo &ev);

	void OnStartClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnLeaveClicked(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnRaceChanged(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnColourChanged(HKWidget &sender, const HKWidgetEventInfo &ev);
	void OnHeroChanged(HKWidget &sender, const HKWidgetEventInfo &ev);

	void Commit(ServerRequest *pReq);
//	void CommitRace(ServerRequest *pReq);
//	void CommitColour(ServerRequest *pReq);
//	void CommitHero(ServerRequest *pReq);
	void OnBegin(ServerRequest *pReq);
	void OnGameLeft(ServerRequest *pReq);

//	void StartGame(ServerRequest *pReq);
};

#endif
