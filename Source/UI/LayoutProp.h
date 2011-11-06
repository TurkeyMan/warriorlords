#if !defined(_LAYOUTPROP_H)
#define _LAYOUTPROP_H

#include "MFIni.h"
#include "MFPtrList.h"
#include "../Tools/Factory.h"
#include "Action.h"

class uiLayoutProp : public uiEntity
{
public:
	static void RegisterEntity();
	static void *Create() { return new uiLayoutProp; }

	static uiLayoutProp *CreateLayout(const char *pName, const char *pLayoutDescription, uiEntity *pParent);

	uiLayoutProp();
	virtual ~uiLayoutProp();

protected:
	static void SetTitle(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	static void SetLayout(uiEntity *pEntity, uiRuntimeArgs *pArguments);

	void LoadLayout(const char *pLayoutDescriptor);

	MFIni *pLayoutDesc;
	MFIniLine *pResources;
	MFIniLine *pMetrics;
	MFIniLine *pActions;

	MFString title;
};

#endif
