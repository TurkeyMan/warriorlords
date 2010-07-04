#if !defined(_SELECTBOX_H)
#define _SELECTBOX_H

#include "InputHandler.h"
#include "ListBox.h"

struct MFMaterial;
struct MFFont;

class SelectBox : public InputReceiver
{
public:
	typedef void (SelectCallback)(int item, void *pUserData, int id);

	static SelectBox *Create(const MFRect *pPosition, MFFont *pFont, MFMaterial *pIcons = NULL, float iconSize = 32);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetImage(const MFMaterial *pImage, const MFRect *pUVs, const MFVector &colour = MFVector::one);
	void SetPos(const MFRect *pPos);

	void SetSelectCallback(SelectCallback *pCallback, void *pUserData, int id = 0);

	void Enable(bool enabled) { bEnabled = enabled; }

	void Clear();

	int AddItem(const char *pText, int icon = -1, void *pUserData = NULL, const MFVector &colour = MFVector::white);
	inline int GetNumItems() { return pList->GetNumItems(); }
	const char *GetItemText(int item);
	const void *GetItemData(int item);

	void SetSelection(int item);
	int GetSelection();

protected:
	SelectBox(const MFRect &rect) : InputReceiver(rect) { }

	MFFont *pFont;
	MFMaterial *pIcons;
	float iconSize;

	float itemHeight;
	float textOffset;

	bool bEnabled;

	bool bShowList;

	ListBox *pList;

	SelectCallback *pCallback;
	void *pUserData;
	int id;

	static void ListCallback(int item, void *pUserData);
};

#endif
