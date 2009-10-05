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

struct Race
{
	const char *pName;
	uint8 castlex, castley;
	uint8 flagx, flagy;
	uint32 colour;
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

struct Special
{
	const char *pName;
	uint8 x, y;
	uint16 canSearch : 1;
	uint16 flags : 15;
};

class UnitDefinitions
{
public:
  static UnitDefinitions *Load(const char *pUnits);
  void Free();

	inline int GetNumRaces() { return raceCount; }
	inline const char *GetRaceName(int type) { return pRaces[type].pName; }
	MFVector GetRaceColour(int race) const;

  inline int GetNumUnits() { return numUnits; }
	inline const char *GetUnitName(int unit) { return pUnits[unit].pName; }

  class Unit *CreateUnit(int unit, int race);

  inline int GetNumSpecials() { return specialCount; }
	inline const char *GetSpecialName(int type) { return pSpecials[type].pName; }

  // rendering
  void AddRenderUnit(int unit, float x, float y, int race = -1, bool bFlip = false);
  void DrawUnits(float texelOffset = 0.5f);

	int DrawCastle(int race);
	int DrawFlag(int race);
  int DrawSpecial(int index);

	void GetCastleUVs(int race, MFRect *pUVs, float texelOffset = 0.5f);
	void GetFlagUVs(int race, MFRect *pUVs, float texelOffset = 0.5f);
	void GetUnitUVs(int unit, bool bFlip, MFRect *pUVs, float texelOffset = 0.5f);
  void GetSpecialUVs(int index, MFRect *pUVs, float texelOffset = 0.5f);

	inline MFMaterial *GetCastleMaterial() const { return pCastleMap; }

protected:
  MFIni *pUnitDefs;

  const char *pName;

  MFMaterial *pColourLayer;
  MFMaterial *pDetailLayer;
  MFMaterial *pCastleMap;

  int tileWidth, tileHeight;
  int unitMapWidth, unitMapHeight;
  int castleMapWidth, castleMapHeight;

	Race *pRaces;
	int raceCount;

  UnitDetails *pUnits;
  int numUnits;

	Special *pSpecials;
	int specialCount;

  // render list
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
