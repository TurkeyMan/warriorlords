#include "Warlords.h"
#include "GameData.h"

GameData *GameData::pGameData = NULL;

GameData::GameData()
{
	pStrings = NULL;

	resourceCache.Init();
//	actionManager.Init();
//	entityManager.Init();

	MFFindData fd;
	MFFind *pFind = MFFileSystem_FindFirst("game:Map*.ini", &fd);
	while(pFind)
	{
		// allocate map details
		MapData &map = maps.push();
		map.name = fd.pFilename;
		map.name.TruncateExtension();
		map.pMap = NULL;

		if(!MFFileSystem_FindNext(pFind, &fd))
		{
			MFFileSystem_FindClose(pFind);
			pFind = NULL;
		}
	}
}

GameData::~GameData()
{
//	entityManager.Deinit();
//	actionManager.Deinit();
	resourceCache.Deinit();

	if(pStrings)
	{
		MFTranslation_DestroyStringTable(pStrings);
		pStrings = NULL;
	}
}

void GameData::Init()
{
//	uiActionManager::InitManager();
//	uiEntityManager::InitManager();
//	uiRuntimeArgs::Init();

	pGameData = new GameData;

//	Session::InitSession();
}

void GameData::Deinit()
{
//	Session::DeinitSession();

	delete pGameData;
	pGameData = NULL;

//	uiRuntimeArgs::Deinit();
//	uiActionManager::DeinitManager();
//	uiEntityManager::DeinitManager();
}

void GameData::Update()
{
//	entityManager.Update();
//	actionManager.Update();
}

void GameData::Draw()
{
//	entityManager.Draw();
}

MFString GameData::GetMapList()
{
	MFString t = "{ \"";

	int numMaps = maps.size();
	for(int a=0; a<numMaps; ++a)
	{
		if(a > 0)
			t += "\", \"";
		t += maps[a].name;
	}

	return t + "\" }";
}
