#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

class Button : public InputReceiver
{
public:
	typedef void (TriggerCallback)(int button, void *pUserData, int buttonID);

	static Button *Create(const MFMaterial *pImage, const MFRect *pPosition, const MFRect *pUVs, const MFVector &colour, TriggerCallback *pCallback, void *pUserData, int ButtonID = 0, bool bTriggerOnDown = false);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetImage(const MFMaterial *pImage, const MFRect *pUVs, const MFVector &colour = MFVector::one);
	void SetPos(const MFRect *pPos);

	void SetOutline(bool bEnable, const MFVector &colour);

protected:
	Button(const MFRect &rect) : InputReceiver(rect) { }

	MFVector colour;
	MFVector outlineColour;
	MFRect uvs;
	const MFMaterial *pMaterial, *pOverlay;
	bool bOutline;

	bool bTriggerOnDown;
	int isPressed;

	TriggerCallback *pCallback;
	void *pUserData;

	int button;
	int buttonID;
};

class CheckBox : public InputReceiver
{
public:
	typedef void (ChangeCallback)(int value, void *pUserData, int buttonID);

	static CheckBox *Create(const MFRect *pPosition, const char *pText, const MFVector &colour, int value, ChangeCallback *pCallback, void *pUserData, int ButtonID = 0);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetFont(MFFont *pFont);

	int GetValue() { return value; }
	int SetValue(int newValue);

protected:
	CheckBox(const MFRect &rect) : InputReceiver(rect) { }

	static void ButtonCallback(int button, void *pUserData, int buttonID);

	char text[64];

	MFVector colour;
	MFMaterial *pIcons;
	MFFont *pFont;

	Button *pButton;

	int isPressed;

	ChangeCallback *pCallback;
	void *pUserData;

	int value;
	int buttonID;
};

#endif
