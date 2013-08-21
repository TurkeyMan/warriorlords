#include "Warlords.h"
#include "Unit.h"
#include "Group.h"

#include "Fuji/MFMaterial.h"
#include "Fuji/Materials/MFMat_Standard.h"
#include "Fuji/MFTexture.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFIni.h"

MFObjectPool Unit::unitPool;

void Unit::Init()
{
	unitPool.Init(sizeof(Unit), 256, 256);
}

void Unit::Deinit()
{
	unitPool.Deinit();
}

void* Unit::operator new(size_t size)
{
	MFDebug_Assert(size <= unitPool.GetObjectSize(), "Object is too small?!");
	return unitPool.Alloc();
}

void Unit::operator delete(void *pUnit)
{
	unitPool.Free(pUnit);
}

Unit::~Unit()
{
	if(pGroup)
		pGroup->RemoveUnit(this);
}

const UnitDefinitions *Unit::UnitDefs() const
{
	return gameState.Map().UnitDefs();
}

UnitRender Unit::Render(float x, float y, bool bFlip, float alpha)
{
	UnitRender u;
	u.unit = type;
	u.x = x;
	u.y = y;
	u.player = player;
	u.alpha = alpha;
	u.bFlip = bFlip;
	u.rank = GetRank();
	return u;
}

bool Unit::IsType(MFString target) const
{
	const UnitDefinitions *pDefs = UnitDefs();
	return (GetType() == UT_Unit && target == "units") ||
			(GetType() == UT_Hero && target == "heroes") ||
			(GetType() == UT_Vehicle && target == "vehicles") ||
			target == GetName() ||
			target == pDefs->GetRaceName(GetRace()) ||
			target == pDefs->GetMovementClassName(GetDetails().movementClass) ||
			target == pDefs->GetUnitTypeName(GetDetails().atkType) ||
			target == pDefs->GetArmourClassName(GetDetails().armour);
}

int Unit::GetRace() const
{
	return gameState.PlayerRace(player);
}

MFVector Unit::GetColour() const
{
	return gameState.PlayerColour(player);
}

float Unit::GetSpecialAttack(MFString special) const
{
	float percent = 0.f;
	for(size_t a=0; a<items.size(); ++a)
	{
		const Item &item = UnitDefs()->GetItem(items[a]);

		Unit *pHero = pGroup->GetHero();
		percent += item.GetSpecial(this, pHero, special);
	}
	return percent;
}

int Unit::GetTerrainPenalty(int terrainType) const
{
	int penalty = UnitDefs()->GetMovementPenalty(details.movementClass, terrainType);
	return ModStatInt(penalty >> 1, Item::MT_Terrain, terrainType) << 1;
}

void Unit::GetTerrainPenalties(int *pTerrainPenalties) const
{
	int numTerrainTypes = gameState.Map().Tileset().NumTerrainTypes();
	for(int a=0; a<numTerrainTypes; ++a)
		pTerrainPenalties[a] = GetTerrainPenalty(a);
}

int Unit::GetMovementPenalty(MapTile *pTile, int *pTerrainType) const
{
	int terrainPenalties[64];
	GetTerrainPenalties(terrainPenalties);
	return Map::GetMovementPenalty(pTile, terrainPenalties, player, HasRoadWalk(), pTerrainType);
}

void Unit::Restore()
{
	// restore movement
	movement = GetMaxMovement() * 2;

	// regen health
	life = MFMin(life + (int)MFCeil((float)lifeMax * GetRegen()), lifeMax);
}

void Unit::Revive()
{
	movement = GetMaxMovement() * 2;
	life = GetMaxHP();

	victories = kills = 0;
}

bool Unit::AddItem(int item)
{
	if(items.size() >= 8)
		return false;

	items.push(item);

	if(pGroup)
		pGroup->UpdateGroupStats();
	else
		UpdateStats();

	return true;
}

float Unit::GetCooldown() const
{
	return ModStatFloat(details.cooldown, Item::MT_Stat, Item::Mod_Cool);
}

float Unit::GetRegen() const
{
	float regenMod = ModStatFloat(1.f, Item::MT_Stat, Item::Mod_Regen);
	return (IsHero() ? 0.4f : 0.25f) * regenMod;
}

