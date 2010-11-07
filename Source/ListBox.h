#if !defined(_LISTBOX_H)
#define _LISTBOX_H

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
	typedef FastDelegate1<int> ListCallback;

	static ListBox *Create(const MFRect *pPosition, MFFont *pFont, MFMaterial *pIcons = NULL, float iconSize = 32);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetPos(const MFRect *pPos);
	void SetBackColour(const MFVector &colour) { backColour = colour; }

	void HighlightCursor(bool bEnable) { bHighlightCursor = bEnable; }

	void SetSelectCallback(ListCallback callback) { selectCallback = callback; }
	void SetDblClickCallback(ListCallback callback) { dblClickCallback = callback; }

	void Clear();

	int AddItem(const char *pText, int icon = -1, void *pUserData = NULL, const MFVector &colour = MFVector::white);
	inline int GetNumItems() { return numItems; }
	const char *GetItemText(int item);
	const void *GetItemData(int item);
	const MFVector &GetItemColour(int item);
	int GetItemIcon(int item);

	void SetSelection(int item) { selection = MFMin(item, numItems-1); }
	int GetSelection() { return selection; }

	float GetItemHeight();

protected:
	ListBox(const MFRect &rect) : InputReceiver(rect) { }

	MFFont *pFont;
	MFMaterial *pIcons;
	float iconSize;

	MFVector backColour;

	bool bHighlightCursor;

	float itemHeight;
	float textOffset;

	struct ListItem
	{
		MFVector colour;
		char text[28];
		int icon;
		void *pUserData;
	};

	ListItem *pItems;
	int numAllocated;
	int numItems;

	int selection;

	float yOffset;
	float velocity;

	float downPos;
	bool bSelecting;

	ListCallback selectCallback;
	ListCallback dblClickCallback;
};

#endif
