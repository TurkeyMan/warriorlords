#if !defined(_SELECTBOXPROP_H)
#define _SELECTBOXPROP_H

#include "Entity.h"
#include "MFMaterial.h"
#include "MFFont.h"

class uiSelectBoxProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiSelectBoxProp; }

	uiSelectBoxProp();
	virtual ~uiSelectBoxProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
};

#endif
