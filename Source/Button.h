#if !defined(_BUTTON_H)
#define _BUTTON_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFMaterial;

class Button : public InputHandler
{
public:
	typedef void (TriggerCallback)(int button, void *pUserData, int buttonID); 

	static Button *Create(MFMaterial *pImage, MFRect *pPosition, MFRect *pUVs, TriggerCallback *pCallback, void *pUserData, int ButtonID = 0, bool bTriggerOnDown = false);
	void Destroy();

	virtual int UpdateInput();
	void Draw();

	void SetPos(MFRect *pPos);
	void SetUVs(MFRect *pUVs);

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

	int buttonID;
};

#endif
