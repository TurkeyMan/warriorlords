#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

class Button : public InputReceiver
{
public:
	typedef void (TriggerCallback)(int button, void *pUserData, int buttonID); 

	Button(const MFRect &rect) : InputReceiver(rect) { }

	static Button *Create(const MFMaterial *pImage, const MFRect *pPosition, const MFRect *pUVs, const MFVector &colour, TriggerCallback *pCallback, void *pUserData, int ButtonID = 0, bool bTriggerOnDown = false);
	void Destroy();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	void Draw();

	void SetImage(const MFMaterial *pImage, const MFRect *pUVs, const MFVector &colour = MFVector::one);
	void SetPos(const MFRect *pPos);

	void SetOutline(bool bEnable, const MFVector &colour);

protected:
	MFVector colour;
	MFVector outlineColour;
	MFRect uvs;
	const MFMaterial *pMaterial, *pOverlay;
	bool bOutline;

	bool bTriggerOnDown;
	int  isPressed;

	TriggerCallback *pCallback;
	void *pUserData;

	int button;
	int buttonID;
};

#endif
