#include "Warlords.h"
#include "UnitDefinitions.h"

#include "Fuji/MFIni.h"

#include "Fuji/MFMaterial.h"
#include "Fuji/Materials/MFMat_Standard.h"
#include "Fuji/MFTexture.h"
#include "Fuji/MFPrimitive.h"

void Item::StatMod::Parse(const char *pString)
{
	value = 0.f;
	flags = 0;

	if(!pString)
		return;

	if(pString[0] != '+' && pString[0] != '-')
		flags |= SMF_Absolute;

	value = MFString_AsciiToFloat(pString);
	if(value == 0.f)
		flags = 0;

	if(pString[MFString_Length(pString) - 1] == '%')
	{
		flags |= SMF_Percent;
		value = 1 + value*0.01f;
	}
}

const Item::StatMod *Item::GetMod(const Unit *pUnit, const Unit *pHero, int type, int index) const
{
	for(size_t a=0; a<mods.size(); ++a)
	{
		const Item::GroupMod &mod = mods[a];

		for(size_t b=0; b<mod.targets.size(); ++b)
		{
			const MFString &target = mod.targets[b];

			if((pUnit == pHero && target == "self") ||
				(pUnit != pHero && target == "group") ||
				pUnit->IsType(target))
			{
				switch(type)
				{
					case MT_Stat:
						return &mod.mods[index];
					case MT_Defence:
						return &mod.defence[index];
					case MT_Terrain:
						return &mod.terrain[index];
					default:
						break;
				}
			}
		}
	}

	return NULL;
}

float Item::GetSpecial(const Unit *pUnit, const Unit *pHero, MFString name) const
{
	for(size_t a=0; a<mods.size(); ++a)
	{
		const Item::GroupMod &mod = mods[a];
		for(size_t b=0; b<mod.targets.size(); ++b)
		{
			const MFString &target = mod.targets[b];

			if((pUnit == pHero && target == "self") ||
				(pUnit != pHero && target == "group") ||
				pUnit->IsType(target))
			{
				if(name == mods[a].special)
					return mods[a].probability;
			}
		}
	}

	return 0.f;
}

