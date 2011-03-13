#if !defined(_DATAPROP_H)
#define _DATAPROP_H

#include "Entity.h"
#include "GameData.h"

class uiDataProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiDataProp; }

	uiDataProp();
	virtual ~uiDataProp();

	virtual void Update();
	virtual void Draw(const uiDrawState &state);

protected:
	static MFString GetMaps(uiEntity *pEntity);

	GameData *pData;
};

#endif
