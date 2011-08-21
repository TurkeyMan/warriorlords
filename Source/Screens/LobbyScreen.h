#if !defined(_LOBBYSCREEN_H)
#define _LOBBYSCREEN_H

#include "Screen.h"
#include "Button.h"
#include "StringBox.h"
#include "SelectBox.h"

#include "MFFont.h"

class uiSelectBoxProp;

class Lobby
{
public:
	void InitLobby(uiEntity *pLobby);

protected:
	static void ShowLobby(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void StartGame(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	void RepopulateHeroes(int race, int player);

	void SelectRace(uiSelectBoxProp *pSelectBox, int item, void *pUserData);
	void SelectColour(uiSelectBoxProp *pSelectBox, int item, void *pUserData);
	void SelectHero(uiSelectBoxProp *pSelectBox, int item, void *pUserData);

	void OnBegin(ServerError error, Session *pSession);

	uiEntity *pLobby;

	// begin game params
	GameParams params;
};

class LobbyScreen : public Screen
{
public:
	LobbyScreen();
	virtual ~LobbyScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	bool ShowOnline(uint32 lobbyID);
	void ShowOffline(const MapDetails *pMap);

	void UpdateLobbyState();

protected:
	void Refresh();

	MFMaterial *pIcons;
	MFFont *pFont;

	Button *pBegin, *pLeave, *pReturn;

	SelectBox *pRaces[8];
	SelectBox *pColours[8];
	SelectBox *pHeroes[8];

	GameDetails details;
	MapDetails map;
	bool bOffline;

	GameParams params;

	HTTPRequest request;
	HTTPRequest begin;
	HTTPRequest enter;
	HTTPRequest leave;
	HTTPRequest race;
	HTTPRequest colour;
	HTTPRequest hero;

	int raceID, colourID, newRace, newColour, heroID, newHero;

	void SetHeroes(int player);
	void Click(int button, int buttonID);
	void SetRace(int item, int id);
	void SetColour(int item, int id);
	void SetHero(int item, int id);
	void GetDetails(HTTPRequest::Status status);
	void BeginGame(HTTPRequest::Status status);
	void EnterGame(HTTPRequest::Status status);
	void CommitRace(HTTPRequest::Status status);
	void CommitColour(HTTPRequest::Status status);
	void CommitHero(HTTPRequest::Status status);
};

#endif
