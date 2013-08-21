#pragma once
#if !defined(_UNIT_H)
#define _UNIT_H

#include "MapTemplate.h"
#include "UnitDefinitions.h"
#include "Path.h"

class MFIni;
struct MFMaterial;

class CastleSet;
class GameState;
class MapTile;
class Unit;
class Group;
struct PendingAction;

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

struct UnitRender
{
public:
	int unit;
	float x, y;
	int player;
	int rank;
	float alpha;
	bool bFlip;
};

class Unit
{
	friend class UnitDefinitions;
	friend class Group;
	friend class GameState;
//	friend class Game;
public:
	static void Init();
	static void Deinit();

	void* operator new(size_t);
	void operator delete(void*);

	Unit(GameState &gameState)			: gameState(gameState) {}
	~Unit();

	GameState& GameState()				{ return gameState; }
	const UnitDefinitions& UnitDefs() const;

	UnitRender Render(float x, float y, bool bFlip = false, float alpha = 1.f);

	Group *GetGroup() const				{ return pGroup; }

	MFString GetName() const			{ return name; }
	bool IsHero() const					{ return details.type == UT_Hero; }
	bool IsVehicle() const				{ return details.type == UT_Vehicle; }
	bool IsType(MFString target) const;

	uint32 GetID() const				{ return id; }
	void SetID(uint32 _id)				{ id = _id; }

	void SetGroup(Group *_pGroup)		{ pGroup = _pGroup; }

//	UnitDetails *GetDetails()			{ return &details; }
	const UnitDetails& GetDetails() const	{ return details; }
	int GetPlayer() const				{ return player; }
	void SetPlayer(int player)			{ this->player = player; }
	int GetRace() const;
	int GetType() const					{ return type; }
	MFVector GetColour() const;
	int GetRank() const					{ return MFMin(victories / 2, 8); }
	float GetSpecialAttack(MFString pSpecial) const;

	BattlePlan *GetBattlePlan()			{ return &plan; }
	const Weapon& GetWeapon() const		{ return UnitDefs().GetWeapon(details.weapon); }

	int GetHP() const					{ return life; }
	void SetHP(int hp)					{ life = hp; }
	float GetHealth() const				{ return (float)life / (float)GetMaxHP(); }
	int Damage(int damage)				{ life = MFClamp(0, life - damage, lifeMax); return life; }
	bool IsDead() const					{ return life == 0; }

	int GetKills() const				{ return kills; }
	int GetVictories() const			{ return victories; }
	void AddKill()						{ ++kills; }
	void AddVictory()					{ ++victories; UpdateStats(); }
	void SetKills(int numKills)			{ kills = numKills; }

	bool IsRanged() const				{ return UnitDefs().IsRanged(details.atkType); }

	void SetMovement(int _movement)		{ movement = _movement; }
	int GetMovement() const				{ return movement; }
	float MoveRemaining() const			{ return MFMax((float)movement*0.5f, 0.f); }
	int GetTerrainPenalty(int terrainType) const;
	void GetTerrainPenalties(int *pTerrainPenalties) const;
	int GetMovementPenalty(MapTile *pTile, int *pTerrainType = NULL) const;
	bool HasRoadWalk() const			{ return UnitDefs().HasRoadWalk(details.movementClass); }
	void Move(int penalty)				{ movement -= penalty; }

	void Restore();
	void Revive();

	int GetNumItems() const				{ return items.size(); }
	int GetItemID(int item) const		{ return items[item]; }
	const Item& GetItem(int item) const	{ return UnitDefs().GetItem(items[item]); }
	bool AddItem(int item);

	// stats
	float GetMinDamage() const			{ return minAtk; }
	float GetMaxDamage() const			{ return maxAtk; }
	int GetMaxMovement() const			{ return movementMax; }
	int GetMaxHP() const				{ return lifeMax; }
	float GetCooldown() const;
	float GetRegen() const;

	const char *AttackSpeedDescription() const;

	float GetDefence(float damage, int wpnClass) const;

	void UpdateStats();

protected:
	int ModStatInt(int stat, int statType, int modIndex) const;
	float ModStatFloat(float stat, int statType, int modIndex) const;

	static const int MaxItems = 8;

	::GameState &gameState;

	uint32 id;

	MFString name;
	int type;
	int player;

	UnitDetails details;
	int movement, movementMax;
	int life, lifeMax;
	int kills, victories;
	float minAtk, maxAtk;

	BattlePlan plan;

	MFArray<int> items;

	Group *pGroup;

private:
	static MFObjectPool unitPool;
};

class Place
{
public:
	const Special *GetPlaceDesc() const	{ return pSpecial; }
	Special::Type GetType() const		{ return pSpecial->type; }
	int GetID() const					{ return id; }
	int GetRenderID() const				{ return pSpecial->index; }

//protected:
	const UnitDefinitions *pUnitDefs;
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
			Unit *pRecruitHero;
			int recruiting;
			int turnsRemaining;
		} recruit;
	};
};

#endif