const char *UnitDetails::AttackSpeedDescription() const
{
	float attackTime = cooldown + attackSpeed;
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

/*
bool UnitDefinitions::GetDetails(MFString unitsetName, UnitSetDetails *pDetails)
{
	if(!pDetails)
		return false;

	MFZeroMemory(pDetails, sizeof(UnitSetDetails));

	MFIni *pIni = MFIni::Create(unitsetName);
	if(!pIni)
		return NULL;

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("UnitDefinitions"))
		{
			pLine = pLine->Sub();
			break;
		}
		pLine = pLine->Next();
	}

	while(pLine)
	{
		if(pLine->IsString(0, "name"))
		{
			MFString_Copy(pDetails->unitSetName, pLine->GetString(1));
		}
		else if(pLine->IsSection("Races"))
		{
			MFIniLine *pRace = pLine->Sub();
			while(pRace)
			{
				int r = pRace->GetInt(0);
				pDetails->numRaces = MFMax(pDetails->numRaces, r + 1);

				MFString_Copy(pDetails->races[r], pRace->GetString(1));
				pDetails->colours[r] = MFString_AsciiToInteger(pRace->GetString(2), true) | 0xFF000000;

				pRace = pRace->Next();
			}
		}
		else if(pLine->IsSection("Units"))
		{
			MFIniLine *pUnit = pLine->Sub();
			while(pUnit)
			{
				if(pUnit->IsSection("Unit"))
				{
					pDetails->units[pDetails->numUnits].type = UT_Unit;

					MFIniLine *pUnitDesc = pUnit->Sub();
					while(pUnitDesc)
					{
						if(pUnitDesc->IsString(0, "name"))
						{
							MFString_CopyN(pDetails->units[pDetails->numUnits].name, pUnitDesc->GetString(1), 63);
						}
						else if(pUnitDesc->IsString(0, "type"))
						{
							if(pUnitDesc->IsString(1, "hero"))
								pDetails->units[pDetails->numUnits].type = UT_Hero;
							else if(pUnitDesc->IsString(1, "unit"))
								pDetails->units[pDetails->numUnits].type = UT_Unit;
							else if(pUnitDesc->IsString(1, "vehicle"))
								pDetails->units[pDetails->numUnits].type = UT_Vehicle;
						}
						else if(pUnitDesc->IsString(0, "race"))
						{
							pDetails->units[pDetails->numUnits].race = pUnitDesc->GetInt(1);
						}

						pUnitDesc = pUnitDesc->Next();
					}

					++pDetails->numUnits;
				}

				pUnit = pUnit->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	return true;
}
*/
UnitDefinitions::UnitDefinitions(MFString unitsetName, int numTerrainTypes)
{
	MFIni *pIni = MFIni::Create(unitsetName.CStr());
	MFDebug_Assert(pIni, "Can't load unit defs!");

	pUnitMat = NULL;
	pHeadMat = NULL;
	pCastleMat = NULL;

	unitMapWidth = unitMapHeight = 0;
	castleMapWidth = castleMapHeight = 0;

	numTerrainTypes = numTerrainTypes;

	pRanks = MFMaterial_Create("UnitRanks");

	MFIniLine *pLine = pIni->GetFirstLine();
	while(pLine)
	{
		if(pLine->IsSection("UnitDefinitions"))
		{
			pLine = pLine->Sub();
			break;
		}
		pLine = pLine->Next();
	}

	while(pLine)
	{
		if(pLine->IsString(0, "name"))
		{
			name = pLine->GetString(1);
		}
		else if(pLine->IsString(0, "detail_map"))
		{
			pUnitMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pUnitMat)
			{
				MFTexture *pTex = MFMaterial_GetParameterT(pUnitMat, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &unitMapWidth, &unitMapHeight);
			}
		}
		else if(pLine->IsString(0, "heads_map"))
		{
			pHeadMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));
		}
		else if(pLine->IsString(0, "castle_map"))
		{
			pCastleMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pCastleMat)
			{
				MFTexture *pTex = MFMaterial_GetParameterT(pCastleMat, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &castleMapWidth, &castleMapHeight);
			}
		}
		else if(pLine->IsString(0, "item_map"))
		{
			pItemMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pItemMat)
			{
				MFTexture *pTex = MFMaterial_GetParameterT(pItemMat, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &itemMapWidth, &itemMapHeight);
			}
		}
		else if(pLine->IsString(0, "misc_map"))
		{
			pMiscMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pMiscMat)
			{
				MFTexture *pTex = MFMaterial_GetParameterT(pMiscMat, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &miscMapWidth, &miscMapHeight);
			}
		}
		else if(pLine->IsString(0, "tile_width"))
		{
			tileWidth = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "tile_height"))
		{
			tileHeight = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "item_width"))
		{
			itemWidth = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "item_height"))
		{
			itemHeight = pLine->GetInt(1);
		}
		else if(pLine->IsSection("Races"))
		{
			MFIniLine *pRace = pLine->Sub();
			while(pRace)
			{
				size_t r = pRace->GetInt(0);
				races.resize(MFMax(r + 1, races.size()));

				races[r].name = pRace->GetString(1);
				races[r].colour = MFString_AsciiToInteger(pRace->GetString(2), true) | 0xFF000000;

				pRace = pRace->Next();
			}
		}
		else if(pLine->IsSection("Castles"))
		{
			MFIniLine *pCastle = pLine->Sub();
			while(pCastle)
			{
				int r = pCastle->GetInt(0);

				races[r].castlex = pCastle->GetInt(1);
				races[r].castley = pCastle->GetInt(2);
				races[r].flagx = pCastle->GetInt(3);
				races[r].flagy = pCastle->GetInt(4);

				pCastle = pCastle->Next();
			}
		}
		else if(pLine->IsSection("Special"))
		{
			MFIniLine *pSpecial = pLine->Sub();
			while(pSpecial)
			{
				size_t s = pSpecial->GetInt(0);
				specials.resize(MFMax(s + 1, specials.size()));

				specials[s].index = s;
				specials[s].name = pSpecial->GetString(1);
				specials[s].x = (uint8)pSpecial->GetInt(2);
				specials[s].y = (uint8)pSpecial->GetInt(3);
				specials[s].width = (uint8)pSpecial->GetInt(4);
				specials[s].height = (uint8)pSpecial->GetInt(5);

				for(int a=6; a<pSpecial->GetStringCount(); ++a)
				{
					if(pSpecial->IsString(a, "searchable"))
						specials[s].type = Special::ST_Searchable;
					else if(pSpecial->IsString(a, "recruit"))
						specials[s].type = Special::ST_Recruit;
				}

				pSpecial = pSpecial->Next();
			}
		}
		else if(pLine->IsSection("Units"))
		{
			MFIniLine *pUnits = pLine->Sub();
			while(pUnits)
			{
				if(pUnits->IsSection("Unit"))
				{
					UnitDetails &unit = units.push();

					unit.type = UT_Unit;
					unit.buildTime = 1;

					MFIniLine *pUnitDesc = pUnits->Sub();
					while(pUnitDesc)
					{
						if(pUnitDesc->IsString(0, "name"))
						{
							unit.name = pUnitDesc->GetString(1);
						}
						else if(pUnitDesc->IsString(0, "pos"))
						{
							unit.x = pUnitDesc->GetInt(1);
							unit.y = pUnitDesc->GetInt(2);
							unit.width = pUnitDesc->GetInt(3);
							unit.height = pUnitDesc->GetInt(4);
						}
						else if(pUnitDesc->IsString(0, "type"))
						{
							if(pUnitDesc->IsString(1, "hero"))
								unit.type = UT_Hero;
							else if(pUnitDesc->IsString(1, "unit"))
								unit.type = UT_Unit;
							else if(pUnitDesc->IsString(1, "vehicle"))
								unit.type = UT_Vehicle;
						}
						else if(pUnitDesc->IsString(0, "race"))
						{
							unit.race = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "atk"))
						{
							unit.attackMin = pUnitDesc->GetInt(1);
							unit.attackMax = pUnitDesc->GetInt(2);
						}
						else if(pUnitDesc->IsString(0, "move"))
						{
							unit.movement = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "class"))
						{
							unit.atkType = pUnitDesc->GetInt(1);
							unit.attack = pUnitDesc->GetInt(2);
							unit.armour = pUnitDesc->GetInt(3);
							unit.movementClass = pUnitDesc->GetInt(4);
							unit.weapon = pUnitDesc->GetInt(5);

							if(pUnitDesc->GetStringCount() >= 7)
							{
								const char *pClass = pUnitDesc->GetString(6);
								if(!MFString_CaseCmp(pClass, "agile"))
								{
									unit.attack = 4;
									unit.armour = 5;
								}
								else if(!MFString_CaseCmp(pClass, "tough"))
								{
									unit.attack = 5;
									unit.armour = 3;
								}
								else if(!MFString_CaseCmp(pClass, "magic"))
								{
									unit.attack = 3;
									unit.armour = 6;
								}
								else if(!MFString_CaseCmp(pClass, "hero"))
								{
									unit.attack = 6;
									unit.armour = 4;
								}
							}
						}
						else if(pUnitDesc->IsString(0, "cooldown"))
						{
							unit.cooldown = pUnitDesc->GetFloat(1);
							unit.attackSpeed = pUnitDesc->GetFloat(2);
						}
						else if(pUnitDesc->IsString(0, "life"))
						{
							unit.life = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "build"))
						{
							unit.buildTime = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "description"))
						{
							unit.description = pUnitDesc->GetString(1);
						}
						else if(pUnitDesc->IsString(0, "items"))
						{
							int numItems = pUnitDesc->GetStringCount() - 1;
							MFDebug_Assert(numItems <= UnitDetails::MaxItems, "Exceeded maximum number of starting items!");

							unit.numItems = 0;
							for(int a=0; a<numItems; ++a)
							{
								int item = FindItem(pUnitDesc->GetString(1 + a));
								if(item >= 0)
									unit.items[unit.numItems++] = item;
							}
						}

						pUnitDesc = pUnitDesc->Next();
					}
				}

				pUnits = pUnits->Next();
			}
		}
		else if(pLine->IsSection("Weapons"))
		{
			MFIniLine *pWeapon = pLine->Sub();
			while(pWeapon)
			{
				size_t w = pWeapon->GetInt(0);
				weapons.resize(MFMax(w + 1, weapons.size()));
				Weapon &weapon = weapons[w];

				weapon.name = pWeapon->GetString(5);
				weapon.x = pWeapon->GetInt(1);
				weapon.y = pWeapon->GetInt(2);
				weapon.width = pWeapon->GetInt(3);
				weapon.height = pWeapon->GetInt(4);
				weapon.bIsProjectile = (weapon.width != 0 && weapon.height != 0);
				weapon.impactAnim = pWeapon->GetInt(6);

				pWeapon = pWeapon->Next();
			}
		}
		else if(pLine->IsSection("Classes"))
		{
			MFIniLine *pClasses = pLine->Sub();
			MFIniLine *pWeaponClasses = NULL;
			MFIniLine *pArmourClasses = NULL;
			while(pClasses)
			{
				if(pClasses->IsSection("Movement"))
				{
					MFIniLine *pMovement = pClasses->Sub();
					while(pMovement)
					{
						size_t movClass = pMovement->GetInt(0);
						movementClasses.resize(MFMax(movClass + 1, movementClasses.size()));

						movementClasses[movClass].name = pMovement->GetString(1);
						for(int a=0; a<numTerrainTypes; ++a)
							movementClasses[movClass].movementPenalty.push(pMovement->GetInt(2 + a));

						int flagsStart = 2 + numTerrainTypes;
						const char *pFlags = NULL;
						while((pFlags = pMovement->GetString(flagsStart++)))
						{
							if(!MFString_CaseCmp(pFlags, "roadwalk"))
								movementClasses[movClass].roadWalk = 1;							
						}

						pMovement = pMovement->Next();
					}
				}
				else if(pClasses->IsSection("Type"))
				{
					MFIniLine *pType = pClasses->Sub();
					while(pType)
					{
						size_t t = pType->GetInt(0);
						types.resize(MFMax(t + 1, types.size()));

						types[t] = pType->GetString(1);

						pType = pType->Next();
					}
				}
				else if(pClasses->IsSection("Attack"))
				{
					MFIniLine *pAttack = pClasses->Sub();
					while(pAttack)
					{
						size_t a = pAttack->GetInt(0);
						attackClasses.resize(MFMax(a + 1, attackClasses.size()));

						attackClasses[a] = pAttack->GetString(1);

						pAttack = pAttack->Next();
					}
				}
				else if(pClasses->IsSection("Defence"))
				{
					pArmourClasses = pClasses->Sub();
				}

				pClasses = pClasses->Next();
			}

			if(pArmourClasses)
			{
				MFIniLine *pArmour = pArmourClasses;
				while(pArmour)
				{
					size_t armourClass = pArmour->GetInt(0);
					defenceClasses.resize(MFMax(armourClass + 1, defenceClasses.size()));
					ArmourClass &amrClass = defenceClasses[armourClass];

					amrClass.name = pArmour->GetString(1);

					for(size_t a=0; a<attackClasses.size(); ++a)
						amrClass.damageMods.push(pArmour->GetFloat(2 + a));

					pArmour = pArmour->Next();
				}
			}
		}
		else if(pLine->IsSection("Items"))
		{
			MFIniLine *pItem = pLine->Sub();
			while(pItem)
			{
				if(pItem->IsSection("Item"))
				{
					Item &item = items.push();
					item.bCollectible = true;

					MFIniLine *pItemDesc = pItem->Sub();
					while(pItemDesc)
					{
						if(pItemDesc->IsString(0, "pos"))
						{
							item.x = pItemDesc->GetInt(1);
							item.y = pItemDesc->GetInt(2);
						}
						else if(pItemDesc->IsString(0, "name"))
						{
							item.name = pItemDesc->GetString(1);
						}
						else if(pItemDesc->IsString(0, "description"))
						{
							item.description = pItemDesc->GetString(1);
						}
						else if(pItemDesc->IsString(0, "collectible"))
						{
							item.bCollectible = pItemDesc->GetBool(1);
						}
						else if(pItemDesc->IsSection("ModGroup"))
						{
							Item::GroupMod &group = item.mods.push();

							MFIniLine *pGroup = pItemDesc->Sub();
							while(pGroup)
							{
								if(pGroup->IsString(0, "target"))
								{
									int numUnits = pGroup->GetStringCount() - 1;
									for(int a=0; a<numUnits; ++a)
										group.targets.push(pGroup->GetString(1 + a));
								}
								else if(pGroup->IsString(0, "attack"))
								{
									group.mods[Item::Mod_MinAtk].Parse(pGroup->GetString(1));
									group.mods[Item::Mod_MaxAtk].Parse(pGroup->GetString(2));
								}
								else if(pGroup->IsString(0, "cooldown"))
								{
									group.mods[Item::Mod_Cool].Parse(pGroup->GetString(1));
								}
								else if(pGroup->IsString(0, "life"))
								{
									group.mods[Item::Mod_Life].Parse(pGroup->GetString(1));
								}
								else if(pGroup->IsString(0, "regen"))
								{
									group.mods[Item::Mod_Regen].Parse(pGroup->GetString(1));
								}
								else if(pGroup->IsString(0, "move"))
								{
									group.mods[Item::Mod_Movement].Parse(pGroup->GetString(1));
								}
								else if(pGroup->IsString(0, "defence"))
								{
									for(size_t a=0; a<attackClasses.size(); ++a)
										group.defence[a].Parse(pGroup->GetString(1 + a));
								}
								else if(pGroup->IsString(0, "terrain"))
								{
									for(int a=0; a<numTerrainTypes; ++a)
										group.terrain[a].Parse(pGroup->GetString(1 + a));
								}
								else if(pGroup->IsString(0, "special"))
								{
									group.special = pGroup->GetString(1);
									group.probability = pGroup->GetFloat(2);
								}

								pGroup = pGroup->Next();
							}
						}

						pItemDesc = pItemDesc->Next();
					}
				}

				pItem = pItem->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);
}

