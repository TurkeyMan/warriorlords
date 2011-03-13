#if !defined(_STRINGPROP_H)
#define _STRINGPROP_H

#include "StringEntryLogic.h"
#include "Entity.h"

#include "MFMaterial.h"
#include "MFFont.h"

class uiStringProp : public uiEntity
{
public:
	typedef FastDelegate1<const char *> ChangeCallback;

	static void RegisterEntity();
	static void *Create() { return new uiStringProp; }

	uiStringProp();
	virtual ~uiStringProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	virtual bool HandleInputEvent(InputEvent ev, const InputInfo &info);
	virtual bool ChangeFocus(bool bGainFocus);

	void SetChangeCallback(ChangeCallback handler) { changeCallback = handler; }
	void SetTabCallback(ChangeCallback handler) { tabCallback = handler; }

	void SetType(StringEntryLogic::StringType type) { stringLogic.SetType(type); }

	void SetString(MFString string) { return stringLogic.SetString(string); }
	MFString GetString() { return stringLogic.GetString(); }

protected:
	void StringChangeCallback(const char *pString);

	void UpdateCursorPos(float x, bool bUpdateSelection);

	static MFString GetText(uiEntity *pEntity);
	static MFString GetEmpty(uiEntity *pEntity);
	static void SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetType(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	StringEntryLogic stringLogic;
	ChangeCallback changeCallback;
	ChangeCallback tabCallback;

	MFFont *pFont;
	float textHeight;
	bool bHandleTab;

	bool bFirstFrame;

	static float blinkTime;
};

#endif
