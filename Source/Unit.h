#if !defined(_UNIT_H)
#define _UNIT_H

#include "Path.h"
#include "MFPoolHeap.h"

class MFIni;
struct MFMaterial;

class CastleSet;
class Game;
class MapTile;
class Unit;
class Group;
struct Action;

struct Race
{
	const char *pName;
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

	const char *pName;
	uint8 x, y;
	uint8 width, height;
	Type type;
	int index;
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
		void Parse(const char *pString);

		enum Flags
		{
			SMF_Absolute = 1,
			SMF_Percent = 2,
		};

		float value;
		uint32 flags;
	};

	StatMod *GetMod(Unit *pUnit, Unit *pHero, int type, int index);
	float GetSpecial(Unit *pUnit, Unit *pHero, const char *pName);

	const char *pName;
	const char *pDescription;
	int x, y;
	bool bCollectible;

	struct GroupMod
	{
		const char **ppTargets;
		int numUnits;

		StatMod mods[Mod_Max];
		StatMod defence[8];
		StatMod terrain[12];

		const char *pSpecial;
		float probability;
	};

	GroupMod *pMods;
	int numGroups;
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

struct UnitSetDetails
{
	char unitSetName[64];
	int numRaces;
	char races[16][64];
	uint32 colours[16];

	struct Unit
	{
		char name[64];
		UnitType type;
		int race;
	} units[256];
	int numUnits;
};

struct UnitDetails
{
	const char *pName;
	const char *pDescription;
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

	const char *AttackSpeedDescription();
};

class UnitDefinitions
{
public:
	static bool GetDetails(const char *pUnitSetName, UnitSetDetails *pDetails);

	static UnitDefinitions *Load(Game *pGame, const char *pUnitSetName, int numTerrainTypes);
	void Free();

	Game *GetGame() { return pGame; }

	inline int GetNumRaces() { return raceCount; }
	inline const char *GetRaceName(int race) { return pRaces[race].pName; }
	MFVector GetRaceColour(int race) const;
	int FindRace(const char *pName);

	inline int GetNumUnitTypes() { return numUnits; }
	inline UnitType GetUnitType(int unit) { return pUnits[unit].type; }
	inline const char *GetUnitTypeName(int unit) { return pUnits[unit].pName; }
	int FindUnit(const char *pName);

	UnitDetails *GetUnitDetails(int unit) { return &pUnits[unit]; }
	class Unit *CreateUnit(int unit, int player);
	void DestroyUnit(Unit *pUnit);

	inline int GetNumSpecials() { return specialCount; }
	inline const char *GetSpecialName(int type) { return pSpecials[type].pName; }
	inline const Special *GetSpecial(int type) { return &pSpecials[type]; }

	int GetNumAttackTypes() { return numTypes; }
	const char *GetAttackTypeName(int type) { return ppTypes[type]; }

	int GetNumArmourClasses() { return numDefenceClasses; }
	const char *GetArmourClassName(int armourClass) { return pDefenceClasses[armourClass].pName; }

	int GetNumWeaponClasses() { return numAttackClasses; }
	const char *GetWeaponClassName(int weaponClass) { return ppAttackClasses[weaponClass]; }
	float GetDamageModifier(int weaponClass, int armourClass) { return pDefenceClasses[armourClass].pDamageMods[weaponClass]; }
	bool IsRanged(int type) { return type > 0; }

	int GetNumMovementClasses() { return numMovementClasses; }
	const char *GetMovementClassName(int movementClass) { return pMovementClasses[movementClass].pName; }
	int GetMovementPenalty(int movementClass, int terrainType) { return pMovementClasses[movementClass].pMovementPenalty[terrainType] * 2; }
	bool HasRoadWalk(int movementClass) { return pMovementClasses[movementClass].roadWalk != 0; }

	int GetNumWeapons() { return numWeapons; }
	Weapon *GetWeapon(int weapon) { return &pWeapons[weapon]; }

	int GetNumItems() { return numItems; }
	Item *GetItem(int item) { return &pItems[item]; }
	int FindItem(const char *pName);

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
	void GetSpecialUVs(int index, MFRect *pUVs, float texelOffset, int *pWidth = NULL, int *pHeight = NULL);
	void GetItemUVs(int item, MFRect *pUVs, float texelOffset);
	void GetBadgeUVs(int rank, MFRect *pUVs, float texelOffset);

