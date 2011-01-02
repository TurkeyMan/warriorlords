#if !defined(_LISTPROP_H)
#define _LISTPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"

class uiListProp : public uiEntity
{
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

	void SetSelection(int item);
	int GetSelection() { return selection; }

protected:
	static MFString GetItems(uiEntity *pEntity);
	static void SetItems(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void AddItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void RemoveItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetSelected(uiEntity *pEntity);
	static void SetSelected(uiEntity *pEntity, uiRuntimeArgs *pArguments);
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

	ListCallback selectCallback;
	ListCallback dblClickCallback;
};

#endif
