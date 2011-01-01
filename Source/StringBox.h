#if !defined(_STRINGBOX_H)
#define _STRINGBOX_H

#include "StringEntryLogic.h"
#include "InputHandler.h"

struct MFFont;

class StringBox : public InputReceiver
{
public:
	typedef FastDelegate1<const char *> ChangeCallback;

	StringBox(MFRect &rect, StringEntryLogic::StringType type);
	virtual ~StringBox();

	static StringBox *Create(MFFont *pFont, MFRect *pPos, StringEntryLogic::StringType type = StringEntryLogic::ST_Regular, const char *pDefaultString = NULL);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Update();
	void Draw();

	void Show() { bVisible = true; }
	void Hide() { bVisible = false; }

	void Enable(bool bEnable);

	void SetChangeCallback(ChangeCallback handler) { changeCallback = handler; }
	void SetTabCallback(ChangeCallback handler) { tabCallback = handler; }
	void SetString(const char *pString) { return stringLogic.SetString(pString); }
	MFString GetString() { return stringLogic.GetString(); }

protected:
	void StringChangeCallback(const char *pString);

	StringEntryLogic stringLogic;
	ChangeCallback changeCallback;
	ChangeCallback tabCallback;

	MFFont *pFont;

	bool bVisible;
	bool bEnabled;

	bool bFirstFrame;
};

#endif