UnitDefinitions::~UnitDefinitions()
{
	if(pUnitMat)
		MFMaterial_Release(pUnitMat);
	if(pHeadMat)
		MFMaterial_Release(pHeadMat);
	if(pCastleMat)
		MFMaterial_Release(pCastleMat);
	if(pRanks)
		MFMaterial_Release(pRanks);
}

MFVector UnitDefinitions::GetRaceColour(int race) const
{
	uint32 c = races[race].colour;
	return MakeVector(((c >> 16) & 0xFF) * (1.f/255.f), ((c >> 8) & 0xFF) * (1.f/255.f), (c & 0xFF) * (1.f/255.f), 1.f);
}

int UnitDefinitions::FindRace(MFString name) const
{
	for(size_t a=0; a<races.size(); ++a)
	{
		if(name == races[a].name)
			return a;
	}
	return -1;
}

int UnitDefinitions::GetNumHeroesForRace(int race) const
{
	int numHeroes = 0;
	int numUnits = GetNumUnitTypes();
	for(int u=0; u<numUnits; ++u)
	{
		const UnitDetails &unit = GetUnitDetails(u);
		if(unit.type == UT_Hero && (unit.race == race || unit.race == 0))
			++numHeroes;
	}
	return numHeroes;
}

int UnitDefinitions::GetHeroForRace(int race, int heroIndex) const
{
	int numUnits = GetNumUnitTypes();
	for(int u=0; u<numUnits; ++u)
	{
		const UnitDetails &unit = GetUnitDetails(u);
		if(unit.type == UT_Hero && (unit.race == race || unit.race == 0))
		{
			if(heroIndex-- == 0)
				return u;
		}
	}
	return -1;
}

