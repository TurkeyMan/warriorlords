#include "Warlords.h"
#include "Unit.h"
#include "Castle.h"

#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFPrimitive.h"
#include "MFIni.h"

UnitDefinitions *UnitDefinitions::Load(const char *pUnits, const CastleSet *pCastleSet)
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
    else if(pLine->IsString(0, "detail"))
    {
      pUnitDefs->pDetailLayer = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));
    }
    else if(pLine->IsString(0, "colour"))
    {
      pUnitDefs->pColourLayer = MFMaterial_Create(MFStr_TruncateExtension(pLine->GetString(1)));
    }
    else if(pLine->IsString(0, "tile_width"))
    {
      pUnitDefs->tileWidth = pLine->GetInt(1);
    }
    else if(pLine->IsString(0, "tile_height"))
    {
      pUnitDefs->tileHeight = pLine->GetInt(1);
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

  pUnitDefs->pCastles = pCastleSet;

  pUnitDefs->numRenderUnits = 0;

  return pUnitDefs;
}

void UnitDefinitions::Free()
{
  if(pDetailLayer)
    MFMaterial_Destroy(pDetailLayer);
  if(pColourLayer)
    MFMaterial_Destroy(pColourLayer);

  if(pUnits)
    MFHeap_Free(pUnits);

  if(pUnitDefs)
    MFIni::Destroy(pUnitDefs);

  delete this;
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

	MFTexture *pTex;
	int diffuse = MFMaterial_GetParameterIndexFromName(pDetailLayer, "diffuseMap");
	MFMaterial_GetParameter(pDetailLayer, diffuse, 0, &pTex);

	int textureWidth, textureHeight;
	MFTexture_GetTextureDimensions(pTex, &textureWidth, &textureHeight);

	float xScale = (1.f / textureWidth) * tileWidth;
	float yScale = (1.f / textureHeight) * tileHeight;
	float halfX = texelOffset / textureWidth;
	float halfY = texelOffset / textureHeight;

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
        MFSetColour(pCastles->GetRaceColour(unit.race));

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

void Unit::Destroy()
{
  delete this;
}

void Unit::Draw(float x, float y, bool bFlip)
{
  pUnitDefs->AddRenderUnit(id, x, y, race, bFlip);
}
