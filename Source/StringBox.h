#if !defined(_STRINGBOX_H)
#define _STRINGBOX_H

#include "StringEntryLogic.h"
#include "InputHandler.h"

struct MFFont;

class StringBox : public InputReceiver
{
public:
	typedef FastDelegate1<MFString> ChangeCallback;

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
	void SetString(MFString string) { return stringLogic.SetString(string); }
	MFString GetString() { return stringLogic.GetString(); }

protected:
	void StringChangeCallback(MFString string);

	StringEntryLogic stringLogic;
	ChangeCallback changeCallback;
	ChangeCallback tabCallback;

	MFFont *pFont;

	bool bVisible;
	bool bEnabled;

	bool bFirstFrame;
};

#endif
