#if !defined(_UNIT_H)
#define _UNIT_H

#include "Path.h"
#include "MFPtrList.h"

class MFIni;
struct MFMaterial;

class CastleSet;
class Game;
class MapTile;
class Group;

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
	uint8 width, height;
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
	int buildTime, cost;

	// visible info
	int x, y;
	int width, height;
};

class UnitDefinitions
{
public:
	static UnitDefinitions *Load(Game *pGame, const char *pUnits, int numTerrainTypes);
	void Free();

	Game *GetGame() { return pGame; }

	inline int GetNumRaces() { return raceCount; }
	inline const char *GetRaceName(int type) { return pRaces[type].pName; }
	MFVector GetRaceColour(int race) const;

	inline int GetNumUnitTypes() { return numUnits; }
	inline const char *GetUnitTypeName(int unit) { return pUnits[unit].pName; }

	UnitDetails *GetUnitDetails(int unit) { return &pUnits[unit]; }
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
	const char *GetMovementClassName(int movementClass) { return pMovementClasses[movementClass].pName; }
	int GetMovementPenalty(int movementClass, int terrainType) { return pMovementClasses[movementClass].pMovementPenalty[terrainType] * 2; }
	bool HasRoadWalk(int movementClass) { return pMovementClasses[movementClass].roadWalk != 0; }

	MFMaterial *GetUnitMaterial() { return pUnitMat; }

	// rendering
	void AddRenderUnit(int unit, float x, float y, int player = -1, bool bFlip = false, float alpha = 1.f);
	void DrawUnits(float scale, float texelOffset, bool bHead = false);

	int DrawCastle(int race);
	int DrawFlag(int race);
	int DrawSpecial(int index);

	void GetCastleUVs(int race, MFRect *pUVs, float texelOffset);
	void GetFlagUVs(int race, MFRect *pUVs, float texelOffset);
	void GetUnitUVs(int unit, bool bFlip, MFRect *pUVs, float texelOffset);
	void GetSpecialUVs(int index, MFRect *pUVs, float texelOffset);

	inline MFMaterial *GetCastleMaterial() const { return pCastleMat; }

protected:
	struct WeaponClass
	{
		const char *pName;
		int ranged;
		float *pAttackModifiers;
	};

	struct MovementClass
	{
		const char *pName;
		int roadWalk;
		int *pMovementPenalty;
	};

	Game *pGame;

	MFIni *pUnitDefs;

	const char *pName;

	MFMaterial *pUnitMat;
	MFMaterial *pHeadMat;
	MFMaterial *pCastleMat;

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

	MovementClass *pMovementClasses;
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

	UnitDefinitions *GetDefs() { return pUnitDefs; }

	void Draw(float x, float y, bool bFlip = false, float alpha = 1.f);

	const char *GetName() { return pName; }
	bool IsHero() { return details.type == UT_Hero; }
	bool IsVehicle() { return details.type == UT_Vehicle; }

	UnitDetails *GetDetails() { return &details; }
	int GetPlayer() { return player; }
	void SetPlayer(int player) { this->player = player; }
	int GetRace();
	MFVector GetColour();

	float GetHealth() { return (float)life / (float)details.life; }
	int Damage(int damage) { life -= MFMin(life, damage); return life; }
	bool IsDead() { return life == 0; }

	bool IsRanged() { return pUnitDefs->IsRanged(details.attackClass); }

	int GetMovement() { return movement; }
	int GetMovementPenalty(int terrainType);
	int GetMovementPenalty(MapTile *pTile);
	bool HasRoadWalk() { return pUnitDefs->HasRoadWalk(details.movementClass); }
	void Move(int penalty) { movement -= penalty; }

	void Restore();

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

struct BuildUnit
{
	int unit;
	int cost;
	int buildTimeMod;
};

struct CastleDetails
{
	const char *pName;
	int x, y;

	BuildUnit buildUnits[4];
	int numBuildUnits;

	int income;
};

class Castle
{
public:
	void Init(CastleDetails *pDetails, int player, UnitDefinitions *pUnitDefs);

	MapTile *GetTile(int index = 0);
	bool IsEmpty();

	Group *GetMercGroup();
	void Capture(int player);

	void SetBuildUnit(int slot);
	int GetBuildUnit();

	const char *GetName() { return details.pName; }
	void SetName(const char *pName) { MFString_Copy((char*)details.pName, pName); }

	int GetPlayer() { return player; }

//protected:
public:
	UnitDefinitions *pUnitDefs;
	MapTile *pTile;
//	Group *pMercGroup;

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

	bool AddUnit(Unit *pUnit);
	bool AddForwardUnit(Unit *pUnit);
	bool AddRearUnit(Unit *pUnit);
	void RemoveUnit(Unit *pUnit);
	void SwapUnits(Unit *pUnit1, Unit *pUnit2);

	int GetMovement();
	bool SubtractMovementCost(MapTile *pTile);

	bool IsSelected() { return bSelected; }
	bool IsInGroup(Unit *pUnit);

	int GetPlayer() { return player; }
	void SetPlayer(int player);

	MapTile *GetTile() { return pTile; }

	int GetNumUnits() const { return numForwardUnits + numRearUnits; }
	int GetNumForwardUnits() const { return numForwardUnits; }
	int GetNumRearUnits() const { return numRearUnits; }

	Unit *GetUnit(int unit) { return (unit >= numForwardUnits) ? pRearUnits[unit - numForwardUnits] : pForwardUnits[unit]; }
	Unit *GetForwardUnit(int forwardUnit) { return pForwardUnits[forwardUnit]; }
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
