#if !defined(_BATTLE_H)
#define _BATTLE_H

#include "MFMaterial.h"

#include "Screen.h"
#include "Game.h"
#include "Unit.h"

struct Group;

class Battle : public Screen
{
public:
	Battle(GameData *pGameData);
	virtual ~Battle();

  void Begin(Group *pGroup1, Group *pGroup2, const char *pForeground, const char *pBackground, const char *pCastle);

	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual int UpdateInput();

protected:
	virtual void Select();

  enum UnitState
  {
    US_Cooldown,        // unit is waiting for cooldown to expire
    US_Engaged,         // unit is being engaged
    US_WaitingToEngage, // unit is waiting for target to be available
    US_Engaging,        // unit is engaging a unit
  };

  struct BattleUnit
  {
    Unit *pUnit;
    float cooldown;

    int UnitState;
    float stateTime;

    // engaging state
    BattleUnit *pTarget;

    float posX, posY;
    float curX, curY;
    float cooldownPos;

    BattleUnit *pCooldownNext;
  };

  struct Army
  {
    Group *pGroup;
    BattleUnit units[10];
    int numUnits;
  };

  UnitDefinitions *pUnitDefs;

  Army armies[2];
  BattleUnit *pCooldownList;

  MFMaterial *pForeground;
  MFMaterial *pBackground;
  MFMaterial *pCastle;

  // library
  MFMaterial **ppForegrounds;
  MFMaterial **ppBackgrounds;
  MFMaterial **ppCastles;
};

#endif