	inline MFMaterial *GetCastleMaterial() const { return pCastleMat; }
	inline MFMaterial *GetMiscMaterial() const { return pMiscMat; }
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

	struct ArmourClass
	{
		const char *pName;
		float *pDamageMods;
	};

	Game *pGame;

	MFIni *pUnitDefs;

	const char *pName;

	MFMaterial *pUnitMat;
	MFMaterial *pHeadMat;
	MFMaterial *pCastleMat;
	MFMaterial *pItemMat;
	MFMaterial *pMiscMat;
	MFMaterial *pRanks;

	int tileWidth, tileHeight;
	int itemWidth, itemHeight;
	int unitMapWidth, unitMapHeight;
	int castleMapWidth, castleMapHeight;
	int itemMapWidth, itemMapHeight;
	int miscMapWidth, miscMapHeight;

	Race *pRaces;
	int raceCount;

	UnitDetails *pUnits;
	int numUnits;

	Special *pSpecials;
	int specialCount;

	const char **ppTypes;
	int numTypes;
	const char **ppAttackClasses;
	int numAttackClasses;
	ArmourClass *pDefenceClasses;
	int numDefenceClasses;

	MovementClass *pMovementClasses;
	int numMovementClasses;
	int numTerrainTypes;

	Weapon *pWeapons;
	int numWeapons;

	Item *pItems;
	int numItems;

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
	friend class Group;
public:
	void Destroy();

	UnitDefinitions *GetDefs() { return pUnitDefs; }

	void Draw(float x, float y, bool bFlip = false, float alpha = 1.f);

	Group *GetGroup() { return pGroup; }

	const char *GetName() const { return pName; }
	bool IsHero() const { return details.type == UT_Hero; }
	bool IsVehicle() const { return details.type == UT_Vehicle; }

	uint32 GetID() { return id; }
	void SetID(uint32 _id) { id = _id; }

	void SetGroup(Group *_pGroup);// { pGroup = _pGroup; }

	UnitDetails *GetDetails() { return &details; }
	int GetPlayer() const { return player; }
	void SetPlayer(int player) { this->player = player; }
	int GetRace();
	int GetType() { return type; }
	MFVector GetColour();
	int GetRank() const { return MFMin(victories / 2, 8); }
	float GetSpecialAttack(const char *pSpecial);

	BattlePlan *GetBattlePlan() { return &plan; }
	Weapon *GetWeapon() { return pUnitDefs->GetWeapon(details.weapon); }

	int GetHP() { return life; }
	void SetHP(int hp) { life = hp; }
	float GetHealth() { return (float)life / (float)GetMaxHP(); }
	int Damage(int damage) { life = MFClamp(0, life - damage, lifeMax); return life; }
	bool IsDead() { return life == 0; }

	int GetKills() { return kills; }
	int GetVictories() { return victories; }
	void AddKill() { ++kills; }
	void AddVictory() { ++victories; UpdateStats(); }
	void SetKills(int numKills) { kills = numKills; }

	bool IsRanged() { return pUnitDefs->IsRanged(details.atkType); }

	void SetMovement(int _movement) { movement = _movement; }
	int GetMovement() { return movement; }
	float MoveRemaining() { return MFMax((float)movement*0.5f, 0.f); }
	int GetTerrainPenalty(int terrainType);
	void GetTerrainPenalties(int *pTerrainPenalties);
	int GetMovementPenalty(MapTile *pTile, int *pTerrainType = NULL);
	bool HasRoadWalk() { return pUnitDefs->HasRoadWalk(details.movementClass); }
	void Move(int penalty) { movement -= penalty; }

	void Restore();
	void Revive();

	int GetNumItems() { return numItems; }
	int GetItemID(int item) { return pItems[item]; }
	Item *GetItem(int item) { return pUnitDefs->GetItem(pItems[item]); }
	bool AddItem(int item);

	// stats
	float GetMinDamage() { return minAtk; }
	float GetMaxDamage() { return maxAtk; }
	int GetMaxMovement() { return movementMax; }
	int GetMaxHP() { return lifeMax; }
	float GetCooldown();
	float GetRegen();

	const char *AttackSpeedDescription();

	float GetDefence(float damage, int wpnClass);

