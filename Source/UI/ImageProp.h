#if !defined(_IMAGEPROP_H)
#define _IMAGEPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"

class uiImageProp : public uiEntity
{
public:
	enum Orientation
	{
		Normal = 0,
		Rotate_90cw,
		Rotate_180,
		Rotate_90ccw,
		HFlip,
		VFlip
	};

	static void RegisterEntity();
	static void *Create() { return new uiImageProp; }

	uiImageProp();
	virtual ~uiImageProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
	static void SetImage(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetOrientation(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	MFMaterial *pImage;
	Orientation orientation;
	MFVector imageSize;
};

#endif
