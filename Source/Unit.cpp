#include "Warlords.h"
#include "Unit.h"

#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFPrimitive.h"
#include "MFIni.h"

extern Game *pGame;

bool UnitDefinitions::GetDetails(const char *pUnitSetName, UnitSetDetails *pDetails)
{
	if(!pDetails)
		return false;

	MFZeroMemory(pDetails, sizeof(UnitSetDetails));

	MFIni *pIni = MFIni::Create(pUnitSetName);
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

UnitDefinitions *UnitDefinitions::Load(Game *pGame, const char *pUnitSetName, int numTerrainTypes)
{
	MFIni *pIni = MFIni::Create(pUnitSetName);
	if(!pIni)
		return NULL;

	UnitDefinitions *pUnitDefs = new UnitDefinitions;

	pUnitDefs->pGame = pGame;
	pUnitDefs->pUnitDefs = pIni;

	pUnitDefs->pUnits = NULL;
	pUnitDefs->numUnits = 0;

	pUnitDefs->pUnitMat = NULL;
	pUnitDefs->pHeadMat = NULL;
	pUnitDefs->pCastleMat = NULL;

	pUnitDefs->unitMapWidth = pUnitDefs->unitMapHeight = 0;
	pUnitDefs->castleMapWidth = pUnitDefs->castleMapHeight = 0;

	pUnitDefs->numMovementClasses = 0;
	pUnitDefs->numTypes = 0;
	pUnitDefs->numAttackClasses = pUnitDefs->numDefenceClasses = 0;
	pUnitDefs->pMovementClasses = NULL;
	pUnitDefs->ppTypes = NULL;
	pUnitDefs->ppAttackClasses = NULL;
	pUnitDefs->pDefenceClasses = NULL;

	pUnitDefs->numTerrainTypes = numTerrainTypes;

	pUnitDefs->pRanks = MFMaterial_Create("UnitRanks");
	pUnitDefs->numRenderUnits = 0;

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
			pUnitDefs->pName = pLine->GetString(1);
		}
		else if(pLine->IsString(0, "detail_map"))
		{
			pUnitDefs->pUnitMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pUnitDefs->pUnitMat)
			{
				int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pUnitMat, "diffuseMap");
				MFTexture *pTex = MFMaterial_GetParameterT(pUnitDefs->pUnitMat, diffuse, 0);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &pUnitDefs->unitMapWidth, &pUnitDefs->unitMapHeight);
			}
		}
		else if(pLine->IsString(0, "heads_map"))
		{
			pUnitDefs->pHeadMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));
		}
		else if(pLine->IsString(0, "castle_map"))
		{
			pUnitDefs->pCastleMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pUnitDefs->pCastleMat)
			{
				int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pCastleMat, "diffuseMap");
				MFTexture *pTex = MFMaterial_GetParameterT(pUnitDefs->pCastleMat, diffuse, 0);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &pUnitDefs->castleMapWidth, &pUnitDefs->castleMapHeight);
			}
		}
		else if(pLine->IsString(0, "item_map"))
		{
			pUnitDefs->pItemMat = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

			if(pUnitDefs->pItemMat)
			{
				int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pCastleMat, "diffuseMap");
				MFTexture *pTex = MFMaterial_GetParameterT(pUnitDefs->pItemMat, diffuse, 0);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &pUnitDefs->itemMapWidth, &pUnitDefs->itemMapHeight);
			}
		}
		else if(pLine->IsString(0, "tile_width"))
		{
			pUnitDefs->tileWidth = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "tile_height"))
		{
			pUnitDefs->tileHeight = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "item_width"))
		{
			pUnitDefs->itemWidth = pLine->GetInt(1);
		}
		else if(pLine->IsString(0, "item_height"))
		{
			pUnitDefs->itemHeight = pLine->GetInt(1);
		}
		else if(pLine->IsSection("Races"))
		{
			pUnitDefs->raceCount = 0;

			MFIniLine *pRace = pLine->Sub();
			while(pRace)
			{
				pUnitDefs->raceCount = MFMax(pUnitDefs->raceCount, pRace->GetInt(0) + 1);
				pRace = pRace->Next();
			}

			pUnitDefs->pRaces = (Race*)MFHeap_AllocAndZero(sizeof(Race) * pUnitDefs->raceCount);

			pRace = pLine->Sub();
			while(pRace)
			{
				int r = pRace->GetInt(0);
				pUnitDefs->pRaces[r].pName = pRace->GetString(1);
				pUnitDefs->pRaces[r].colour = MFString_AsciiToInteger(pRace->GetString(2), true) | 0xFF000000;

				pRace = pRace->Next();
			}
		}
		else if(pLine->IsSection("Castles"))
		{
			MFIniLine *pCastle = pLine->Sub();
			while(pCastle)
			{
				int r = pCastle->GetInt(0);

				pUnitDefs->pRaces[r].castlex = pCastle->GetInt(1);
				pUnitDefs->pRaces[r].castley = pCastle->GetInt(2);
				pUnitDefs->pRaces[r].flagx = pCastle->GetInt(3);
				pUnitDefs->pRaces[r].flagy = pCastle->GetInt(4);

				pCastle = pCastle->Next();
			}
		}
		else if(pLine->IsSection("Special"))
		{
			pUnitDefs->specialCount = 0;

			MFIniLine *pSpecial = pLine->Sub();
			while(pSpecial)
			{
				pUnitDefs->specialCount = MFMax(pUnitDefs->specialCount, pSpecial->GetInt(0) + 1);
				pSpecial = pSpecial->Next();
			}

			pUnitDefs->pSpecials = (Special*)MFHeap_AllocAndZero(sizeof(Special) * pUnitDefs->specialCount);

			pSpecial = pLine->Sub();
			while(pSpecial)
			{
				int s = pSpecial->GetInt(0);
				pUnitDefs->pSpecials[s].pName = pSpecial->GetString(1);
				pUnitDefs->pSpecials[s].x = (uint8)pSpecial->GetInt(2);
				pUnitDefs->pSpecials[s].y = (uint8)pSpecial->GetInt(3);
				pUnitDefs->pSpecials[s].width = (uint8)pSpecial->GetInt(4);
				pUnitDefs->pSpecials[s].height = (uint8)pSpecial->GetInt(5);

				for(int a=6; a<pSpecial->GetStringCount(); ++a)
				{
					if(pSpecial->IsString(a, "searchable"))
						pUnitDefs->pSpecials[s].canSearch = 1;
				}

				pSpecial = pSpecial->Next();
			}
		}
		else if(pLine->IsSection("Units"))
		{
			MFIniLine *pUnits = pLine->Sub();

			pUnitDefs->numUnits = 0;
			while(pUnits)
			{
				if(pUnits->IsSection("Unit"))
					++pUnitDefs->numUnits;
				pUnits = pUnits->Next();
			}

			pUnitDefs->pUnits = (UnitDetails*)MFHeap_AllocAndZero(pUnitDefs->numUnits * sizeof(UnitDetails));
			UnitDetails *pUnit = pUnitDefs->pUnits;

			pUnits = pLine->Sub();
			while(pUnits)
			{
				if(pUnits->IsSection("Unit"))
				{
					pUnit->type = UT_Unit;
					pUnit->buildTime = 1;

					MFIniLine *pUnitDesc = pUnits->Sub();
					while(pUnitDesc)
					{
						if(pUnitDesc->IsString(0, "name"))
						{
							pUnit->pName = pUnitDesc->GetString(1);
						}
						else if(pUnitDesc->IsString(0, "pos"))
						{
							pUnit->x = pUnitDesc->GetInt(1);
							pUnit->y = pUnitDesc->GetInt(2);
							pUnit->width = pUnitDesc->GetInt(3);
							pUnit->height = pUnitDesc->GetInt(4);
						}
						else if(pUnitDesc->IsString(0, "type"))
						{
							if(pUnitDesc->IsString(1, "hero"))
								pUnit->type = UT_Hero;
							else if(pUnitDesc->IsString(1, "unit"))
								pUnit->type = UT_Unit;
							else if(pUnitDesc->IsString(1, "vehicle"))
								pUnit->type = UT_Vehicle;
						}
						else if(pUnitDesc->IsString(0, "race"))
						{
							pUnit->race = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "atk"))
						{
							pUnit->attackMin = pUnitDesc->GetInt(1);
							pUnit->attackMax = pUnitDesc->GetInt(2);
						}
						else if(pUnitDesc->IsString(0, "move"))
						{
							pUnit->movement = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "class"))
						{
							pUnit->atkType = pUnitDesc->GetInt(1);
							pUnit->attack = pUnitDesc->GetInt(2);
							pUnit->armour = pUnitDesc->GetInt(3);
							pUnit->movementClass = pUnitDesc->GetInt(4);
							pUnit->weapon = pUnitDesc->GetInt(5);

							if(pUnitDesc->GetStringCount() >= 7)
							{
								const char *pClass = pUnitDesc->GetString(6);
								if(!MFString_CaseCmp(pClass, "agile"))
								{
									pUnit->attack = 4;
									pUnit->armour = 5;
								}
								else if(!MFString_CaseCmp(pClass, "tough"))
								{
									pUnit->attack = 5;
									pUnit->armour = 3;
								}
								else if(!MFString_CaseCmp(pClass, "magic"))
								{
									pUnit->attack = 3;
									pUnit->armour = 6;
								}
								else if(!MFString_CaseCmp(pClass, "hero"))
								{
									pUnit->attack = 6;
									pUnit->armour = 4;
								}
							}
						}
						else if(pUnitDesc->IsString(0, "cooldown"))
						{
							pUnit->cooldown = pUnitDesc->GetFloat(1);
							pUnit->attackSpeed = pUnitDesc->GetFloat(2);
						}
						else if(pUnitDesc->IsString(0, "life"))
						{
							pUnit->life = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "build"))
						{
							pUnit->buildTime = pUnitDesc->GetInt(1);
						}
						else if(pUnitDesc->IsString(0, "description"))
						{
							pUnit->pDescription = pUnitDesc->GetString(1);
						}
						else if(pUnitDesc->IsString(0, "items"))
						{
							int numItems = pUnitDesc->GetStringCount() - 1;
							MFDebug_Assert(numItems <= UnitDetails::MaxItems, "Exceeded maximum number of starting items!");

							for(int a=0; a<numItems; ++a)
							{
								int item = pUnitDefs->FindItem(pUnitDesc->GetString(1 + a));
								if(item >= 0)
									pUnit->items[pUnit->numItems++] = item;
							}
						}

						pUnitDesc = pUnitDesc->Next();
					}

					++pUnit;
				}

				pUnits = pUnits->Next();
			}
		}
		else if(pLine->IsSection("Weapons"))
		{
			MFIniLine *pWeapon = pLine->Sub();
			pUnitDefs->numWeapons = 0;
			while(pWeapon)
			{
				++pUnitDefs->numWeapons;
				pWeapon = pWeapon->Next();
			}

			pUnitDefs->pWeapons = (Weapon*)MFHeap_Alloc(sizeof(Weapon)*pUnitDefs->numWeapons);

			pWeapon = pLine->Sub();
			while(pWeapon)
			{
				Weapon &weapon = pUnitDefs->pWeapons[pWeapon->GetInt(0)];
				weapon.pName = pWeapon->GetString(5);
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
					pUnitDefs->numMovementClasses = 0;
					while(pMovement)
					{
						++pUnitDefs->numMovementClasses;
						pMovement = pMovement->Next();
					}

					pUnitDefs->pMovementClasses = (MovementClass*)MFHeap_AllocAndZero((sizeof(MovementClass)+sizeof(int)*numTerrainTypes)*pUnitDefs->numMovementClasses);
					int *pMovementPenalties = (int*)(pUnitDefs->pMovementClasses + pUnitDefs->numMovementClasses);

					for(int a=0; a<pUnitDefs->numMovementClasses; ++a)
					{
						pUnitDefs->pMovementClasses[a].pMovementPenalty = pMovementPenalties;
						pMovementPenalties += numTerrainTypes;
					}

					pMovement = pClasses->Sub();
					while(pMovement)
					{
						int movClass = pMovement->GetInt(0);
						pUnitDefs->pMovementClasses[movClass].pName = pMovement->GetString(1);
						for(int a=0; a<numTerrainTypes; ++a)
						{
							int &penalty = pUnitDefs->pMovementClasses[movClass].pMovementPenalty[a];
							penalty = pMovement->GetInt(2 + a);
						}

						int flagsStart = 2 + numTerrainTypes;
						const char *pFlags = NULL;
						while((pFlags = pMovement->GetString(flagsStart++)))
						{
							if(pFlags && !MFString_CaseCmp(pFlags, "roadwalk"))
								pUnitDefs->pMovementClasses[movClass].roadWalk = 1;							
						}

						pMovement = pMovement->Next();
					}
				}
				else if(pClasses->IsSection("Type"))
				{
					MFIniLine *pType = pClasses->Sub();
					pUnitDefs->numTypes = 0;
					while(pType)
					{
						++pUnitDefs->numTypes;
						pType = pType->Next();
					}

					pUnitDefs->ppTypes = (const char **)MFHeap_Alloc(sizeof(const char **)*pUnitDefs->numTypes);

					pType = pClasses->Sub();
					while(pType)
					{
						pUnitDefs->ppTypes[pType->GetInt(0)] = pType->GetString(1);
						pType = pType->Next();
					}
				}
				else if(pClasses->IsSection("Attack"))
				{
					MFIniLine *pAttack = pClasses->Sub();
					pUnitDefs->numAttackClasses = 0;
					while(pAttack)
					{
						++pUnitDefs->numAttackClasses;
						pAttack = pAttack->Next();
					}

					pUnitDefs->ppAttackClasses = (const char **)MFHeap_Alloc(sizeof(const char *)*pUnitDefs->numAttackClasses);

					pAttack = pClasses->Sub();
					while(pAttack)
					{
						pUnitDefs->ppAttackClasses[pAttack->GetInt(0)] = pAttack->GetString(1);
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
				pUnitDefs->numDefenceClasses = 0;
				while(pArmour)
				{
					++pUnitDefs->numDefenceClasses;
					pArmour = pArmour->Next();
				}

				pUnitDefs->pDefenceClasses = (ArmourClass*)MFHeap_Alloc(sizeof(ArmourClass)*pUnitDefs->numDefenceClasses);

				pArmour = pArmourClasses;
				while(pArmour)
				{
					int armourClass = pArmour->GetInt(0);
					ArmourClass &amrClass = pUnitDefs->pDefenceClasses[armourClass];
					amrClass.pDamageMods = (float*)MFHeap_Alloc(sizeof(float)*pUnitDefs->numAttackClasses);

					amrClass.pName = pArmour->GetString(1);

					for(int a=0; a<pUnitDefs->numAttackClasses; ++a)
						amrClass.pDamageMods[a] = pArmour->GetFloat(2 + a);

					pArmour = pArmour->Next();
				}
			}
		}
		else if(pLine->IsSection("Items"))
		{
			MFIniLine *pItem = pLine->Sub();
			pUnitDefs->numItems = 0;
			while(pItem)
			{
				if(pItem->IsSection("Item"))
					++pUnitDefs->numItems;
				pItem = pItem->Next();
			}

			pUnitDefs->pItems = (Item*)MFHeap_AllocAndZero(sizeof(Item)*pUnitDefs->numItems);

			pItem = pLine->Sub();
			int i = 0;
			while(pItem)
			{
				if(pItem->IsSection("Item"))
				{
					Item &item = pUnitDefs->pItems[i++];
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
							item.pName = pItemDesc->GetString(1);
						}
						else if(pItemDesc->IsString(0, "description"))
						{
							item.pDescription = pItemDesc->GetString(1);
						}
						else if(pItemDesc->IsString(0, "collectible"))
						{
							item.bCollectible = pItemDesc->GetBool(1);
						}
						else if(pItemDesc->IsSection("ModGroup"))
							++item.numGroups;

						pItemDesc = pItemDesc->Next();
					}

					item.pMods = (Item::GroupMod*)MFHeap_AllocAndZero(sizeof(Item::GroupMod)*item.numGroups);

					pItemDesc = pItem->Sub();
					int j = 0;
					while(pItemDesc)
					{
						if(pItemDesc->IsSection("ModGroup"))
						{
							Item::GroupMod &group = item.pMods[j++];

							MFIniLine *pGroup = pItemDesc->Sub();
							while(pGroup)
							{
								if(pGroup->IsString(0, "target"))
								{
									group.numUnits = pGroup->GetStringCount() - 1;
									group.ppTargets = (const char**)MFHeap_Alloc(sizeof(const char *)*group.numUnits);
									for(int a=0; a<group.numUnits; ++a)
										group.ppTargets[a] = pGroup->GetString(1 + a);
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
									for(int a=0; a<pUnitDefs->numAttackClasses; ++a)
										group.defence[a].Parse(pGroup->GetString(1 + a));
								}
								else if(pGroup->IsString(0, "terrain"))
								{
									for(int a=0; a<numTerrainTypes; ++a)
										group.terrain[a].Parse(pGroup->GetString(1 + a));
								}
								else if(pGroup->IsString(0, "special"))
								{
									group.pSpecial = pGroup->GetString(1);
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

	return pUnitDefs;
}

void UnitDefinitions::Free()
{
	if(pUnitMat)
		MFMaterial_Destroy(pUnitMat);
	if(pHeadMat)
		MFMaterial_Destroy(pHeadMat);
	if(pCastleMat)
		MFMaterial_Destroy(pCastleMat);
	if(pRanks)
		MFMaterial_Destroy(pRanks);

	if(pRaces)
		MFHeap_Free(pRaces);

	if(pUnits)
		MFHeap_Free(pUnits);

	if(pSpecials)
		MFHeap_Free(pSpecials);

	if(pUnitDefs)
		MFIni::Destroy(pUnitDefs);

	delete this;
}

MFVector UnitDefinitions::GetRaceColour(int race) const
{
	uint32 c = pRaces[race].colour;
	return MakeVector(((c >> 16) & 0xFF) * (1.f/255.f), ((c >> 8) & 0xFF) * (1.f/255.f), (c & 0xFF) * (1.f/255.f), 1.f);
}

int UnitDefinitions::FindRace(const char *pName)
{
	for(int a=0; a<raceCount; ++a)
	{
		if(!MFString_CaseCmp(pRaces[a].pName, pName))
			return a;
	}
	return -1;
}

int UnitDefinitions::FindUnit(const char *pName)
{
	for(int a=0; a<numUnits; ++a)
	{
		if(!MFString_CaseCmp(pUnits[a].pName, pName))
			return a;
	}
	return -1;
}

int UnitDefinitions::FindItem(const char *pName)
{
	for(int a=0; a<numItems; ++a)
	{
		if(!MFString_CaseCmp(pItems[a].pName, pName))
			return a;
	}
	return -1;
}

Unit *UnitDefinitions::CreateUnit(int unit, int player)
{
	Unit *pUnit = pGame->AllocUnit();

	pUnit->type = unit;
	pUnit->player = player;

	pUnit->pUnitDefs = this;
	pUnit->details = pUnits[unit];
	pUnit->pName = pUnit->details.pName;
	pUnit->pGroup = NULL;

	pUnit->pItems = NULL;
	pUnit->numItems = 0;

	if(pUnits[unit].type == UT_Hero)
	{
		MFDebug_Assert(pUnit->details.numItems <= Unit::MaxItems, "Too many items!");

		pUnit->pItems = (int*)MFHeap_Alloc(sizeof(int)*Unit::MaxItems);

		pUnit->numItems = pUnit->details.numItems;
		for(int a=0; a<pUnit->details.numItems; ++a)
			pUnit->pItems[a] = pUnit->details.items[a];
	}

	pUnit->kills = pUnit->victories = 0;
	pUnit->UpdateStats();

	pUnit->life = pUnit->GetMaxHP();
	pUnit->movement = pUnit->GetMaxMovement() * 2;

	pUnit->plan.type = TT_Ranged;
	pUnit->plan.strength = TS_Weakest;
	pUnit->plan.bAttackAvailable = true;

	return pUnit;
}

void UnitDefinitions::DestroyUnit(Unit *pUnit)
{
	pGame->DestroyUnit(pUnit);
}

void UnitDefinitions::AddRenderUnit(int unit, float x, float y, int player, bool bFlip, float alpha, int rank)
{
	renderUnits[numRenderUnits].unit = unit;
	renderUnits[numRenderUnits].x = x;
	renderUnits[numRenderUnits].y = y;
	renderUnits[numRenderUnits].player = player;
	renderUnits[numRenderUnits].alpha = alpha;
	renderUnits[numRenderUnits].bFlip = bFlip;
	renderUnits[numRenderUnits].rank = rank;
	++numRenderUnits;
}

void UnitDefinitions::DrawUnits(float scale, float texelOffset, bool bHead, bool bRank)
{
	if(!numRenderUnits)
		return;

	int numRanked = 0;

	MFMaterial_SetMaterial(bHead ? pHeadMat : pUnitMat);

	MFPrimitive(PT_TriList);
	MFBegin(6*numRenderUnits);

	for(int u=0; u<numRenderUnits; ++u)
	{
		UnitRender &unit = renderUnits[u];
		UnitDetails &def = pUnits[unit.unit];

		if(unit.rank > 0)
			++numRanked;

		MFSetColour(MakeVector(pGame->GetPlayerColour(unit.player), unit.alpha));

		MFRect uvs;
		GetUnitUVs(unit.unit, unit.bFlip, &uvs, texelOffset);

		float depth = 0.f;//bHead ? 0.f : (1000.f - unit.y) / 1000.f;

		float xOffset = -(def.width - 1) / 2.f * scale;
		float yOffset = -(def.height - 1) / 2.f * scale;
		MFSetTexCoord1(uvs.x, uvs.y);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset+def.height*scale, depth);

		MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset, depth);
		MFSetTexCoord1(uvs.x+uvs.width, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset+def.width*scale, unit.y+yOffset+def.height*scale, depth);
		MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
		MFSetPosition(unit.x+xOffset, unit.y+yOffset+def.height*scale, depth);
	}

	MFEnd();

	if(bRank && numRanked != 0)
	{
		MFMaterial_SetMaterial(pRanks);

		MFPrimitive(PT_TriList);
		MFBegin(6*numRanked);

		for(int u=0; u<numRenderUnits; ++u)
		{
			UnitRender &unit = renderUnits[u];
			if(unit.rank == 0)
				continue;

			UnitDetails &def = pUnits[unit.unit];

			MFSetColour(1.f, 1.f, 1.f, unit.alpha);

			MFRect uvs;
			GetBadgeUVs(unit.rank, &uvs, texelOffset);

			float depth = 0.f;//bHead ? 0.f : (1000.f - unit.y) / 1000.f;

			float xOffset = -(def.width - 1) / 2.f * scale + (unit.bFlip ? def.width : 0.f)*scale - 0.25f*scale;
			float yOffset = -(def.height - 1) / 2.f * scale;
			MFSetTexCoord1(uvs.x, uvs.y);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset+0.25f*scale, depth);

			MFSetTexCoord1(uvs.x+uvs.width, uvs.y);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset, depth);
			MFSetTexCoord1(uvs.x+uvs.width, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset+0.25f*scale, unit.y+yOffset+0.25f*scale, depth);
			MFSetTexCoord1(uvs.x, uvs.y+uvs.height);
			MFSetPosition(unit.x+xOffset, unit.y+yOffset+0.25f*scale, depth);
		}

		MFEnd();
	}

	numRenderUnits = 0;
}

int UnitDefinitions::DrawCastle(int race)
{
	return 0;
}

int UnitDefinitions::DrawFlag(int race)
{
	return 0;
}

int UnitDefinitions::DrawSpecial(int index)
{
	return 0;
}

void UnitDefinitions::GetCastleUVs(int race, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	Race &r = pRaces[race];
	pUVs->x = r.castlex*xScale + halfX;
	pUVs->y = r.castley*yScale + halfY;
	pUVs->width = xScale*2;
	pUVs->height = yScale*2;
}

void UnitDefinitions::GetFlagUVs(int race, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	Race &r = pRaces[race];
	pUVs->x = r.flagx*xScale + halfX;
	pUVs->y = r.flagy*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void UnitDefinitions::GetUnitUVs(int unit, bool bFlip, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)unitMapWidth;
	float fHeight = (float)unitMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	UnitDetails &def = pUnits[unit];
	pUVs->x = bFlip ? (def.x+def.width)*xScale - halfX : def.x*xScale + halfX;
	pUVs->y = def.y*yScale + halfY;
	pUVs->width = (bFlip ? -def.width : def.width)*xScale;
	pUVs->height = def.height*yScale;
}

void UnitDefinitions::GetSpecialUVs(int index, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)castleMapWidth;
	float fHeight = (float)castleMapHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	Special &s = pSpecials[index];
	pUVs->x = s.x*xScale + halfX;
	pUVs->y = s.y*yScale + halfY;
	pUVs->width = xScale*(float)s.width;
	pUVs->height = yScale*(float)s.height;
}

