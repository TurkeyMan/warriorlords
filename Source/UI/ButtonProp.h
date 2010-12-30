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
		TriggerOnDown,
		TriggerOnClick,

		MaxMode
	};

	static void RegisterEntity();
	static void *Create() { return new uiButtonProp; }

	uiButtonProp();
	virtual ~uiButtonProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
	static void SetState(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static MFString GetState(uiEntity *pEntity);

	static void SetStyle(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	static void SetHoverMessage(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetTriggerMode(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	MFMaterial *pImage;
	const MFString text;

	Style style;
	TriggerMode mode;

	int state;

	MFVector imageSize;
	MFVector textSize;

	static const char *pStyles[MaxStyle];
	static const char *pModes[MaxMode];
};

#endif
