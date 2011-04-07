#if !defined(_SELECTBOXPROP_H)
#define _SELECTBOXPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"
#include "ListProp.h"

class uiSelectBoxProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiSelectBoxProp; }

	uiSelectBoxProp();
	virtual ~uiSelectBoxProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	virtual bool HandleInputEvent(InputEvent ev, const InputInfo &info);

	int GetItemCount() { return list.GetItemCount(); }

	void SetSelection(int item);
	int GetSelection();

	MFString GetItem(int item) { return list.GetItem(item); }

	void ClearItems() { list.ClearItems(); }
	void AddItem(const char *pItem);

protected:
	void SelectItem(int item);

	static MFString GetItems(uiEntity *pEntity);
	static void SetItems(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void ClearItems(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void AddItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void RemoveItem(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetSelected(uiEntity *pEntity);
	static void SetSelected(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetCurrent(uiEntity *pEntity);
	static void SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	uiListProp list;

	MFFont *pFont;
};

#endif