void UnitDefinitions::GetItemUVs(int item, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)itemMapWidth;
	float fHeight = (float)itemMapHeight;
	float xScale = (1.f / fWidth) * itemWidth;
	float yScale = (1.f / fHeight) * itemHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	Item &i = pItems[item];
	pUVs->x = i.x*xScale + halfX;
	pUVs->y = i.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void UnitDefinitions::GetBadgeUVs(int rank, MFRect *pUVs, float texelOffset)
{
	if(rank == 0)
		return;
	--rank;

	float fWidth = 64.f;
	float fHeight = 32.f;
	float xScale = 1.f / 4.f;
	float yScale = 1.f / 2.f;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	pUVs->x = (float)(rank & 3)*xScale + halfX;
	pUVs->y = (float)(rank >> 2)*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

const char *UnitDetails::AttackSpeedDescription()
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

const char *Unit::AttackSpeedDescription()
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

void Unit::Destroy()
{
	pUnitDefs->DestroyUnit(this);
}

void Unit::Draw(float x, float y, bool bFlip, float alpha)
{
	pUnitDefs->AddRenderUnit(type, x, y, player, bFlip, alpha, GetRank());
}

void Unit::SetGroup(Group *_pGroup)
{
	pGroup = _pGroup;
}

int Unit::GetRace()
{
	return pGame->GetPlayerRace(player);
}

MFVector Unit::GetColour()
{
	return pGame->GetPlayerColour(player);
}

float Unit::GetSpecialAttack(const char *pSpecial)
{
	float percent = 0.f;
	for(int a=0; a<numItems; ++a)
	{
		Item *pItem = pUnitDefs->GetItem(pItems[a]);

		Unit *pHero = pGroup->GetHero();
		percent += pItem->GetSpecial(this, pHero, pSpecial);
	}
	return percent;
}

int Unit::GetTerrainPenalty(int terrainType)
{
	int penalty = pUnitDefs->GetMovementPenalty(details.movementClass, terrainType);
	return ModStatInt(penalty >> 1, Item::MT_Terrain, terrainType) << 1;
}

void Unit::GetTerrainPenalties(int *pTerrainPenalties)
{
	int numTerrainTypes = Game::GetCurrent()->GetMap()->GetTileset()->GetNumTerrainTypes();
	for(int a=0; a<numTerrainTypes; ++a)
		pTerrainPenalties[a] = GetTerrainPenalty(a);
}

int Unit::GetMovementPenalty(MapTile *pTile, int *pTerrainType)
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
	if(numItems >= 8)
		return false;

	pItems[numItems++] = item;

	if(pGroup)
		pGroup->UpdateGroupStats();
	else
		UpdateStats();

	return true;
}

