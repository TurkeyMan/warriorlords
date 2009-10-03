#if !defined(_UNIT_H)
#define _UNIT_H

class MFIni;
struct MFMaterial;

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
  int movement;
  int attackClass, defenceClass;
  int strength, defence;
  int attackSpeed;
  int life;

  // visible info
  int x, y;
  int width, height;
};

class UnitDefinitions
{
public:
  UnitDefinitions *Load(const char *pUnits);
  void Free();

  class Unit *CreateUnit(int unit, int race);

  void AddRenderUnit(int unit, float x, float y);
  void DrawUnits(float texelOffset = 0.5f);

protected:
  MFMaterial *pColourLayer;
  MFMaterial *pDetailLayer;
  int tileWidth, tileHeight;

  MFIni *pUnitDefs;

  UnitDetails *pUnits;
  int numUnits;

  struct UnitRender
  {
    int unit;
    float x, y;
  };

  UnitRender renderUnits[256];
  int numRenderUnits;
};

class Unit
{
  friend class UnitDefinitions;
public:
  void Destroy();

  void Draw(float x, float y);

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
