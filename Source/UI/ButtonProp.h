#if !defined(_BUTTONPROP_H)
#define _BUTTONPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"

class uiButtonProp : public uiEntity
{
public:
	enum Style
	{
		Button,
		StateButton,
		Checkbox,

		MaxStyle
	};

	enum TriggerMode
	{
		TriggerOnClick,
		TriggerOnDown,

		MaxMode
	};

	static void RegisterEntity();
	static void *Create() { return new uiButtonProp; }

	uiButtonProp();
	virtual ~uiButtonProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	virtual bool HandleInputEvent(InputEvent ev, const InputInfo &info);

protected:
	static void SetState(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetState(uiEntity *pEntity);

	static void SetStyle(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetHoverMessage(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetTriggerMode(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetJustification(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	void UpdateTextSize();
	void UpdateSize();

	MFMaterial *pImage;
	MFString text;

	Style style;
	TriggerMode mode;

	int state;

	int id;
	int button;

	MFFont *pFont;
	float textHeight;
	MFFontJustify justification;

	MFVector imageSize;
	MFVector textSize;

	static const char *pStyles[MaxStyle];
	static const char *pModes[MaxMode];
};

#endif