float Unit::GetCooldown()
{
	return ModStatFloat(details.cooldown, Item::MT_Stat, Item::Mod_Cool);
}

float Unit::GetRegen()
{
	float regenMod = ModStatFloat(1.f, Item::MT_Stat, Item::Mod_Regen);
	return (IsHero() ? 0.4f : 0.25f) * regenMod;
}

float Unit::GetDefence(float damage, int wpnClass)
{
	damage = ModStatFloat(damage, Item::MT_Defence, wpnClass);

	if(player != -1 && pGroup && pGroup->GetTile())
	{
		Castle *pCastle = pGroup->GetTile()->GetCastle();
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
		if(pVehicle != this && !MFString_Compare(pVehicle->GetName(), "Galleon"))
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

int Unit::ModStatInt(int stat, int statType, int modIndex)
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
					Item::StatMod *pMod = pHero->GetItem(a)->GetMod(this, pHero, statType, modIndex);
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

float Unit::ModStatFloat(float stat, int statType, int modIndex)
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
					Item::StatMod *pMod = pHero->GetItem(a)->GetMod(this, pHero, statType, modIndex);
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

void Castle::Init(int _id, const CastleDetails &_details, int _player)
{
	pUnitDefs = NULL;
	pTile = NULL;

	id = _id;
	details = _details;
	player = _player;

	building = -1;
	buildTime = 0;
	nextBuild = -1;
}

MapTile *Castle::GetTile(int index)
{
	if(index >= 2)
	{
		int width;
		Map *pMap = pUnitDefs->GetGame()->GetMap();
		pMap->GetMapSize(&width, NULL);
		return pTile + width + (index & 1);
	}
	else
	{
		return pTile + index;
	}
}

bool Castle::IsEmpty()
{
	if(pTile[0].GetNumUnits() != 0 || pTile[1].GetNumUnits() != 0)
		return false;

	// check the second row... very painfully mind you.
	int width;
	Map *pMap = pUnitDefs->GetGame()->GetMap();
	pMap->GetMapSize(&width, NULL);
	MapTile *pT = pTile + width;
	return pT[0].GetNumUnits() == 0 && pT[1].GetNumUnits() == 0;
}

Group *Castle::GetMercGroup()
{
	static const int odds[16] = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4 };

	Group *pGroup = Group::Create(-1);

	int choices[4];
	int numChoices = 0;
	for(int a=0; a<details.numBuildUnits; ++a)
	{
		int unit = details.buildUnits[a].unit;
		if(pUnitDefs->GetUnitDetails(unit)->type == UT_Unit)
			choices[numChoices++] = unit;

	}

	if(numChoices == 0)
	{
		// OH NO! This castle doesn't seem to have a unit type!!
		choices[numChoices++] = 24; // spearman
		choices[numChoices++] = 25; // maceman
	}

	int range = MFMin(15, 10 + Game::GetCurrent()->GetCurrentTurn());
	int numUnits = odds[MFRand() % range];
	for(int a=0; a<numUnits; ++a)
	{
		pGroup->AddUnit(pGame->GetUnitDefs()->CreateUnit(choices[MFRand()%numChoices], -1));
	}

	return pGroup;
}

void Castle::Capture(Group *pGroup)
{
	player = pGroup->GetPlayer();
	building = -1;
	nextBuild = -1;
	buildTime = 0;

	for(int a=0; a<4; ++a)
	{
		MapTile *pCastleTile = GetTile(a);

		for(int b=0; b<pCastleTile->GetNumGroups(); ++b)
		{
			Group *pUnit = pCastleTile->GetGroup(b);
			pGame->PushCaptureUnits(pGroup, pUnit);
			pUnit->SetPlayer(player);
		}
	}
}

void Castle::SetBuildUnit(int slot)
{
	nextBuild = slot;
}

int Castle::GetBuildUnit()
{
	if(nextBuild == -1)
		return -1;
	return details.buildUnits[nextBuild].unit;
}

int Castle::GetBuildTime()
{
	if(nextBuild == -1)
		return 0;

	if(nextBuild == building)
		return buildTime;

	return details.buildUnits[nextBuild].buildTime;
}

void Ruin::InitRuin(int _id, int specialID, int _item)
{
	pUnitDefs = NULL;
	pSpecial = NULL;
	pTile = NULL;

	id = _id;
	type = specialID;
	item = _item;
	bHasSearched = false;
}

Group *Group::Create(int _player)
{
	Group * pGroup = new Group;
	pGroup->player = _player;
	pGroup->numForwardUnits = pGroup->numRearUnits = 0;
	pGroup->bSelected = false;
	pGroup->pVehicle = NULL;
	pGroup->pPath = NULL;
	pGroup->x = pGroup->y = -1;
	pGroup->pathX = pGroup->pathY = -1;
	pGroup->pLastAction = NULL;
	pGroup->pNext = NULL;
	return pGroup;
}

void Group::Destroy()
{
	delete this;
}

bool Group::AddUnit(Unit *pUnit)
{
	if(pUnit->IsVehicle())
	{
		if(pVehicle)
			return false;

		pVehicle = pUnit;
		pUnit->pGroup = this;
		UpdateGroupStats(); // it could be a galleon
		return true;
	}

	if(numForwardUnits + numRearUnits >= 10)
		return false;

	bool bRear = pUnit->IsRanged();

	if(bRear)
	{
		if(numRearUnits < 5)
			pRearUnits[numRearUnits++] = pUnit;
		else
			pForwardUnits[numForwardUnits++] = pUnit;
	}
	else
	{
		if(numForwardUnits < 5)
			pForwardUnits[numForwardUnits++] = pUnit;
		else
			pRearUnits[numRearUnits++] = pUnit;
	}

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

bool Group::AddForwardUnit(Unit *pUnit)
{
	if(numForwardUnits >= 5)
		return false;

	pForwardUnits[numForwardUnits++] = pUnit;

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

bool Group::AddRearUnit(Unit *pUnit)
{
	if(numRearUnits >= 5)
		return false;

	pRearUnits[numRearUnits++] = pUnit;

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

void Group::RemoveUnit(Unit *pUnit)
{
	if(pUnit->pGroup == this)
	{
		pUnit->pGroup = NULL;
		pUnit->UpdateStats();
	}

	if(pUnit->IsVehicle() && pVehicle == pUnit)
	{
		pVehicle = NULL;
		UpdateGroupStats(); // we may have lost a galleon
		return;
	}

	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pUnit == pForwardUnits[a])
		{
			--numForwardUnits;
			for(int b=a; b<numForwardUnits; ++b)
				pForwardUnits[b] = pForwardUnits[b+1];
			pForwardUnits[numForwardUnits] = NULL;
			break;
		}
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pUnit == pRearUnits[a])
		{
			--numRearUnits;
			for(int b=a; b<numRearUnits; ++b)
				pRearUnits[b] = pRearUnits[b+1];
			pRearUnits[numRearUnits] = NULL;
			break;
		}
	}

	if(pUnit->IsHero())
		UpdateGroupStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
}

void Group::UpdateGroupStats()
{
	for(int a=0; a<numForwardUnits; ++a)
		pForwardUnits[a]->UpdateStats();

	for(int a=0; a<numRearUnits; ++a)
		pRearUnits[a]->UpdateStats();

	if(pVehicle)
		pVehicle->UpdateStats();
}

int Group::GetMovement()
{
	int movement = 0x7FFFFFFF;

	if(pVehicle)
	{
		return pVehicle->GetMovement();
	}
	else
	{
		for(int a=0; a<numForwardUnits; ++a)
		{
			movement = MFMin(movement, pForwardUnits[a]->GetMovement());
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			movement = MFMin(movement, pRearUnits[a]->GetMovement());
		}
	}

	return movement;
}

bool Group::SubtractMovementCost(MapTile *pTile)
{
	if(pVehicle)
	{
		int penalty = pVehicle->GetMovementPenalty(pTile);
		if(!penalty || pVehicle->GetMovement() < penalty)
			return false;

		pVehicle->Move(penalty);
	}
	else
	{
		for(int a=0; a<numForwardUnits; ++a)
		{
			int penalty = pForwardUnits[a]->GetMovementPenalty(pTile);
			if(!penalty || pForwardUnits[a]->GetMovement() < penalty)
				return false;
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			int penalty = pRearUnits[a]->GetMovementPenalty(pTile);
			if(!penalty || pRearUnits[a]->GetMovement() < penalty)
				return false;
		}

		// all units can move, lets go!
		for(int a=0; a<numForwardUnits; ++a)
		{
			int penalty = pForwardUnits[a]->GetMovementPenalty(pTile);
			pForwardUnits[a]->Move(penalty);
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			int penalty = pRearUnits[a]->GetMovementPenalty(pTile);
			pRearUnits[a]->Move(penalty);
		}
	}

	return true;
}

bool Group::IsInGroup(Unit *pUnit)
{
	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pUnit == pForwardUnits[a])
			return true;
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pUnit == pRearUnits[a])
			return true;
	}
	return false;
}

