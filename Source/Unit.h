#if !defined(_UNIT_H)
#define _UNIT_H

#include "Path.h"
#include "MFPtrList.h"

class MFIni;
struct MFMaterial;

class CastleSet;
class Game;
class MapTile;

struct Race
{
	const char *pName;
	uint8 castlex, castley;
	uint8 flagx, flagy;
	uint32 colour;
};

struct Special
{
	const char *pName;
	uint8 x, y;
	uint16 canSearch : 1;
	uint16 flags : 15;
};

enum UnitType
{
	UT_Unknown = -1,

	UT_Hero = 0,
	UT_Unit,
	UT_Vehicle,

	UT_Max
};

enum AttackPlan
{
	AP_AttackStrongest,
	AP_AttackWeakest,
	AP_AttackStrongestAvailable,
	AP_AttackWeakestAvailable,
	AP_AttackFront,
	AP_AttackRear,

	AP_Max
};

struct UnitDetails
{
	const char *pName;
	UnitType type;
	int race;

	// stats
	int attackMin, attackMax, movement;
	int attackClass, defenceClass, movementClass;
	float cooldown, attackSpeed;
	int life;

	// visible info
	int x, y;
	int width, height;
};

class UnitDefinitions
{
public:
	static UnitDefinitions *Load(Game *pGame, const char *pUnits, int numTerrainTypes);
	void Free();

	inline int GetNumRaces() { return raceCount; }
	inline const char *GetRaceName(int type) { return pRaces[type].pName; }
	MFVector GetRaceColour(int race) const;

	inline int GetNumUnitTypes() { return numUnits; }
	inline const char *GetUnitTypeName(int unit) { return pUnits[unit].pName; }

	class Unit *CreateUnit(int unit, int player);
	void DestroyUnit(Unit *pUnits);

	inline int GetNumSpecials() { return specialCount; }
	inline const char *GetSpecialName(int type) { return pSpecials[type].pName; }

	int GetNumArmourClasses() { return numArmourClasses; }
	const char *GetArmourClassName(int armourClass) { return ppArmourClasses[armourClass]; }

	int GetNumWeaponClasses() { return numWeaponClasses; }
	const char *GetWeaponClassName(int weaponClass) { return pWeaponClasses[weaponClass].pName; }
	float GetDamageModifier(int weaponClass, int armourClass) { return pWeaponClasses[weaponClass].pAttackModifiers[armourClass]; }
	bool IsRanged(int weaponClass) { return pWeaponClasses[weaponClass].ranged != 0; }

	int GetNumMovementClasses() { return numMovementClasses; }
	const char *GetMovementClassName(int movementClass) { return ppMovementClasses[movementClass]; }
	int GetMovementPenalty(int movementClass, int terrainType) { return pMovementPenalty[movementClass*numTerrainTypes + terrainType]; }

	// rendering
	void AddRenderUnit(int unit, float x, float y, int player = -1, bool bFlip = false, float alpha = 1.f);
	void DrawUnits(float scale = 1.f, float texelOffset = 0.5f, bool bHead = false);

	int DrawCastle(int race);
	int DrawFlag(int race);
	int DrawSpecial(int index);

	void GetCastleUVs(int race, MFRect *pUVs, float texelOffset = 0.5f);
	void GetFlagUVs(int race, MFRect *pUVs, float texelOffset = 0.5f);
	void GetUnitUVs(int unit, bool bFlip, MFRect *pUVs, float texelOffset = 0.5f);
	void GetSpecialUVs(int index, MFRect *pUVs, float texelOffset = 0.5f);

	inline MFMaterial *GetCastleMaterial() const { return pCastleMap; }

protected:
	struct WeaponClass
	{
		const char *pName;
		int ranged;
		float *pAttackModifiers;
	};

	Game *pGame;

	MFIni *pUnitDefs;

	const char *pName;

	MFMaterial *pColourLayer;
	MFMaterial *pDetailLayer;
	MFMaterial *pUnitHeads;
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

	const char **ppArmourClasses;
	int numArmourClasses;

	WeaponClass *pWeaponClasses;
	int numWeaponClasses;

	const char **ppMovementClasses;
	int *pMovementPenalty;
	int numMovementClasses;
	int numTerrainTypes;

	MFPtrListDL<Unit> units;

	// render list
	struct UnitRender
	{
		int unit;
		float x, y;
		int player;
		float alpha;
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

	void Draw(float x, float y, bool bFlip = false, float alpha = 1.f);

	const char *GetName() { return pName; }
	bool IsHero() { return details.type == UT_Hero; }
	bool IsVehicle() { return details.type == UT_Vehicle; }

	UnitDetails *GetDetails() { return &details; }
	int GetPlayer() { return player; }
	int GetRace();
	MFVector GetColour();

	float GetHealth() { return (float)life / (float)details.life; }
	int Damage(int damage) { life -= MFMin(life, damage); return life; }

	bool IsRanged() { return pUnitDefs->IsRanged(details.attackClass); }

protected:
	UnitDefinitions *pUnitDefs;
	int id;

	const char *pName;
	int player;

	UnitDetails details;
	int movement;
	int life;

	// unit stats
	int kills, victories;
};

struct CastleDetails
{
	const char *pName;
	int x, y;

	int buildUnits[4];
	int numBuildUnits;

	int income;
};

class Castle
{
public:
	void Init(CastleDetails *pDetails, int player, UnitDefinitions *pUnitDefs);

	UnitDefinitions *pUnitDefs;

	CastleDetails details;
	int player;

	int building;   // the current unit being built
	int buildTime;  // how many turns this unit has been in production
};

class Group
{
public:
	static Group *Create(int player);
	void Destroy();

	int GetPlayer() { return player; }

	MapTile *GetTile() { return pTile; }

	bool IsSelected() { return bSelected; }

	void AddUnit(Unit *pUnit);
	void RemoveUnit(Unit *pUnit);

	int GetNumUnits() const { return numForwardUnits + numRearUnits; }
	int GetNumForwardUnits() const { return numForwardUnits; }
	int GetNumRearUnits() const { return numRearUnits; }

	Unit *GetUnit(int unit) { return (unit >= numForwardUnits) ? pRearUnits[unit - numForwardUnits] : pForwardUnits[unit]; }
	Unit *GetForwardUnit(int forwardUnit) { pForwardUnits[forwardUnit]; }
	Unit *GetRearUnit(int rearUnit) { return pRearUnits[rearUnit]; }

	Unit *GetFeatureUnit() { return GetUnit(0); }
	Unit *GetVehicle() { return pVehicle; }

	Step *GetPath() { return pPath; }

//protected:
	int player;
	MapTile *pTile;

	Unit *pVehicle;

	Unit *pForwardUnits[5];
	Unit *pRearUnits[5];

	int numForwardUnits;
	int numRearUnits;

	AttackPlan forwardPlan;
	AttackPlan rearPlan;

	bool bSelected;
	Step *pPath;

	Group *pNext;
};

#endif
