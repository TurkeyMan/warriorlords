#if !defined(_TEXTPROP_H)
#define _TEXTPROP_H

#include "Entity.h"
#include "MFFont.h"

class uiTextProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiTextProp; }

	uiTextProp();
	virtual ~uiTextProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

	void SetText(const char *pText) { text = pText; UpdateSize(); }

protected:
	static void SetFont(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetText(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetTextHeight(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetJustification(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	void UpdateSize();

	MFFont *pFont;
	MFString text;
	float textHeight;
	MFFontJustify justification;

	static const char *gJustifyStrings[MFFontJustify_Max];
};

#endif
