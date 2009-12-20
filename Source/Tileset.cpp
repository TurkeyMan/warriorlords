#include "Warlords.h"
#include "Tileset.h"

#include "MFIni.h"
#include "MFMaterial.h"
#include "MFTexture.h"
#include "MFPrimitive.h"

Tileset *Tileset::Create(const char *pFilename)
{
	MFIni *pIni = MFIni::Create(pFilename);
	if(!pIni)
		return NULL;

	Tileset *pNew = NULL;

	MFIniLine *pLine = pIni->GetFirstLine();

	while(pLine)
	{
		if(pLine->IsSection("Tilemap"))
		{
			pNew = (Tileset*)MFHeap_AllocAndZero(sizeof(Tileset));
			MFMemSet(pNew->tiles, 0xFF, sizeof(pNew->tiles));

			MFIniLine *pTilemap = pLine->Sub();

			while(pTilemap)
			{
				if(pTilemap->IsString(0, "name"))
				{
					MFString_Copy(pNew->name, pTilemap->GetString(1));
				}
				else if(pTilemap->IsString(0, "tilemap"))
				{
					pNew->pTileMap = MFMaterial_Create(MFStr_TruncateExtension(pTilemap->GetString(1)));

					if(pNew->pTileMap)
					{
						MFTexture *pTex;
						int diffuse = MFMaterial_GetParameterIndexFromName(pNew->pTileMap, "diffuseMap");
						MFMaterial_GetParameter(pNew->pTileMap, diffuse, 0, &pTex);
						if(pTex)
							MFTexture_GetTextureDimensions(pTex, &pNew->imageWidth, &pNew->imageHeight);
					}
				}
				else if(pTilemap->IsString(0, "road"))
				{
					pNew->pRoadMap = MFMaterial_Create(MFStr_TruncateExtension(pTilemap->GetString(1)));

					if(pNew->pRoadMap)
					{
						MFTexture *pTex;
						int diffuse = MFMaterial_GetParameterIndexFromName(pNew->pRoadMap, "diffuseMap");
						MFMaterial_GetParameter(pNew->pRoadMap, diffuse, 0, &pTex);
						if(pTex)
							MFTexture_GetTextureDimensions(pTex, &pNew->roadWidth, &pNew->roadHeight);
					}
				}
				else if(pTilemap->IsString(0, "water"))
				{
					pNew->pWater = MFMaterial_Create(MFStr_TruncateExtension(pTilemap->GetString(1)));
				}
				else if(pTilemap->IsString(0, "tile_width"))
				{
					pNew->tileWidth = pTilemap->GetInt(1);
				}
				else if(pTilemap->IsString(0, "tile_height"))
				{
					pNew->tileHeight = pTilemap->GetInt(1);
				}
				else if(pTilemap->IsSection("Terrain"))
				{
					pNew->terrainCount = 0;

					MFIniLine *pTerrain = pTilemap->Sub();
					while(pTerrain)
					{
						if(!pTerrain->IsString(0, "section"))
							pNew->terrainCount = MFMax(pNew->terrainCount, pTerrain->GetInt(0) + 1);

						pTerrain = pTerrain->Next();
					}

					pNew->pTerrainTypes = (TerrainType*)MFHeap_AllocAndZero(sizeof(TerrainType) * pNew->terrainCount);

					pTerrain = pTilemap->Sub();
					while(pTerrain)
					{
						if(!pTerrain->IsString(0, "section"))
							MFString_Copy(pNew->pTerrainTypes[pTerrain->GetInt(0)].name, pTerrain->GetString(1));
						else if(pTerrain->IsSection("Transitions"))
						{
							MFIniLine *pTransitions = pTerrain->Sub();

							while(pTransitions)
							{
								MFDebug_Assert(pTransitions->GetStringCount()-1 == pNew->terrainCount, "Invalid transition table dimensions!");
								for(int a=0; a<pNew->terrainCount; ++a)
									pNew->terrainTransitions[pTransitions->GetInt(0)][a] = pTransitions->GetInt(a + 1);

								pTransitions = pTransitions->Next();
							}
						}
						else if(pTerrain->IsSection("MapColours"))
						{
							MFIniLine *pColours = pTerrain->Sub();

							while(pColours)
							{
								uint32 colour = MFString_AsciiToInteger(pColours->GetString(1), true) | 0xFF000000;
								pNew->pTerrainTypes[pColours->GetInt(0)].mapColour.Set(((colour >> 16) & 0xFF) * (1.f/255.f), ((colour >> 8) & 0xFF) * (1.f/255.f), (colour & 0xFF) * (1.f/255.f));
								pColours = pColours->Next();
							}
						}

						pTerrain = pTerrain->Next();
					}
				}
				else if(pTilemap->IsSection("Tiles"))
				{
					MFIniLine *pTile = pTilemap->Sub();
					while(pTile)
					{
						int x = pTile->GetInt(0);
						int y = pTile->GetInt(1);
						MFDebug_Assert(x < 16 && y < 16, "Tile sheets may have a maximum of 16x16 tiles.");
						MFDebug_Assert(pTile->IsString(2, "="), "Expected '='.");

						int i = (x & 0xF) | ((y & 0xF) << 4);
						Tile &t = pNew->tiles[i];

						t.x = x;
						t.y = y;
						t.terrain = EncodeTile(pTile->GetInt(3), pTile->GetInt(4), pTile->GetInt(5), pTile->GetInt(6));
						t.bias = 255;
						t.canBuild = 0;

						int numStrings = pTile->GetStringCount();
						for(int f = 7; f < numStrings; ++f)
						{
							const char *pDetail = pTile->GetString(f);
							if(!MFString_CaseCmp(pDetail, "build"))
								t.canBuild = 1;
							else if(pDetail[MFString_Length(pDetail)-1] == '%')
							{
								int percent = MFString_AsciiToInteger(pDetail);
								t.bias = (percent * 255) / 100;
							}
						}

						pTile = pTile->Next();
					}
				}
				else if(pTilemap->IsSection("Road"))
				{
					pNew->roadCount = 0;
					int r = 0;

					MFIniLine *pRoad = pTilemap->Sub();
					while(pRoad)
					{
						++pNew->roadCount;
						pRoad = pRoad->Next();
					}

					pNew->pRoads = (Road*)MFHeap_AllocAndZero(sizeof(Road) * pNew->roadCount);

					pRoad = pTilemap->Sub();
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

				pTilemap = pTilemap->Next();
			}
		}

		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	return pNew;
}

void Tileset::Destroy()
{
	MFMaterial_Destroy(pTileMap);
	MFMaterial_Destroy(pWater);
	MFMaterial_Destroy(pRoadMap);
	MFHeap_Free(pTerrainTypes);
	MFHeap_Free(pRoads);
	MFHeap_Free(this);
}

void Tileset::DrawMap(int xTiles, int yTiles, uint8 *pTileData, int stride, int lineStride, float texelOffset)
{
	float xScale = (1.f / imageWidth) * tileWidth;
	float yScale = (1.f / imageHeight) * tileHeight;
	float halfX = texelOffset / imageWidth;
	float halfY = texelOffset / imageHeight;

	MFMaterial_SetMaterial(pWater);

	MFPrimitive(PT_TriList);
	MFBegin(6*xTiles*yTiles);
	MFSetColour(MFVector::white);

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			MFSetTexCoord1(0 + (0.5f/64.f), 0 + (0.5f/64.f));
			MFSetPosition((float)x, (float)y, 0);
			MFSetTexCoord1(1 + (0.5f/64.f), 0 + (0.5f/64.f));
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1(0 + (0.5f/64.f), 1 + (0.5f/64.f));
			MFSetPosition((float)x, (float)(y+1), 0);

			MFSetTexCoord1(1 + (0.5f/64.f), 0 + (0.5f/64.f));
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1(1 + (0.5f/64.f), 1 + (0.5f/64.f));
			MFSetPosition((float)(x+1), (float)(y+1), 0);
			MFSetTexCoord1(0 + (0.5f/64.f), 1 + (0.5f/64.f));
			MFSetPosition((float)x, (float)(y+1), 0);
		}
	}

	MFEnd();

	MFMaterial_SetMaterial(pTileMap);

	MFPrimitive(PT_TriList);
	MFBegin(6*xTiles*yTiles);
	MFSetColour(MFVector::white);

	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			Tile &t = tiles[pTileData[x*stride]];

			MFSetTexCoord1(t.x*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)x, (float)y, 0);
			MFSetTexCoord1((t.x+1)*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1(t.x*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)x, (float)(y+1), 0);

			MFSetTexCoord1((t.x+1)*xScale + halfX, t.y*yScale + halfY);
			MFSetPosition((float)(x+1), (float)y, 0);
			MFSetTexCoord1((t.x+1)*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)(x+1), (float)(y+1), 0);
			MFSetTexCoord1(t.x*xScale + halfX, (t.y+1)*yScale + halfY);
			MFSetPosition((float)x, (float)(y+1), 0);
		}

		pTileData += lineStride*stride;
	}

	MFEnd();
}

