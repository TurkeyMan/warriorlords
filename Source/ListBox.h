#if !defined(_LISTBOX_H)
#define _LISTBOX_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

/*
class ListItem : public InputReceiver
{
public:
	typedef void (SelectCallback)(int button, void *pUserData, int itemID);

	static ListItem *Create(const char *pText, MFRect uvs, SelectCallback *pCallback, void *pUserData);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

protected:
	ListItem(const MFRect &rect) : InputReceiver(rect) { }

	const char *pString;
	MFRect uvs;
	bool bImage;

	SelectCallback *pCallback;
	void *pUserData;
};
*/

class ListBox : public InputReceiver
{
public:
	typedef void (ListCallback)(int item, void *pUserData);

	static ListBox *Create(const MFRect *pPosition, MFFont *pFont, MFMaterial *pIcons = NULL, float iconSize = 32);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetPos(const MFRect *pPos);

	void SetSelectCallback(ListCallback *pCallback, void *pUserData) { pSelectCallback = pCallback; pSelectUserData = pUserData; }

	int AddItem(const char *pText, int icon = -1);
	const char *GetItemText(int item);

	int GetSelection() { return selection; }

protected:
	ListBox(const MFRect &rect) : InputReceiver(rect) { }

	MFFont *pFont;
	MFMaterial *pIcons;
	float iconSize;

	float itemHeight;
	float textOffset;

	struct ListItem
	{
		char text[28];
		int icon;
	};

	ListItem *pItems;
	int numAllocated;
	int numItems;

	int selection;

	float yOffset;
	float velocity;

	float downPos;

	ListCallback *pSelectCallback;
	void *pSelectUserData;
};

#endif
