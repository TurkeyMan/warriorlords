#if !defined(_MENUSCREEN_H)
#define _MENUSCREEN_H

#include "Screen.h"
#include "Button.h"
#include "ListBox.h"
#include "StringBox.h"

#include "MFFont.h"

class MenuScreen : public Screen
{
public:
	MenuScreen();
	virtual ~MenuScreen();

	virtual void Select();
	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void SetGameType(int type) { gameType = type; }

protected:
	struct MapData
	{
		char name[64];
		MapDetails details;
		bool bDetailsLoaded;
		MapData *pNext;
	};

	MFMaterial *pIcons;
	MFFont *pFont;

	ListBox *pMapList;
	StringBox *pGameName;
	MapData *pMaps;

	Button *pStart, *pEdit, *pReturn;

	HTTPRequest createRequest;

	int gameType;

	const char *pMessage;

	void Click(int button, int buttonID);
	void SelectMap(int item);
	void Created(HTTPRequest::Status status);
};

#endif
