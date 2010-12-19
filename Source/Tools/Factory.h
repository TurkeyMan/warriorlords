#if !defined(_FACTORY_H)
#define _FACTORY_H

typedef void *(Factory_CreateFunc)();

struct FactoryType
{
	char typeName[64];
	Factory_CreateFunc *pCreateFunc;
	FactoryType *pParent;
	FactoryType *pNext;
};

template <typename T>
class Factory
{
public:
	Factory() { pTypes = NULL; }

	FactoryType *RegisterType(const char *pTypeName, Factory_CreateFunc *pCreateCallback, FactoryType *pParent = NULL)
	{
		FactoryType *pType = new FactoryType;
		MFString_Copy(pType->typeName, pTypeName);
		pType->pCreateFunc = pCreateCallback;
		pType->pParent = pParent;
		pType->pNext = pTypes;
		pTypes = pType;
		return pType;
	}

	T *Create(const char *pTypeName, FactoryType **ppType = NULL)
	{
		for(FactoryType *pType = pTypes; pType; pType = pType->pNext)
		{
			if(!MFString_CaseCmp(pTypeName, pType->typeName))
			{
				if(ppType)
					*ppType = pType;
				return (T*)pType->pCreateFunc();
			}
		}

		MFDebug_Warn(2, MFStr("Unknown factory type: '%s'", pTypeName));

		return NULL;
	}

	FactoryType *FindType(const char *pTypeName)
	{
		for(FactoryType *pType = pTypes; pType; pType = pType->pNext)
		{
			if(!MFString_CaseCmp(pTypeName, pType->typeName))
				return pType;
		}
		return NULL;
	}

protected:
	FactoryType *pTypes;
};

#endif
