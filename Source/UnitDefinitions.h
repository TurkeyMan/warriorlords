#pragma once
#if !defined(_UNITDEFS_H)
#define _UNITDEFS_H

#include "Fuji/MFResource.h"

class GameState;
class Unit;

struct Race
{
	MFString name;
	uint8 castlex, castley;
	uint8 flagx, flagy;
	uint32 colour;
};

struct Special
{
	enum Type
	{
		ST_Searchable,
		ST_Recruit
	};

	MFString name;
	uint8 x, y;
	uint8 width, height;
	Type type;
	int index;
};

struct Weapon
{
	MFString name;
	bool bIsProjectile;
	int x, y;
	int width, height;
	int impactAnim;

	inline void GetUVs(MFRect *pRect, float tWidth, float tHeight, bool bFlip, float texelOffset) const
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
	enum ModType
	{
		MT_Stat = 0,
		MT_Defence,
		MT_Terrain
	};

	enum StatMods
	{
		Mod_MinAtk = 0,
		Mod_MaxAtk,
		Mod_Cool,
		Mod_Life,
		Mod_Regen,
		Mod_Movement,

		Mod_Max
	};

	struct StatMod
	{
		StatMod() : value(0.f), flags(0) {}
		void Parse(const char *pString);

		enum Flags
		{
			SMF_Absolute = 1,
			SMF_Percent = 2,
		};

		float value;
		uint32 flags;
	};

	const StatMod *GetMod(const Unit *pUnit, const Unit *pHero, int type, int index) const;
	float GetSpecial(const Unit *pUnit, const Unit *pHero, MFString name) const;

	MFString name;
	MFString description;
	int x, y;
	bool bCollectible;

	struct GroupMod
	{
		MFArray<MFString> targets;

		StatMod mods[Mod_Max];
		StatMod defence[8];
		StatMod terrain[12];

		MFString special;
		float probability;
	};

	MFArray<GroupMod> mods;
};

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
	MFString name;
	MFString description;
	UnitType type;
	int race;

	// stats
	int attackMin, attackMax, movement;
	int movementClass;
	int atkType, attack, armour;
	float cooldown, attackSpeed;
	int life;
	int buildTime;
	int weapon;

	// visible info
	int x, y;
	int width, height;

	// start items
	static const int MaxItems = 1;
	int items[MaxItems];
	int numItems;

	const char *AttackSpeedDescription() const;
};

class UnitDefinitions : public MFResource
{
public:
	static void Init();

	static UnitDefinitions *Create(MFString unitSetName, int numTerrainTypes);

	int AddRef();
	int Release();

	void LoadResources();
	void ReleaseResources();

	inline int GetNumRaces() const { return races.size(); }
	inline MFString GetRaceName(int race) const { return races[race].name; }
	MFVector GetRaceColour(int race) const;
	int FindRace(MFString name) const;
	int GetNumHeroesForRace(int race) const;
	int GetHeroForRace(int race, int heroIndex) const;

	inline int GetNumUnitTypes() const { return units.size(); }
	inline UnitType GetUnitType(int unit) const { return units[unit].type; }
	inline MFString GetUnitTypeName(int unit) const { return units[unit].name; }
	int FindUnit(MFString name) const;

	const UnitDetails& GetUnitDetails(int unit) const { return units[unit]; }

	Unit *CreateUnit(int unit, int player, GameState *pGameState) const;

	inline int GetNumSpecials() const { return specials.size(); }
	inline MFString GetSpecialName(int type) const { return specials[type].name; }
	inline const Special& GetSpecial(int type) const { return specials[type]; }

	int GetNumAttackTypes() const { return types.size(); }
	MFString GetAttackTypeName(int type) const { return types[type]; }

	int GetNumArmourClasses() const { return defenceClasses.size(); }
	MFString GetArmourClassName(int armourClass) const { return defenceClasses[armourClass].name; }

	int GetNumWeaponClasses() const { return attackClasses.size(); }
	MFString GetWeaponClassName(int weaponClass) const { return attackClasses[weaponClass]; }
	float GetDamageModifier(int weaponClass, int armourClass) const { return defenceClasses[armourClass].damageMods[weaponClass]; }
	bool IsRanged(int type) const { return type > 0; }

	int GetNumMovementClasses() const { return movementClasses.size(); }
	MFString GetMovementClassName(int movementClass) const { return movementClasses[movementClass].name; }
	int GetMovementPenalty(int movementClass, int terrainType) const { return movementClasses[movementClass].movementPenalty[terrainType] * 2; }
	bool HasRoadWalk(int movementClass) const { return movementClasses[movementClass].roadWalk != 0; }

	int GetNumWeapons() const { return weapons.size(); }
	const Weapon& GetWeapon(int weapon) const { return weapons[weapon]; }

	int GetNumItems() const { return items.size(); }
	const Item& GetItem(int item) const { return items[item]; }
	int FindItem(MFString name) const;

	// rendering
	MFRect GetCastleUVs(int race, float texelOffset) const;
	MFRect GetFlagUVs(int race, float texelOffset) const;
	MFRect GetUnitUVs(int unit, bool bFlip, float texelOffset) const;
	MFRect GetSpecialUVs(int index, float texelOffset, int *pWidth = NULL, int *pHeight = NULL) const;
	MFRect GetItemUVs(int item, float texelOffset) const;
	MFRect GetBadgeUVs(int rank, float texelOffset) const;

	inline MFMaterial *GetCastleMaterial() const { return pCastleMat; }
	inline MFMaterial *GetMiscMaterial() const { return pMiscMat; }
	inline MFMaterial *GetItemMaterial() const { return pItemMat; }
	inline MFMaterial *GetUnitMaterial() const { return pUnitMat; }
	inline MFMaterial *GetUnitHeadsMaterial() const { return pHeadMat; }
	inline MFMaterial *GetRanksMaterial() const { return pRanks; }

protected:
	UnitDefinitions(MFString unitSetName, int numTerrainTypes);
	~UnitDefinitions();

	static void Destroy(MFResource *pRes);

	struct WeaponClass
	{
		MFString name;
		int ranged;
		float *pAttackModifiers;
	};

	struct MovementClass
	{
		MFString name;
		int roadWalk;
		MFArray<int> movementPenalty;
	};

	struct ArmourClass
	{
		MFString name;
		MFArray<float> damageMods;
	};

	MFString name;

	MFArray<Race> races;
	MFArray<UnitDetails> units;
	MFArray<Special> specials;

	MFArray<MFString> types;
	MFArray<MFString> attackClasses;
	MFArray<ArmourClass> defenceClasses;

	MFArray<MovementClass> movementClasses;

	MFArray<Weapon> weapons;
	MFArray<Item> items;

	int tileWidth, tileHeight;
	int itemWidth, itemHeight;
	int unitMapWidth, unitMapHeight;
	int castleMapWidth, castleMapHeight;
	int itemMapWidth, itemMapHeight;
	int miscMapWidth, miscMapHeight;

	MFString unitMat;
	MFString headMat;
	MFString castleMat;
	MFString itemMat;
	MFString miscMat;
	MFString rankMat;

	MFMaterial *pUnitMat;
	MFMaterial *pHeadMat;
	MFMaterial *pCastleMat;
	MFMaterial *pItemMat;
	MFMaterial *pMiscMat;
	MFMaterial *pRanks;

	int resourceRefCount;
};

#endif