int UnitDefinitions::FindUnit(MFString name) const
{
	for(size_t a=0; a<units.size(); ++a)
	{
		if(name == units[a].name)
			return a;
	}
	return -1;
}

int UnitDefinitions::FindItem(MFString name) const
{
	for(size_t a=0; a<items.size(); ++a)
	{
		if(name == items[a].name)
			return a;
	}
	return -1;
}

Unit *UnitDefinitions::CreateUnit(int unit, int player, GameState *pGame) const
{
	Unit *pUnit = new Unit(*pGame);

	pUnit->type = unit;
	pUnit->player = player;

	pUnit->details = units[unit];
	pUnit->name = pUnit->details.name;
	pUnit->pGroup = NULL;

	if(units[unit].type == UT_Hero)
	{
		MFDebug_Assert(pUnit->details.numItems <= Unit::MaxItems, "Too many items!");

		pUnit->items.reserve(Unit::MaxItems); // might as well reserve space for the maximum

		for(int a=0; a<pUnit->details.numItems; ++a)
			pUnit->items.push(pUnit->details.items[a]);
	}

	pUnit->kills = pUnit->victories = 0;
	pUnit->life = pUnit->lifeMax = pUnit->details.life;
	pUnit->movement = pUnit->movementMax = pUnit->details.movement;

	pUnit->UpdateStats();

	pUnit->life = pUnit->GetMaxHP();
	pUnit->movement = pUnit->GetMaxMovement() * 2;

	pUnit->plan.type = TT_Ranged;
	pUnit->plan.strength = TS_Weakest;
	pUnit->plan.bAttackAvailable = true;

	return pUnit;
}

