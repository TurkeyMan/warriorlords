#include "Warlords.h"
#include "GameData.h"

GameData *GameData::pGameData = NULL;

GameData::GameData()
{
	pStrings = NULL;

	resourceCache.Init();
	actionManager.Init();
	entityManager.Init();
}

GameData::~GameData()
{
	entityManager.Deinit();
	actionManager.Deinit();
	resourceCache.Deinit();

	if(pStrings)
	{
		MFTranslation_DestroyStringTable(pStrings);
		pStrings = NULL;
	}
}

void GameData::Init()
{
	uiActionManager::InitManager();
	uiEntityManager::InitManager();
	uiRuntimeArgs::Init();

	pGameData = new GameData;
}

void GameData::Deinit()
{
	delete pGameData;
	pGameData = NULL;

	uiRuntimeArgs::Deinit();
	uiActionManager::DeinitManager();
	uiEntityManager::DeinitManager();
}

void GameData::Update()
{
	entityManager.Update();
	actionManager.Update();
}

void GameData::Draw()
{
	entityManager.Draw();
}
