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

struct Weapon
{
	const char *pName;
	bool bIsProjectile;
	int x, y;
	int width, height;
	int impactAnim;

	void GetUVs(MFRect *pRect, float tWidth, float tHeight, bool bFlip, float texelOffset)
	{
		float xScale = gWeaponTileSize / tWidth;
		float yScale = gWeaponTileSize / tHeight;

		pRect->x = bFlip ? (x+width)*xScale - texelOffset : x*xScale + texelOffset;
		pRect->y = y*yScale + texelOffset;
		pRect->width = (bFlip ? -width : width)*xScale;
		pRect->height = height*yScale;
	}

	static const int gWeaponTileSize = 32;
};

struct Item
{
	struct StatMod
	{
		void Parse(const char *pString);

		float value;
		bool bAbsolute;
		bool bPercent;
	};

	const char *pName;
	int x, y;

	// personal buffs
	struct
	{
		StatMod minAtk, maxAtk;
		StatMod speed;
		StatMod hp;
		StatMod regen;
		StatMod def[2];
		StatMod movement;
	} user;

	// group buffs
	struct
	{
		StatMod minAtk, maxAtk;
		StatMod speed;
		StatMod hp;
		StatMod regen;
		StatMod def[2];
	} group;

	// movement
	StatMod terrain[10];
	StatMod vehicle[10];
};

enum UnitType
{
	UT_Unknown = -1,

	UT_Hero = 0,
	UT_Unit,
	UT_Vehicle,

	UT_Max
};

enum TargetType
{
	TT_Any,
	TT_Melee,
	TT_Ranged
};

enum TargetStrength
{
	TS_Strongest,
	TS_Weakest
};

struct BattlePlan
{
	TargetType type;
	TargetStrength strength;
	bool bAttackAvailable;
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
	int weapon;

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
	inline const char *GetRaceName(int race) { return pRaces[race].pName; }
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

	int GetNumWeapons() { return numWeapons; }
	Weapon *GetWeapon(int weapon) { return &pWeapons[weapon]; }

	int GetNumItems() { return numItems; }
	Item *GetItem(int item) { return &pItems[item]; }

	MFMaterial *GetUnitMaterial() { return pUnitMat; }

	// rendering
	void AddRenderUnit(int unit, float x, float y, int player = -1, bool bFlip = false, float alpha = 1.f, int rank = 0);
	void DrawUnits(float scale, float texelOffset, bool bHead = false, bool bRank = false);

	int DrawCastle(int race);
	int DrawFlag(int race);
	int DrawSpecial(int index);

	void GetCastleUVs(int race, MFRect *pUVs, float texelOffset);
	void GetFlagUVs(int race, MFRect *pUVs, float texelOffset);
	void GetUnitUVs(int unit, bool bFlip, MFRect *pUVs, float texelOffset);
	void GetSpecialUVs(int index, MFRect *pUVs, float texelOffset);
	void GetItemUVs(int item, MFRect *pUVs, float texelOffset);
	void GetBadgeUVs(int rank, MFRect *pUVs, float texelOffset);

	inline MFMaterial *GetCastleMaterial() const { return pCastleMat; }
	inline MFMaterial *GetItemMaterial() const { return pItemMat; }

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
	MFMaterial *pItemMat;
	MFMaterial *pBattle;

	int tileWidth, tileHeight;
	int itemWidth, itemHeight;
	int unitMapWidth, unitMapHeight;
	int castleMapWidth, castleMapHeight;
	int itemMapWidth, itemMapHeight;

	Race *pRaces;
	int raceCount;

	UnitDetails *pUnits;
	int numUnits;

	Special *pSpecials;
	int specialCount;

	const char **ppArmourClasses;
	int numArmourClasses;

	Weapon *pWeapons;
	int numWeapons;

	WeaponClass *pWeaponClasses;
	int numWeaponClasses;

	MovementClass *pMovementClasses;
	int numMovementClasses;
	int numTerrainTypes;

	Item *pItems;
	int numItems;

	MFPtrListDL<Unit> units;

	// render list
	struct UnitRender
	{
		int unit;
		float x, y;
		int player;
		int rank;
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

	const char *GetName() const { return pName; }
	bool IsHero() const { return details.type == UT_Hero; }
	bool IsVehicle() const { return details.type == UT_Vehicle; }

	UnitDetails *GetDetails() { return &details; }
	int GetPlayer() const { return player; }
	void SetPlayer(int player) { this->player = player; }
	int GetRace();
	MFVector GetColour();
	int GetRank() const { return MFMin(victories / 2, 8); }

	BattlePlan *GetBattlePlan() { return &plan; }
	Weapon *GetWeapon() { return pUnitDefs->GetWeapon(details.weapon); }

	float GetHealth() { return (float)life / (float)details.life; }
	int Damage(int damage) { life -= MFMin(life, damage); return life; }
	bool IsDead() { return life == 0; }

	int GetKills() { return kills; }
	int GetVictories() { return victories; }
	void AddKill() { ++kills; }
	void AddVictory() { ++victories; }

	bool IsRanged() { return pUnitDefs->IsRanged(details.attackClass); }

	int GetMovement() { return movement; }
	int GetTerrainPenalty(int terrainType);
	int GetMovementPenalty(MapTile *pTile);
	bool HasRoadWalk() { return pUnitDefs->HasRoadWalk(details.movementClass); }
	void Move(int penalty) { movement -= penalty; }

	void Restore();
	void Revive();

	int GetNumItems() { return numItems; }
	int GetItemID(int item) { return pItems[item]; }
	Item *GetItem(int item) { return pUnitDefs->GetItem(pItems[item]); }
	bool AddItem(int item);

	// stats
	int GetMinDamage();
	int GetMaxDamage();
	int GetMaxMovement();
	int GetMaxHP();

protected:
	UnitDefinitions *pUnitDefs;
	int id;

	const char *pName;
	int player;

	UnitDetails details;
	int movement;
	int life;
	int kills, victories;

	BattlePlan plan;

	int *pItems;
	int numItems;
};

struct BuildUnit
{
	int unit;
	int cost;
	int buildTime;
};

struct CastleDetails
{
	char name[32];
	int x, y;

	BuildUnit buildUnits[4];
	int numBuildUnits;

	int income;
};

class Castle
{
public:
	void Init(const CastleDetails &details, int player);

	MapTile *GetTile(int index = 0);
	bool IsEmpty();

	Group *GetMercGroup();
	void Capture(int player);

	void SetBuildUnit(int slot);
	int GetBuildUnit();

	const char *GetName() { return details.name; }
	void SetName(const char *pName) { MFString_Copy(details.name, pName); }

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
	Unit *GetHero();

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

	bool bSelected;
	Step *pPath;

	Group *pNext;
};

#endif
