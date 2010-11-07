#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "InputHandler.h"

struct MFMaterial;
struct MFFont;

class Button : public InputReceiver
{
public:
	typedef FastDelegate2<int, int> ClickCallback;

	static Button *Create(const MFMaterial *pImage, const MFRect *pPosition, const MFRect *pUVs, const MFVector &colour, int ButtonID = 0, bool bTriggerOnDown = false);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetClickCallback(ClickCallback clickHandler) { clickCallback = clickHandler; }
	void SetImage(const MFMaterial *pImage, const MFRect *pUVs, const MFVector &colour = MFVector::one);
	void SetPos(const MFRect *pPos);

	void SetOutline(bool bEnable, const MFVector &colour);

	void Enable(bool enabled) { bEnabled = enabled; }

protected:
	Button(const MFRect &rect) : InputReceiver(rect) { }

	MFVector colour;
	MFVector outlineColour;
	MFRect uvs;
	const MFMaterial *pMaterial, *pOverlay;
	bool bOutline;
	bool bEnabled;

	bool bTriggerOnDown;
	int isPressed;

	ClickCallback clickCallback;
	int buttonID;

	int button;
};

class CheckBox : public InputReceiver
{
public:
	typedef FastDelegate2<int, int> ChangeCallback;

	static CheckBox *Create(const MFRect *pPosition, const char *pText, const MFVector &colour, int value, int ButtonID = 0);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetFont(MFFont *pFont);

	void SetClickCallback(Button::ClickCallback clickHandler) { clickCallback = clickHandler; }
	void SetChangeCallback(ChangeCallback changeHandler) { changeCallback = changeHandler; }
	int GetValue() { return value; }
	int SetValue(int newValue);

protected:
	CheckBox(const MFRect &rect) : InputReceiver(rect) { }

	void ButtonCallback(int button, int buttonID);

	char text[64];

	MFVector colour;
	MFMaterial *pIcons;
	MFFont *pFont;

	Button *pButton;

	int isPressed;

	Button::ClickCallback clickCallback;
	ChangeCallback changeCallback;

	int value;
	int buttonID;
};

#endif