	void UpdateStats();

protected:
	int ModStatInt(int stat, int statType, int modIndex);
	float ModStatFloat(float stat, int statType, int modIndex);

	static const int MaxItems = 8;

	UnitDefinitions *pUnitDefs;

	uint32 id;

	const char *pName;
	int type;
	int player;

	UnitDetails details;
	int movement, movementMax;
	int life, lifeMax;
	int kills, victories;
	float minAtk, maxAtk;

	BattlePlan plan;

	int *pItems;
	int numItems;

	Group *pGroup;
};

struct BuildUnit
{
	int unit;
	int buildTime;
};

struct CastleDetails
{
	char name[32];
	int x, y;
	bool bCapital;

	BuildUnit buildUnits[4];
	int numBuildUnits;
};

class Castle
{
public:
	void Init(int id, const CastleDetails &details, int player);

	MapTile *GetTile(int index = 0);
	bool IsEmpty();

	Group *GetMercGroup();
	void Capture(Group *pGroup);

	void ClearBuildUnit();
	void SetBuildUnit(int slot);
	int GetBuildUnit();
	int GetUnitBuildTime();
	int BuildTimeRemaining();

	const char *GetName() { return details.name; }
	void SetName(const char *pName) { MFString_Copy(details.name, pName); }

	int GetPlayer() { return player; }

//protected:
	UnitDefinitions *pUnitDefs;
	MapTile *pTile;
//	Group *pMercGroup;

	CastleDetails details;

	int id;
	int player;

	int building;   // the current unit being built
	int buildTime;  // how many turns this unit has been in production
	int nextBuild;
};

class Place
{
public:
	const Special *GetPlaceDesc() const { return pSpecial; }
	Special::Type GetType() const { return pSpecial->type; }
	int GetID() const { return id; }
	int GetRenderID() const { return pSpecial->index; }

	void InitRuin(int item);


//protected:
	UnitDefinitions *pUnitDefs;
	const Special *pSpecial;

	MapTile *pTile;

	int id;

	union
	{
		struct
		{
			int item;
			bool bHasSearched;
		} ruin;

		struct
		{
			int building;
			int turnsRemaining;
		} recruit;
	};
};

class Group
{
public:
	static Group *Create(int player);
	void Destroy();
 
	uint32 GetID() { return id; }
	void SetID(uint32 _id) { id = _id; }

	bool AddUnit(Unit *pUnit);
	bool AddForwardUnit(Unit *pUnit);
	bool AddRearUnit(Unit *pUnit);
	void RemoveUnit(Unit *pUnit);

	int GetMovement();
	float MoveRemaining() { return MFMax((float)GetMovement()*0.5f, 0.f); }
	bool SubtractMovementCost(MapTile *pTile);

	bool IsSelected() const { return bSelected; }
	bool IsInGroup(Unit *pUnit);
	Unit *GetHero();

	int GetPlayer() const { return player; }
	void SetPlayer(int player);

	MapTile *GetTile() const { return pTile; }

	int GetNumUnits() const { return numForwardUnits + numRearUnits; }
	int GetNumForwardUnits() const { return numForwardUnits; }
	int GetNumRearUnits() const { return numRearUnits; }

	Unit *GetUnit(int unit) const { return (unit >= numForwardUnits) ? pRearUnits[unit - numForwardUnits] : pForwardUnits[unit]; }
	Unit *GetForwardUnit(int forwardUnit) const { return pForwardUnits[forwardUnit]; }
	Unit *GetRearUnit(int rearUnit) const { return pRearUnits[rearUnit]; }

	Unit *GetFeatureUnit() const { return GetUnit(0); }
	Unit *GetVehicle() const { return pVehicle; }

	void FindPath(int x, int y);
	Path *GetPath() const { return pPath; }

	Action *GetLastAction() { return pLastAction; }

	bool ValidateGroup();

//protected:
	void UpdateGroupStats();

	int id;

	int player;
	MapTile *pTile;
	int x, y;

	union
	{
		Unit *pUnits[11];
		struct
		{
			Unit *pForwardUnits[5];
			Unit *pRearUnits[5];
			Unit *pVehicle;
		};
	};

	int numForwardUnits;
	int numRearUnits;

	bool bSelected;
	int pathX, pathY;
	Path *pPath;

	Action *pLastAction;

	Group *pNext;
};

#endif
