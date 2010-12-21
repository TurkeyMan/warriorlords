#if !defined(_GAMEDATA_H)
#define _GAMEDATA_H

#include "Tools/ResourceCache.h"
#include "Tools/HashTable.h"
#include "UI/Entity.h"
#include "UI/Action.h"

#include "MFTranslation.h"

class GameData
{
public:
	static void Init();
	static void Deinit();

	static GameData *Get() { return pGameData; }

	void Update();
	void Draw();

	uiActionManager *GetActionManager() { return &actionManager; }
	uiEntityManager *GetEntityManager() { return &entityManager; }
	ResourceCache *GetResourceCache() { return &resourceCache; }

	int FindString(const char *pKey) { return MFTranslation_FindString(pStrings, pKey); }
	const char *GetString(int string) { return MFTranslation_GetString(pStrings, string); }

protected:
	GameData();
	~GameData();

	ResourceCache resourceCache;
	uiEntityManager entityManager;
	uiActionManager actionManager;

	MFStringTable *pStrings;

	static GameData *pGameData;
};

#endif