#if !defined(_MODELPROP_H)
#define _MODELPROP_H

#include "Entity.h"
#include "MFModel.h"

class uiModelProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiModelProp; }

	uiModelProp();
	virtual ~uiModelProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
	static void SetModel(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	MFModel *pModel;
};

#endif
