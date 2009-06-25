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
					pNew->imageWidth = 1024;
					pNew->imageHeight = 1024;
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
						else if(pTerrain->IsSection("Speed"))
						{
							MFIniLine *pSpeeds = pTerrain->Sub();

							while(pSpeeds)
							{
								uint32 speed = MFString_AsciiToInteger(pSpeeds->GetString(1));
								pNew->pTerrainTypes[pSpeeds->GetInt(0)].speed = speed;
								pSpeeds = pSpeeds->Next();
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
						t.speed = pNew->GetTileSpeed(t.terrain);

						int numStrings = pTile->GetStringCount();
						for(int f = 7; f < numStrings; ++f)
						{
							if(pTile->IsString(f, "walk"))
								t.canWalk = 1;
							else if(pTile->IsString(f, "build"))
								t.canBuild = 1;
						}

						pTile = pTile->Next();
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
	MFHeap_Free(pTerrainTypes);
	MFHeap_Free(this);
}

void Tileset::DrawMap(int xTiles, int yTiles, uint8 *pTileData, int stride, int lineStride)
{
	MFTexture *pTex;
	int diffuse = MFMaterial_GetParameterIndexFromName(pTileMap, "diffuseMap");
	MFMaterial_GetParameter(pTileMap, diffuse, 0, &pTex);

	int textureWidth, textureHeight;
	MFTexture_GetTextureDimensions(pTex, &textureWidth, &textureHeight);

	float xScale = (1.f / textureWidth) * tileWidth;
	float yScale = (1.f / textureHeight) * tileHeight;
	float halfX = 0.5f / textureWidth;
	float halfY = 0.5f / textureHeight;

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

int Tileset::GetTileSpeed(uint32 terrain)
{
	uint32 t0 = terrain & 0xFF;
	uint32 t1 = (terrain >> 8) & 0xFF;
	uint32 t2 = (terrain >> 16) & 0xFF;
	uint32 t3 = (terrain >> 24) & 0xFF;
	return MFMax(MFMax(pTerrainTypes[t0].speed, pTerrainTypes[t1].speed), MFMax(pTerrainTypes[t0].speed, pTerrainTypes[t1].speed));
}

int Tileset::FindBestTiles(int *pTiles, uint32 tile, uint32 mask, int maxMatches)
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