int Tileset::FindBestTiles(int *pTiles, uint32 tile, uint32 mask, int maxMatches) const
{
	uint32 maskedTile = tile & mask;

	int found = 0;
	int error = 256;
	for(int a=0; a<256; ++a)
	{
		if((tiles[a].terrain & mask) == maskedTile)
		{
			int e = GetTileError(tiles[a].terrain, tile);
			if(e < error)
			{
				error = e;
				pTiles[0] = a;
				found = 1;
			}
			else if(e == error && found < maxMatches)
				pTiles[found++] = a;
		}
	}

	return found;
}

int Tileset::FindBestRoads(int *pRoad, uint32 directions, uint32 terrain) const
{
	int numRoads = 0;
	int error = 4;

	for(int a=0; a<roadCount; ++a)
	{
		if(pRoads[a].terrain == terrain)
		{
			uint32 d = pRoads[a].directions ^ directions;
			int e = 0;
			for(int b=1; b<0x10; b<<=1)
			{
				if(d & b)
					++e;
			}

			if(e < error)
			{
				numRoads = 0;
				error = e;
			}
			if(e == error)
				pRoad[numRoads++] = a;
		}
	}

	return numRoads;
}

void Tileset::GetTileUVs(int tile, MFRect *pUVs)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = 0.5f / fWidth;
	float halfY = 0.5f / fHeight;

	Tile &t = tiles[tile];
	pUVs->x = t.x*xScale + halfX;
	pUVs->y = t.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

void Tileset::GetRoadUVs(int index, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)roadWidth;
	float fHeight = (float)roadHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	Road &r = pRoads[index];
	pUVs->x = r.x*xScale + halfX;
	pUVs->y = r.y*yScale + halfY;
	pUVs->width = xScale;
	pUVs->height = yScale;
}

int Tileset::FindRoad(uint32 directions, uint32 terrain) const
{
	for(int a=0; a<roadCount; ++a)
	{
		if(pRoads[a].terrain == terrain && pRoads[a].directions == directions)
			return a;
	}

	return -1;
}
