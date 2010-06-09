#if !defined(_STRINGBOX_H)
#define _STRINGBOX_H

#include "StringEntryLogic.h"
#include "InputHandler.h"

struct MFFont;

class StringBox : public InputReceiver
{
public:
	typedef void (ChangeCallback)(const char *pString, void *pUserData);

	StringBox(MFRect &rect, StringEntryLogic::StringType type);
	virtual ~StringBox();

	static StringBox *Create(MFFont *pFont, MFRect *pPos, ChangeCallback *pCallback = NULL, void *pUserData = NULL, StringEntryLogic::StringType type = StringEntryLogic::ST_Regular, const char *pDefaultString = NULL);
	void Destroy();

	void RegisterTabCallback(ChangeCallback *pCallback, void *pUserData = NULL);

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Update();
	void Draw();

	void Show() { bVisible = true; }
	void Hide() { bVisible = false; }

	void Enable(bool bEnable);

	void SetString(const char *pString) { return stringLogic.SetString(pString); }
	const char *GetString() { return stringLogic.GetString(); }

protected:
	static void StringChangeCallback(const char *pString, void *pUserData);

	StringEntryLogic stringLogic;
	ChangeCallback *pCallback;
	void *pUserData;
	ChangeCallback *pTab;
	void *pTabData;

	MFFont *pFont;

	bool bVisible;
	bool bEnabled;

	bool bFirstFrame;
};

#endif
