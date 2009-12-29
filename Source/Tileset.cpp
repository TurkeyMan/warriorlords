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
	float waterOffset = texelOffset / 64.f;

	struct RenderTile
	{
		uint8 x, y, tx, ty, flags;
	} renderTiles[1024];

	int numTiles = 0;
	int numTerrainTiles = 0;
	int numWaterTiles = 0;

	// count the tiles to render
	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			renderTiles[numTiles].x = x;
			renderTiles[numTiles].y = y;

			Tile &t = tiles[pTileData[x*stride]];
			renderTiles[numTiles].tx = t.x;
			renderTiles[numTiles].ty = t.y;

			int water = (int)TileContains(t.terrain, 1);
			renderTiles[numTiles].flags = water;
			numWaterTiles += water;

			if(t.terrain != 0x01010101)
			{
				++numTerrainTiles;
				renderTiles[numTiles].flags |= 2;
			}

			++numTiles;
		}
		pTileData += lineStride*stride;
	}

	// render any water tiles
	if(numWaterTiles)
	{
		MFMaterial_SetMaterial(pWater);

		MFPrimitive(PT_QuadList);
		MFBegin(numWaterTiles*2);

		for(int a=0; a<numTiles; ++a)
		{
			if(renderTiles[a].flags & 1)
			{
				MFSetTexCoord1(0 + waterOffset, 0 + waterOffset);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(1 + waterOffset, 1 + waterOffset);
				MFSetPosition((float)renderTiles[a].x + 1.f, (float)renderTiles[a].y + 1.f, 0);
			}
		}

		MFEnd();
	}

	// render any terrain tiles
	if(numTerrainTiles)
	{
		MFMaterial_SetMaterial(pTileMap);

		MFPrimitive(PT_QuadList);
		MFBegin(numTerrainTiles*2);

		for(int a=0; a<numTiles; ++a)
		{
			if(renderTiles[a].flags & 2)
			{
				MFSetTexCoord1(renderTiles[a].tx*xScale + halfX, renderTiles[a].ty*xScale + halfX);
				MFSetPosition((float)renderTiles[a].x, (float)renderTiles[a].y, 0);
				MFSetTexCoord1(((float)renderTiles[a].tx + 1.f)*xScale + halfX, ((float)renderTiles[a].ty + 1.f)*xScale + halfX);
				MFSetPosition((float)renderTiles[a].x + 1.f, (float)renderTiles[a].y + 1.f, 0);
			}
		}

		MFEnd();
	}
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

void Tileset::GetTileUVs(int tile, MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

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

void Tileset::GetWaterUVs(MFRect *pUVs, float texelOffset)
{
	float fWidth = (float)tileWidth;
	float fHeight = (float)tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	pUVs->x = halfX;
	pUVs->y = halfY;
	pUVs->width = 1.f;
	pUVs->height = 1.f;
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
