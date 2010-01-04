#include "Warlords.h"
#include "Unit.h"
#include "Castle.h"

#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFPrimitive.h"
#include "MFIni.h"

extern Game *pGame;

UnitDefinitions *UnitDefinitions::Load(Game *pGame, const char *pUnits, int numTerrainTypes)
{
	MFIni *pIni = MFIni::Create("Units");
	if(!pIni)
		return NULL;

	UnitDefinitions *pUnitDefs = new UnitDefinitions;
	pUnitDefs->units.Init("Unit List", 4096);
	pUnitDefs->pGame = pGame;

	pUnitDefs->pUnitDefs = pIni;

	pUnitDefs->pUnits = NULL;
	pUnitDefs->numUnits = 0;

	pUnitDefs->pUnitMat = NULL;
	pUnitDefs->pHeadMat = NULL;
	pUnitDefs->pCastleMat = NULL;

	pUnitDefs->unitMapWidth = pUnitDefs->unitMapHeight = 0;
	pUnitDefs->castleMapWidth = pUnitDefs->castleMapHeight = 0;

	pUnitDefs->numArmourClasses = pUnitDefs->numWeaponClasses = pUnitDefs->numMovementClasses = 0;
	pUnitDefs->ppArmourClasses = NULL;
	pUnitDefs->pWeaponClasses = NULL;
	pUnitDefs->pMovementClasses = NULL;

	pUnitDefs->numTerrainTypes = numTerrainTypes;

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
				MFTexture *pTex;
				int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pUnitMat, "diffuseMap");
				MFMaterial_GetParameter(pUnitDefs->pUnitMat, diffuse, 0, &pTex);
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
				MFTexture *pTex;
				int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pCastleMat, "diffuseMap");
				MFMaterial_GetParameter(pUnitDefs->pCastleMat, diffuse, 0, &pTex);
				if(pTex)
					MFTexture_GetTextureDimensions(pTex, &pUnitDefs->castleMapWidth, &pUnitDefs->castleMapHeight);
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
				++pUnitDefs->numUnits;
				pUnits = pUnits->Next();
			}

			pUnitDefs->pUnits = (UnitDetails*)MFHeap_AllocAndZero(pUnitDefs->numUnits * sizeof(UnitDetails));
			UnitDetails *pUnit = pUnitDefs->pUnits;

			pUnits = pLine->Sub();
			while(pUnits)
			{
				pUnit->x = pUnits->GetInt(0);
				pUnit->y = pUnits->GetInt(1);
				pUnit->width = pUnits->GetInt(2);
				pUnit->height = pUnits->GetInt(3);

				pUnit->pName = pUnits->GetString(5);
				if(pUnits->IsString(6, "hero"))
					pUnit->type = UT_Hero;
				else if(pUnits->IsString(6, "unit"))
					pUnit->type = UT_Unit;
				else if(pUnits->IsString(6, "vehicle"))
					pUnit->type = UT_Vehicle;

				pUnit->race = pUnits->GetInt(7);

				pUnit->attackMin = pUnits->GetInt(8);
				pUnit->attackMax = pUnits->GetInt(9);
				pUnit->movement = pUnits->GetInt(10);

				pUnit->attackClass = pUnits->GetInt(11);
				pUnit->defenceClass = pUnits->GetInt(12);
				pUnit->movementClass = pUnits->GetInt(13);

				pUnit->cooldown = pUnits->GetFloat(14);
				pUnit->attackSpeed = pUnits->GetFloat(15);
				pUnit->life = pUnits->GetInt(16);

				pUnit->buildTime = pUnits->GetInt(17);
				pUnit->cost = pUnits->GetInt(18);

				++pUnit;
				pUnits = pUnits->Next();
			}
		}
		else if(pLine->IsSection("Classes"))
		{
			MFIniLine *pClasses = pLine->Sub();
			MFIniLine *pWeaponClasses = NULL;
			while(pClasses)
			{
				if(pClasses->IsSection("Armour"))
				{
					MFIniLine *pArmour = pClasses->Sub();
					pUnitDefs->numArmourClasses = 0;
					while(pArmour)
					{
						++pUnitDefs->numArmourClasses;
						pArmour = pArmour->Next();
					}

					pUnitDefs->ppArmourClasses = (const char **)MFHeap_Alloc(sizeof(const char **)*pUnitDefs->numArmourClasses);

					pArmour = pClasses->Sub();
					while(pArmour)
					{
						pUnitDefs->ppArmourClasses[pArmour->GetInt(0)] = pArmour->GetString(1);
						pArmour = pArmour->Next();
					}
				}
				else if(pClasses->IsSection("Weapon"))
				{
					pWeaponClasses = pClasses->Sub();
				}
				else if(pClasses->IsSection("Movement"))
				{
					MFIniLine *pMovement = pClasses->Sub();
					pUnitDefs->numWeaponClasses = 0;
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
							pUnitDefs->pMovementClasses[movClass].pMovementPenalty[a] = pMovement->GetInt(2 + a);

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

				pClasses = pClasses->Next();
			}

			if(pWeaponClasses)
			{
				MFIniLine *pWeapon = pWeaponClasses;
				pUnitDefs->numWeaponClasses = 0;
				while(pWeapon)
				{
					++pUnitDefs->numWeaponClasses;
					pWeapon = pWeapon->Next();
				}

				pUnitDefs->pWeaponClasses = (WeaponClass*)MFHeap_Alloc(sizeof(const char *)*pUnitDefs->numWeaponClasses);

				pWeapon = pWeaponClasses;
				while(pWeapon)
				{
					int weaponClass = pWeapon->GetInt(0);
					WeaponClass &wpnClass = pUnitDefs->pWeaponClasses[weaponClass];
					wpnClass.pAttackModifiers = (float*)MFHeap_Alloc(sizeof(float)*pUnitDefs->numArmourClasses);

					wpnClass.pName = pWeapon->GetString(1);
					wpnClass.ranged = pWeapon->GetInt(pUnitDefs->numArmourClasses + 2);

					for(int a=0; a<pUnitDefs->numArmourClasses; ++a)
						wpnClass.pAttackModifiers[a] = pWeapon->GetFloat(2 + a);

					pWeapon = pWeapon->Next();
				}
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

	if(pRaces)
		MFHeap_Free(pRaces);

	if(pUnits)
		MFHeap_Free(pUnits);

	if(pSpecials)
		MFHeap_Free(pSpecials);

	if(pUnitDefs)
		MFIni::Destroy(pUnitDefs);

	units.Deinit();

	delete this;
}

MFVector UnitDefinitions::GetRaceColour(int race) const
{
	uint32 c = pRaces[race].colour;
	return MakeVector(((c >> 16) & 0xFF) * (1.f/255.f), ((c >> 8) & 0xFF) * (1.f/255.f), (c & 0xFF) * (1.f/255.f));
}

Unit *UnitDefinitions::CreateUnit(int unit, int player)
{
	Unit *pUnit = units.Create();

	pUnit->id = unit;
	pUnit->player = player;

	pUnit->pUnitDefs = this;
	pUnit->details = pUnits[unit];
	pUnit->kills = pUnit->victories = 0;
	pUnit->life = pUnit->details.life;
	pUnit->movement = pUnit->details.movement * 2;
	pUnit->pName = pUnit->details.pName;

	return pUnit;
}

void UnitDefinitions::DestroyUnit(Unit *pUnits)
{
	units.Destroy(pUnits);
}

void UnitDefinitions::AddRenderUnit(int unit, float x, float y, int player, bool bFlip, float alpha)
{
	renderUnits[numRenderUnits].unit = unit;
	renderUnits[numRenderUnits].x = x;
	renderUnits[numRenderUnits].y = y;
	renderUnits[numRenderUnits].player = player;
	renderUnits[numRenderUnits].alpha = alpha;
	renderUnits[numRenderUnits].bFlip = bFlip;
	++numRenderUnits;
}

void UnitDefinitions::DrawUnits(float scale, float texelOffset, bool bHead)
{
	if(!numRenderUnits)
		return;

	MFMaterial_SetMaterial(bHead ? pHeadMat : pUnitMat);

	MFPrimitive(PT_TriList);
	MFBegin(6*numRenderUnits);

	for(int u=0; u<numRenderUnits; ++u)
	{
		UnitRender &unit = renderUnits[u];
		UnitDetails &def = pUnits[unit.unit];

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
	pUVs->width = (bFlip ? -def.width : def.width)*yScale;
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

void Unit::Destroy()
{
	pUnitDefs->DestroyUnit(this);
}

void Unit::Draw(float x, float y, bool bFlip, float alpha)
{
	pUnitDefs->AddRenderUnit(id, x, y, player, bFlip, alpha);
}

int Unit::GetRace()
{
	return pGame->GetPlayerRace(player);
}

MFVector Unit::GetColour()
{
	return pGame->GetPlayerColour(player);
}

int Unit::GetMovementPenalty(int terrainType)
{
	return pUnitDefs->GetMovementPenalty(details.movementClass, terrainType);
}

int Unit::GetMovementPenalty(MapTile *pTile)
{
	ObjectType type = pTile->GetType();
	if(type == OT_Road || (type == OT_Castle && pTile->IsFriendlyTile(player)))
		return 1;

	uint32 terrain = pTile->GetTerrain();
	int penalty = pUnitDefs->GetMovementPenalty(details.movementClass, terrain & 0xFF);
	penalty = MFMax(penalty, pUnitDefs->GetMovementPenalty(details.movementClass, (terrain >> 8) & 0xFF));
	penalty = MFMax(penalty, pUnitDefs->GetMovementPenalty(details.movementClass, (terrain >> 16) & 0xFF));
	penalty = MFMax(penalty, pUnitDefs->GetMovementPenalty(details.movementClass, (terrain >> 24) & 0xFF));
	return penalty;
}

void Unit::Restore()
{
	// restore movement
	movement = details.movement * 2;

	// heal 25% life
	life = MFMin(life + (details.life + 3) / 4, details.life);
}

void Castle::Init(CastleDetails *pDetails, int _player, UnitDefinitions *_pUnitDefs)
{
	pUnitDefs = _pUnitDefs;

	details = *pDetails;
	player = _player;

	building = -1;
	buildTime = 0;
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
	if(pTile->GetNumGroups() != 0 || pTile[1].GetNumGroups() != 0)
		return false;

	// check the second row... very painfully mind you.
	int width;
	Map *pMap = pUnitDefs->GetGame()->GetMap();
	pMap->GetMapSize(&width, NULL);
	MapTile *pT = pTile + width;
	return pT[0].GetNumGroups() == 0 && pTile[1].GetNumGroups() == 0;
}

Group *Castle::GetMercGroup()
{
	static const int odds[16] = { 1, 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 4 };

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

	int numUnits = odds[MFRand() & 0xF];
	for(int a=0; a<numUnits; ++a)
	{
		pGroup->AddUnit(pGame->GetUnitDefs()->CreateUnit(choices[MFRand()%numChoices], -1));
	}

	return pGroup;
}

void Castle::Capture(int _player)
{
	player = _player;
	building = -1;
	buildTime = 0;
}

void Castle::SetBuildUnit(int slot)
{
	int unit = details.buildUnits[slot].unit;
	UnitDetails *pUnit = pUnitDefs->GetUnitDetails(unit);

	building = slot;
	buildTime = pUnit->buildTime + details.buildUnits[slot].buildTimeMod;
}

int Castle::GetBuildUnit()
{
	if(building == -1)
		return -1;
	return details.buildUnits[building].unit;
}

Group *Group::Create(int _player)
{
	Group * pGroup = new Group;
	pGroup->player = _player;
	pGroup->numForwardUnits = pGroup->numRearUnits = 0;
	pGroup->forwardPlan = AP_AttackFront;
	pGroup->rearPlan = AP_AttackRear;
	pGroup->bSelected = false;
	pGroup->pPath = NULL;
	pGroup->pNext = NULL;
	pGroup->pVehicle = NULL;
	return pGroup;
}

void Group::Destroy()
{
	if(pVehicle)
		pVehicle->Destroy();
	delete this;
}

bool Group::AddUnit(Unit *pUnit)
{
	if(pUnit->IsVehicle())
	{
		if(pVehicle)
			return false;
		pVehicle = pUnit;
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

	return true;
}

bool Group::AddForwardUnit(Unit *pUnit)
{
	if(numForwardUnits >= 5)
		return false;

	pForwardUnits[numForwardUnits++] = pUnit;

	return true;
}

bool Group::AddRearUnit(Unit *pUnit)
{
	if(numRearUnits >= 5)
		return false;

	pRearUnits[numRearUnits++] = pUnit;

	return true;
}

void Group::RemoveUnit(Unit *pUnit)
{
	if(pUnit->IsVehicle() && pVehicle == pUnit)
	{
		pVehicle = NULL;
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
}

void Group::SwapUnits(Unit *pUnit1, Unit *pUnit2)
{
	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pForwardUnits[a] == pUnit1)
			pForwardUnits[a] = pUnit2;
		else if(pForwardUnits[a] == pUnit2)
			pForwardUnits[a] = pUnit1;
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pRearUnits[a] == pUnit1)
			pRearUnits[a] = pUnit2;
		else if(pRearUnits[a] == pUnit2)
			pRearUnits[a] = pUnit1;
	}	
}

int Group::GetMovement()
{
	int movement = 0x7FFFFFFF;

	for(int a=0; a<numForwardUnits; ++a)
	{
		movement = MFMin(movement, pForwardUnits[a]->GetMovement());
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		movement = MFMin(movement, pRearUnits[a]->GetMovement());
	}

	return movement;
}

bool Group::SubtractMovementCost(MapTile *pTile)
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