Unit *Group::GetHero()
{
	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pForwardUnits[a]->IsHero())
			return pForwardUnits[a];
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pRearUnits[a]->IsHero())
			return pRearUnits[a];
	}
	return NULL;
}

void Group::SetPlayer(int _player)
{
	player = _player;

	if(pVehicle)
		pVehicle->SetPlayer(_player);

	for(int a=0; a<numForwardUnits; ++a)
	{
		pForwardUnits[a]->SetPlayer(_player);
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		pRearUnits[a]->SetPlayer(_player);
	}
}

void Group::FindPath(int x, int y)
{
	pathX = x;
	pathY = y;

	if(bSelected)
		pPath = Game::GetCurrent()->GetMap()->FindPath(this, x, y);
}

bool Group::ValidateGroup()
{
	if(pVehicle && pVehicle->pGroup != this)
		return false;

	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pForwardUnits[a]->pGroup != this)
			return false;
	}

	for(int a=0; a<numRearUnits; ++a)
	{
		if(pRearUnits[a]->pGroup != this)
			return false;
	}

	return true;
}

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

Item::StatMod *Item::GetMod(Unit *pUnit, Unit *pHero, int type, int index)
{
	for(int a=0; a<numGroups; ++a)
	{
		for(int b=0; b<pMods[a].numUnits; ++b)
		{
			if((pUnit == pHero && !MFString_CaseCmp("self", pMods[a].ppTargets[b])) ||
				(pUnit != pHero && !MFString_CaseCmp("group", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Unit && !MFString_CaseCmp("units", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Hero && !MFString_CaseCmp("heroes", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Vehicle && !MFString_CaseCmp("vehicles", pMods[a].ppTargets[b])) ||
				!MFString_CaseCmp(pUnit->GetName(), pMods[a].ppTargets[b]))
			{
				switch(type)
				{
					case MT_Stat:
						return &pMods[a].mods[index];
					case MT_Defence:
						return &pMods[a].defence[index];
					case MT_Terrain:
						return &pMods[a].terrain[index];
					default:
						break;
				}
			}
		}
	}

	return NULL;
}

float Item::GetSpecial(Unit *pUnit, Unit *pHero, const char *pName)
{
	for(int a=0; a<numGroups; ++a)
	{
		for(int b=0; b<pMods[a].numUnits; ++b)
		{
			if((pUnit == pHero && !MFString_CaseCmp("self", pMods[a].ppTargets[b])) ||
				(pUnit != pHero && !MFString_CaseCmp("group", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Unit && !MFString_CaseCmp("units", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Hero && !MFString_CaseCmp("heroes", pMods[a].ppTargets[b])) ||
				(pUnit->GetType() == UT_Vehicle && !MFString_CaseCmp("vehicles", pMods[a].ppTargets[b])) ||
				!MFString_CaseCmp(pUnit->GetName(), pMods[a].ppTargets[b]))
			{
				if(pMods[a].pSpecial && !MFString_CaseCmp(pMods[a].pSpecial, pName))
					return pMods[a].probability;
			}
		}
	}

	return 0.f;
}
