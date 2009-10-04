#if !defined(_UNIT_H)
#define _UNIT_H

class MFIni;
struct MFMaterial;

class CastleSet;

enum UnitType
{
  UT_Unknown = -1,

  UT_Hero = 0,
  UT_Unit,
  UT_Vehicle,

  UT_Max
};

struct UnitDetails
{
  const char *pName;
  UnitType type;
  int race;

  // stats
  int attack, defence, movement;
  int attackClass, defenceClass, movementClass;
  int attackSpeed;
  int life;

  // visible info
  int x, y;
  int width, height;
};

class UnitDefinitions
{
public:
  static UnitDefinitions *Load(const char *pUnits, const CastleSet *pCastleSet);
  void Free();

  class Unit *CreateUnit(int unit, int race);

  void AddRenderUnit(int unit, float x, float y, int race = -1, bool bFlip = false);
  void DrawUnits(float texelOffset = 0.5f);

protected:
  const char *pName;

  MFMaterial *pColourLayer;
  MFMaterial *pDetailLayer;
  int tileWidth, tileHeight;

  MFIni *pUnitDefs;
  const CastleSet *pCastles; // DELETE ME!!

  UnitDetails *pUnits;
  int numUnits;

  struct UnitRender
  {
    int unit;
    float x, y;
    int race;
    bool bFlip;
  };

  UnitRender renderUnits[256];
  int numRenderUnits;
};

class Unit
{
  friend class UnitDefinitions;
public:
  void Destroy();

  void Draw(float x, float y, bool bFlip = false);

  const char *GetName() { return pName; }
  bool IsHero() { return details.type == UT_Hero; }
  bool IsVehicle() { return details.type == UT_Vehicle; }

protected:
  UnitDefinitions *pUnitDefs;
  int id;

  const char *pName;
  int race;

  UnitDetails details;
  int movement;
  int life;

  // unit stats
  int kills, victories;
};

#endif
