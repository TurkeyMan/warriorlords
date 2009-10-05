#include "Warlords.h"
#include "Unit.h"
#include "Castle.h"

#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFPrimitive.h"
#include "MFIni.h"

UnitDefinitions *UnitDefinitions::Load(const char *pUnits)
{
  MFIni *pIni = MFIni::Create("Units");
  if(!pIni)
    return NULL;

  UnitDefinitions *pUnitDefs = new UnitDefinitions;

  pUnitDefs->pUnitDefs = pIni;

  pUnitDefs->pUnits = NULL;
  pUnitDefs->numUnits = 0;

  pUnitDefs->pDetailLayer = NULL;
  pUnitDefs->pColourLayer = NULL;
  pUnitDefs->pCastleMap = NULL;

  pUnitDefs->unitMapWidth = pUnitDefs->unitMapHeight = 0;
  pUnitDefs->castleMapWidth = pUnitDefs->castleMapHeight = 0;

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
      pUnitDefs->pDetailLayer = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

      if(pUnitDefs->pDetailLayer)
      {
	      MFTexture *pTex;
	      int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pDetailLayer, "diffuseMap");
	      MFMaterial_GetParameter(pUnitDefs->pDetailLayer, diffuse, 0, &pTex);
        if(pTex)
  	      MFTexture_GetTextureDimensions(pTex, &pUnitDefs->unitMapWidth, &pUnitDefs->unitMapHeight);
      }
    }
    else if(pLine->IsString(0, "colour_map"))
    {
      pUnitDefs->pColourLayer = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));
    }
    else if(pLine->IsString(0, "castle_map"))
    {
      pUnitDefs->pCastleMap = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));

      if(pUnitDefs->pCastleMap)
      {
	      MFTexture *pTex;
	      int diffuse = MFMaterial_GetParameterIndexFromName(pUnitDefs->pDetailLayer, "diffuseMap");
	      MFMaterial_GetParameter(pUnitDefs->pDetailLayer, diffuse, 0, &pTex);
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

				for(int a=2; a<pSpecial->GetStringCount(); ++a)
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

        pUnit->attack = pUnits->GetInt(8);
        pUnit->defence = pUnits->GetInt(9);
        pUnit->movement = pUnits->GetInt(10);

        pUnit->attackClass = pUnits->GetInt(11);
        pUnit->defenceClass = pUnits->GetInt(12);
        pUnit->movementClass = pUnits->GetInt(13);

        pUnit->attackSpeed = pUnits->GetInt(14);
        pUnit->life = pUnits->GetInt(15);

        ++pUnit;
        pUnits = pUnits->Next();
      }
    }
    else if(pLine->IsSection("Classes"))
    {

    }

    pLine = pLine->Next();
  }

  pUnitDefs->numRenderUnits = 0;

  return pUnitDefs;
}

void UnitDefinitions::Free()
{
  if(pDetailLayer)
    MFMaterial_Destroy(pDetailLayer);
  if(pColourLayer)
    MFMaterial_Destroy(pColourLayer);
  if(pCastleMap)
    MFMaterial_Destroy(pCastleMap);

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
	return MakeVector(((c >> 16) & 0xFF) * (1.f/255.f), ((c >> 8) & 0xFF) * (1.f/255.f), (c & 0xFF) * (1.f/255.f));
}

Unit *UnitDefinitions::CreateUnit(int unit, int race)
{
  Unit *pUnit = new Unit;

  pUnit->id = unit;
  pUnit->race = race;

  pUnit->pUnitDefs = this;
  pUnit->details = pUnits[unit];
  pUnit->kills = pUnit->victories = 0;
  pUnit->life = pUnit->details.life;
  pUnit->movement = pUnit->details.movement;
  pUnit->pName = pUnit->details.pName;

  return pUnit;
}

void UnitDefinitions::AddRenderUnit(int unit, float x, float y, int race, bool bFlip)
{
  renderUnits[numRenderUnits].unit = unit;
  renderUnits[numRenderUnits].x = x;
  renderUnits[numRenderUnits].y = y;
  renderUnits[numRenderUnits].race = race;
  renderUnits[numRenderUnits].bFlip = bFlip;
  ++numRenderUnits;
}

void UnitDefinitions::DrawUnits(float texelOffset)
{
  if(!numRenderUnits)
    return;

	float xScale = (1.f / unitMapWidth) * tileWidth;
	float yScale = (1.f / unitMapHeight) * tileHeight;
	float halfX = texelOffset / unitMapWidth;
	float halfY = texelOffset / unitMapHeight;

  float scale = 64.f;

  for(int a=0; a<2; ++a)
  {
    MFMaterial_SetMaterial(a == 0 ? pDetailLayer : pColourLayer);

	  MFPrimitive(PT_TriList);
	  MFBegin(6*numRenderUnits);

    if(a == 0)
      MFSetColour(MFVector::white);

	  for(int u=0; u<numRenderUnits; ++u)
	  {
      UnitRender &unit = renderUnits[u];
      UnitDetails &def = pUnits[unit.unit];

      if(a == 1)
        MFSetColour(GetRaceColour(unit.race));

      float depth = (1000.f - unit.y) / 1000.f;

      float x0 = unit.x + (unit.bFlip ? (def.x+def.width)*xScale - halfX : def.x*xScale + halfX);
      float x1 = unit.x + (unit.bFlip ? def.x*xScale - halfX : (def.x+def.width)*xScale + halfX);

      MFSetTexCoord1(x0, def.y*yScale + halfY);
			MFSetPosition(unit.x, unit.y, depth);
			MFSetTexCoord1(x1, def.y*yScale + halfY);
      MFSetPosition(unit.x+def.width*scale, unit.y, depth);
			MFSetTexCoord1(x0, (def.y+def.height)*yScale + halfY);
			MFSetPosition(unit.x, unit.y+def.height*scale, depth);

			MFSetTexCoord1(x1, def.y*yScale + halfY);
			MFSetPosition(unit.x+def.width*scale, unit.y, depth);
			MFSetTexCoord1(x1, (def.y+def.height)*yScale + halfY);
			MFSetPosition(unit.x+def.width*scale, unit.y+def.height*scale, depth);
			MFSetTexCoord1(x0, (def.y+def.height)*yScale + halfY);
			MFSetPosition(unit.x, unit.y+def.height*scale, depth);
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
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void Unit::Destroy()
{
  delete this;
}

void Unit::Draw(float x, float y, bool bFlip)
{
  pUnitDefs->AddRenderUnit(id, x, y, race, bFlip);
}
