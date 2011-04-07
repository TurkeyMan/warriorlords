#if !defined(_LISTPROP_H)
#define _LISTPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"

class uiListProp : public uiEntity
{
	friend class uiSelectBoxProp;
public:
	typedef FastDelegate1<int> ListCallback;

	static void RegisterEntity();
	static void *Create() { return new uiListProp; }

	uiListProp();
	virtual ~uiListProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	virtual bool HandleInputEvent(InputEvent ev, const InputInfo &info);

	void SetSelectCallback(ListCallback callback) { selectCallback = callback; }
	void SetDblClickCallback(ListCallback callback) { dblClickCallback = callback; }

	int GetItemCount() { return items.size(); }

	void SetSelection(int item);
	int GetSelection() { return selection; }

	MFString GetItem(int item) { return selection >= 0 ? items[selection].text : ""; }

	void ClearItems();
	void AddItem(const char *pItem);

protected:
	void FollowCursor(bool bFollowCursor);

	static MFString GetItems(uiEntity *pEntity);
	static void SetItems(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void ClearItems(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void AddItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void RemoveItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString IsSelected(uiEntity *pEntity);
	static MFString GetSelected(uiEntity *pEntity);
	static void SetSelected(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetCurrent(uiEntity *pEntity);
	static void SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	class ListItem
	{
	public:
		MFString text;
	};

	MFArray<ListItem> items;

	MFFont *pFont;

	int selection;

	float yOffset;
	float velocity;

	float downPos;
	bool bSelecting;

	float hoverX, hoverY;

	bool bFollowCursor;

	ListCallback selectCallback;
	ListCallback dblClickCallback;
};

#endif
