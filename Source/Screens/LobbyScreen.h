#if !defined(_LOBBYSCREEN_H)
#define _LOBBYSCREEN_H

#include "Screen.h"
#include "Button.h"
#include "StringBox.h"
#include "SelectBox.h"

#include "MFFont.h"

struct Lobby
{
	GameDetails details;

	bool bOffline;
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
	MFMaterial *pIcons;
	MFFont *pFont;

	Button *pBegin, *pLeave, *pReturn;

	SelectBox *pRaces[8];
	SelectBox *pColours[8];

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

	int raceID, colourID, newRace, newColour;

	void Click(int button, int buttonID);
	void SetRace(int item, int id);
	void SetColour(int item, int id);
	void GetDetails(HTTPRequest::Status status);
	void BeginGame(HTTPRequest::Status status);
	void EnterGame(HTTPRequest::Status status);
	void CommitRace(HTTPRequest::Status status);
	void CommitColour(HTTPRequest::Status status);
};

#endif
