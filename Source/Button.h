#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

class Button : public InputReceiver
{
public:
	typedef void (TriggerCallback)(int button, void *pUserData, int buttonID); 

	Button(MFRect &rect) : InputReceiver(rect) { }

	static Button *Create(MFMaterial *pImage, MFRect *pPosition, MFRect *pUVs, TriggerCallback *pCallback, void *pUserData, int ButtonID = 0, bool bTriggerOnDown = false);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetImage(MFMaterial *pImage, MFRect *pUVs);
	void SetPos(MFRect *pPos);

	void SetOutline(bool bEnable, const MFVector &colour);

protected:
	MFMaterial *pMaterial;
	MFRect pos;
	MFRect uvs;

	MFVector outlineColour;
	bool bOutline;

	bool bTriggerOnDown;
	bool bIsPressed;

	TriggerCallback *pCallback;
	void *pUserData;

	int button;
	int buttonID;
};

#endif
