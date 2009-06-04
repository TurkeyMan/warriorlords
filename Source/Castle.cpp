#include "Warlords.h"
#include "Castle.h"
#include "Tileset.h"

#include "MFIni.h"
#include "MFMaterial.h"
#include "MFPrimitive.h"

CastleSet *CastleSet::Create(const char *pFilename)
{
	MFIni *pIni = MFIni::Create(pFilename);
	if(!pIni)
		return NULL;

	CastleSet *pNew = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();

	while(pLine)
	{
		if(pLine->IsSection("CastleSet"))
		{
			pNew = (CastleSet*)MFHeap_AllocAndZero(sizeof(CastleSet));

			MFIniLine *pCastleSet = pLine->Sub();

			while(pCastleSet)
			{
				if(pCastleSet->IsString(0, "name"))
				{
					MFString_Copy(pNew->name, pCastleSet->GetString(1));
				}
				else if(pCastleSet->IsString(0, "image"))
				{
					pNew->pImage = MFMaterial_Create(MFStr_TruncateExtension(pCastleSet->GetString(1)));
					pNew->imageWidth = 512;
					pNew->imageHeight = 512;
				}
				else if(pCastleSet->IsString(0, "roads"))
				{
					pNew->pRoadMap = MFMaterial_Create(MFStr_TruncateExtension(pCastleSet->GetString(1)));
				}
				else if(pCastleSet->IsString(0, "tile_width"))
				{
					pNew->tileWidth = pCastleSet->GetInt(1);
				}
				else if(pCastleSet->IsString(0, "tile_height"))
				{
					pNew->tileHeight = pCastleSet->GetInt(1);
				}
				else if(pCastleSet->IsSection("Races"))
				{
					pNew->raceCount = 0;

					MFIniLine *pRace = pCastleSet->Sub();
					while(pRace)
					{
						pNew->raceCount = MFMax(pNew->raceCount, pRace->GetInt(0) + 1);
						pRace = pRace->Next();
					}

					pNew->pRaces = (Race*)MFHeap_AllocAndZero(sizeof(Race) * pNew->raceCount);

					pRace = pCastleSet->Sub();
					while(pRace)
					{
						int r = pRace->GetInt(0);
						MFString_Copy(pNew->pRaces[r].name, pRace->GetString(1));

						pNew->pRaces[r].castlex = pRace->GetInt(2);
						pNew->pRaces[r].castley = pRace->GetInt(3);
						pNew->pRaces[r].flagx = pRace->GetInt(4);
						pNew->pRaces[r].flagx = pRace->GetInt(5);
						pNew->pRaces[r].colour = MFString_AsciiToInteger(pRace->GetString(6), true) | 0xFF000000;

						pRace = pRace->Next();
					}
				}
				else if(pCastleSet->IsSection("Special"))
				{
					pNew->specialCount = 0;

					MFIniLine *pSpecial = pCastleSet->Sub();
					while(pSpecial)
					{
						pNew->specialCount = MFMax(pNew->specialCount, pSpecial->GetInt(0) + 1);
						pSpecial = pSpecial->Next();
					}

					pNew->pSpecials = (Special*)MFHeap_AllocAndZero(sizeof(Special) * pNew->specialCount);

					pSpecial = pCastleSet->Sub();
					while(pSpecial)
					{
						int s = pSpecial->GetInt(0);
						MFString_Copy(pNew->pSpecials[s].name, pSpecial->GetString(1));

						for(int a=2; a<pSpecial->GetStringCount(); ++a)
						{
							if(pSpecial->IsString(a, "searchable"))
								pNew->pSpecials[s].canSearch = 1;
						}

						pSpecial = pSpecial->Next();
					}
				}
				else if(pCastleSet->IsSection("Road"))
				{
					pNew->roadCount = 0;
					int r = 0;

					MFIniLine *pRoad = pCastleSet->Sub();
					while(pRoad)
					{
						++pNew->roadCount;
						pRoad = pRoad->Next();
					}

					pNew->pRoads = (Road*)MFHeap_AllocAndZero(sizeof(Road) * pNew->roadCount);

					pRoad = pCastleSet->Sub();
					while(pRoad)
					{
						int x = pRoad->GetInt(0);
						int y = pRoad->GetInt(1);
						MFDebug_Assert(x < 16 && y < 16, "Tile sheets may have a maximum of 16x16 tiles.");
						MFDebug_Assert(pRoad->IsString(2, "="), "Expected '='.");

						Road &road = pNew->pRoads[r++];
						road.x = x;
						road.y = y;
						road.directions = (pRoad->GetInt(3) << 3) | (pRoad->GetInt(4) << 2) | (pRoad->GetInt(5) << 1) | pRoad->GetInt(6);
						road.terrain = EncodeTile(pRoad->GetInt(7), pRoad->GetInt(8), pRoad->GetInt(9), pRoad->GetInt(10));

						pRoad = pRoad->Next();
					}
				}

				pCastleSet = pCastleSet->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	return pNew;
}

void CastleSet::Destroy()
{
	MFMaterial_Destroy(pImage);
	MFHeap_Free(pRaces);
	MFHeap_Free(pSpecials);
	MFHeap_Free(this);
}

int CastleSet::DrawCastle(int race)
{
	return 0;
}

int CastleSet::DrawFlag(int race)
{
	return 0;
}

int CastleSet::DrawSpecial(int index)
{
	return 0;
}

void CastleSet::GetCastleUVs(int race, MFRect *pUVs)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = 0.5f / fWidth;
	float halfY = 0.5f / fHeight;

	Race &r = pRaces[race];
	pUVs->x = r.castlex*xScale + halfX;
	pUVs->y = r.castley*yScale + halfY;
	pUVs->width = xScale*2;
	pUVs->height = yScale*2;
}

void CastleSet::GetFlagUVs(int race, MFRect *pUVs)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = 0.5f / fWidth;
	float halfY = 0.5f / fHeight;

	Race &r = pRaces[race];
	pUVs->x = r.flagx*xScale + halfX;
	pUVs->y = r.flagy*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void CastleSet::GetSpecialUVs(int index, MFRect *pUVs)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = 0.5f / fWidth;
	float halfY = 0.5f / fHeight;

	Special &s = pSpecials[index];
	pUVs->x = s.x*xScale + halfX;
	pUVs->y = s.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void CastleSet::GetRoadUVs(int index, MFRect *pUVs)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = 0.5f / fWidth;
	float halfY = 0.5f / fHeight;

	Road &r = pRoads[index];
	pUVs->x = r.x*xScale + halfX;
	pUVs->y = r.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

MFVector CastleSet::GetRaceColour(int race)
{
	uint32 c = pRaces[race].colour;
	return MakeVector(((c >> 16) & 0xFF) * (1.f/255.f), ((c >> 8) & 0xFF) * (1.f/255.f), (c & 0xFF) * (1.f/255.f));
}

int CastleSet::FindRoad(uint32 directions, uint32 terrain)
{
	for(int a=0; a<roadCount; ++a)
	{
		if(pRoads[a].terrain == terrain && pRoads[a].directions == directions)
			return a;
	}

	return -1;
}