const char *Unit::AttackSpeedDescription() const
{
	float attackTime = GetCooldown() + details.attackSpeed;
	if(attackTime <= 3.f)
		return "Very Fast ";
	else if(attackTime <= 4.f)
		return "Fast ";
	else if(attackTime <= 6.f)
		return "Moderate ";
	else if(attackTime <= 8.f)
		return "Slow ";
	else
		return "Very Slow ";
}

float Unit::GetDefence(float damage, int wpnClass) const
{
	damage = ModStatFloat(damage, Item::MT_Defence, wpnClass);

	if(player != -1 && pGroup && pGroup->GetTile())
	{
		const Castle *pCastle = pGroup->GetTile()->GetCastle();
		if(pCastle && pCastle->GetPlayer() && GetPlayer())
			damage *= 0.80f;
	}

	return damage;
}

void Unit::UpdateStats()
{
	int newLifeMax = ModStatInt(details.life + (victories & ~1), Item::MT_Stat, Item::Mod_Life);
	int newMoveMax = ModStatInt(details.movement, Item::MT_Stat, Item::Mod_Movement);
	maxAtk = ModStatFloat((float)(details.attackMax + victories / 2), Item::MT_Stat, Item::Mod_MaxAtk);
	minAtk = MFMin(ModStatFloat((float)(details.attackMin + victories / 2), Item::MT_Stat, Item::Mod_MinAtk), maxAtk);

	// galleon gives combat advantages
	if(pGroup && pGroup->GetVehicle())
	{
		Unit *pVehicle = pGroup->GetVehicle();
		if(pVehicle != this && pVehicle->GetName() == "Galleon")
		{
			// units in a galleon kick more arse
			minAtk *= 1.3f;
			maxAtk *= 1.3f;
		}
	}

	// find difference from current
	int lifeDiff = newLifeMax - lifeMax;
	int moveDiff = newMoveMax - movementMax;

	// adjust current stats
	life = life ? MFMax(life + lifeDiff, 1) : 0;
	movement += moveDiff * 2;

	// update current
	lifeMax = newLifeMax;
	movementMax = newMoveMax;
}

int Unit::ModStatInt(int stat, int statType, int modIndex) const
{
	float scale = 1.f;
	float diff = 0.f;

	if(pGroup)
	{
		// find any heroes in the group
		int numUnits = pGroup->GetNumUnits();
		for(int u=0; u<numUnits; ++u)
		{
			Unit *pHero = pGroup->GetUnit(u);
			if(pHero->IsHero())
			{
				// all items heroes possess may affect each unit in the group...
				int numItems = pHero->GetNumItems();
				for(int a=0; a<numItems; ++a)
				{
					const Item::StatMod *pMod = pHero->GetItem(a).GetMod(this, pHero, statType, modIndex);
					if(pMod)
					{
						if(pMod->flags & Item::StatMod::SMF_Absolute)
						{
							int val = (int)pMod->value;
							if(statType == Item::MT_Terrain)
							{
								if(stat > 0 && val > 0)
									stat = MFMin(stat, val);
								else
									stat = MFMax(stat, val);
							}
							else
								stat = val;
						}
						else if(pMod->flags & Item::StatMod::SMF_Percent)
							scale *= pMod->value;
						else
							diff += pMod->value;
					}
				}
			}
		}
	}

	return MFMax((int)(((float)stat + diff) * scale), 0);
}

float Unit::ModStatFloat(float stat, int statType, int modIndex) const
{
	float scale = 1.f;
	float diff = 0.f;

	if(pGroup)
	{
		// find any heroes in the group
		int numUnits = pGroup->GetNumUnits();
		for(int u=0; u<numUnits; ++u)
		{
			Unit *pHero = pGroup->GetUnit(u);
			if(pHero->IsHero())
			{
				// all items heroes possess may affect each unit in the group...
				int numItems = pHero->GetNumItems();
				for(int a=0; a<numItems; ++a)
				{
					const Item::StatMod *pMod = pHero->GetItem(a).GetMod(this, pHero, statType, modIndex);
					if(pMod)
					{
						if(pMod->flags & Item::StatMod::SMF_Absolute)
							stat = pMod->value;
						else if(pMod->flags & Item::StatMod::SMF_Percent)
							scale *= pMod->value;
						else
							diff += pMod->value;
					}
				}
			}
		}
	}

	return MFMax((stat + diff) * scale, 0.f);
}
