#include "Warlords.h"
#include "Game.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"

#include "Path.h"

GameData::GameData(const char *_pMap)
{
  pMap = Map::Create("Map");
  pUnitDefs = pMap->GetUnitDefinitions();
}

GameData::~GameData()
{

}
