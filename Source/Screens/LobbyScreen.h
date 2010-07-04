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

	static void Click(int button, void *pUserData, int buttonID);
	static void SetRace(int item, void *pUserData, int id);
	static void SetColour(int item, void *pUserData, int id);
};

#endif