MFRect UnitDefinitions::GetCastleUVs(int race, float texelOffset) const
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Race &r = races[race];
	MFRect uvs;
	uvs.x = r.castlex*xScale + halfX;
	uvs.y = r.castley*yScale + halfY;
	uvs.width = xScale*2;
	uvs.height = yScale*2;
	return uvs;
}

MFRect UnitDefinitions::GetFlagUVs(int race, float texelOffset) const
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Race &r = races[race];
	MFRect uvs;
	uvs.x = r.flagx*xScale + halfX;
	uvs.y = r.flagy*yScale + halfY;
	uvs.width = xScale;
	uvs.height = yScale;
	return uvs;
}

MFRect UnitDefinitions::GetUnitUVs(int unit, bool bFlip, float texelOffset) const
{
	float fWidth = (float)unitMapWidth;
	float fHeight = (float)unitMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const UnitDetails &def = units[unit];
	MFRect uvs;
	uvs.x = bFlip ? (def.x+def.width)*xScale - halfX : def.x*xScale + halfX;
	uvs.y = def.y*yScale + halfY;
	uvs.width = (bFlip ? -def.width : def.width)*xScale;
	uvs.height = def.height*yScale;
	return uvs;
}

MFRect UnitDefinitions::GetSpecialUVs(int index, float texelOffset, int *pWidth, int *pHeight) const
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Special &s = specials[index];
	MFRect uvs;
	uvs.x = s.x*xScale + halfX;
	uvs.y = s.y*yScale + halfY;
	uvs.width = xScale*(float)s.width;
	uvs.height = yScale*(float)s.height;

	if(pWidth)
		*pWidth = s.width;
	if(pHeight)
		*pHeight = s.height;

	return uvs;
}

MFRect UnitDefinitions::GetItemUVs(int item, float texelOffset) const
{
	float fWidth = (float)itemMapWidth;
	float fHeight = (float)itemMapHeight;
	float xScale = (1.f / fWidth) * itemWidth;
	float yScale = (1.f / fHeight) * itemHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Item &i = items[item];
	MFRect uvs;
	uvs.x = i.x*xScale + halfX;
	uvs.y = i.y*yScale + halfY;
	uvs.width = xScale;
	uvs.height = yScale;
	return uvs;
}

MFRect UnitDefinitions::GetBadgeUVs(int rank, float texelOffset) const
{
	if(rank == 0)
	{
		MFRect uvs = { 0, 0, 0, 0 };
		return uvs;
	}
	--rank;

	float fWidth = 64.f;
	float fHeight = 32.f;
	float xScale = 1.f / 4.f;
	float yScale = 1.f / 2.f;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	MFRect uvs;
	uvs.x = (float)(rank & 3)*xScale + halfX;
	uvs.y = (float)(rank >> 2)*yScale + halfY;
	uvs.width = xScale;
	uvs.height = yScale;
	return uvs;
}
