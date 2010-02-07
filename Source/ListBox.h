#if !defined(_LISTBOX_H)
#define _LISTBOX_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

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

class ListBox : public InputReceiver
{
public:
	static ListBox *Create(const MFRect *pPosition);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetPos(const MFRect *pPos);

	ListItem *GetItem(int item);

protected:
	ListBox(const MFRect &rect) : InputReceiver(rect) { }

	ListItem **ppItems;
	int numItems;
};

#endif
