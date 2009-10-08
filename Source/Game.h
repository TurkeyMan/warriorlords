#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"

class GameData
{
public:
  GameData(const char *pMap);
  ~GameData();

  Map *GetMap() { return pMap; }
  UnitDefinitions *GetUnitDefs() { return pUnitDefs; }

protected:
  Map *pMap;
  UnitDefinitions *pUnitDefs;

};

#endif
