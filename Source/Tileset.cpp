#include "Warlords.h"
#include "Tileset.h"

#include "Fuji/MFIni.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/Materials/MFMat_Standard.h"
#include "Fuji/MFTexture.h"
#include "Fuji/MFPrimitive.h"

static int gTilesetResource = -1;

void Tileset::Init()
{
	gTilesetResource = MFResource_Register("Tileset", &Tileset::Destroy);
}

Tileset *Tileset::Create(MFString filename)
{
	Tileset *pTileset = (Tileset*)MFResource_Find(filename.GetHash());
	if(!pTileset)
	{
		MFIni *pIni = MFIni::Create(filename.CStr());
		if(!pIni)
			return NULL;

		MFIniLine *pLine = pIni->GetFirstLine();
		while(pLine)
		{
			if(pLine->IsSection("Tilemap"))
			{
				pTileset = new Tileset(pLine);
				break;
			}

			pLine = pLine->Next();
		}

		MFIni::Destroy(pIni);

		if(pTileset)
			MFResource_AddResource(pTileset, gTilesetResource, filename.GetHash(), pTileset->name.CStr());
	}
	return pTileset;
}

int Tileset::AddRef()
{
	return MFResource_AddRef(this);
}

int Tileset::Release()
{
	return MFResource_Release(this);
}

void Tileset::Destroy(MFResource *pRes)
{
	Tileset *pTileset = (Tileset*)pRes;
	delete pTileset;
}

void Tileset::LoadResources()
{
	if(resourceRefCount == 0)
	{
		pTileMap[0] = MFMaterial_Create(tileMap[0].CStr());
		pTileMap[1] = MFMaterial_Create(tileMap[1].CStr());
		pWater = MFMaterial_Create(waterMat.CStr());
		pRoadMap = MFMaterial_Create(roadMat.CStr());

		if(pTileMap[0])
		{
			MFTexture *pTex = MFMaterial_GetParameterT(pTileMap[0], MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
			if(pTex)
				MFTexture_GetTextureDimensions(pTex, &imageWidth, &imageHeight);
		}
		if(pTileMap[1])
		{
			MFTexture *pTex = MFMaterial_GetParameterT(pTileMap[1], MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
			if(pTex)
				MFTexture_GetTextureDimensions(pTex, &imageWidth, &imageHeight);
		}
		if(pRoadMap)
		{
			MFTexture *pTex = MFMaterial_GetParameterT(pRoadMap, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
			if(pTex)
				MFTexture_GetTextureDimensions(pTex, &roadWidth, &roadHeight);
		}
	}

	++resourceRefCount;
}

void Tileset::ReleaseResources()
{
	if(--resourceRefCount == 0)
	{
		MFMaterial_Release(pTileMap[0]);
		MFMaterial_Release(pTileMap[1]);
		MFMaterial_Release(pWater);
		MFMaterial_Release(pRoadMap);
	}
}

Tileset::Tileset(MFIniLine *pLine)
{
	resourceRefCount = 0;

	MFMemSet(tiles, 0xFF, sizeof(tiles));

	MFIniLine *pTilemap = pLine->Sub();

	while(pTilemap)
	{
		if(pTilemap->IsString(0, "name"))
		{
			name, pTilemap->GetString(1);
		}
		else if(pTilemap->IsString(0, "tilemap"))
		{
			tileMap[0] = MFStr_TruncateExtension(pTilemap->GetString(1));
		}
		else if(pTilemap->IsString(0, "tilemap2"))
		{
			tileMap[1] = MFStr_TruncateExtension(pTilemap->GetString(1));
		}
		else if(pTilemap->IsString(0, "road"))
		{
			roadMat = MFStr_TruncateExtension(pTilemap->GetString(1));
		}
		else if(pTilemap->IsString(0, "water"))
		{
			waterMat = MFStr_TruncateExtension(pTilemap->GetString(1));
		}
		else if(pTilemap->IsString(0, "tile_width"))
		{
			tileWidth = pTilemap->GetInt(1);
		}
		else if(pTilemap->IsString(0, "tile_height"))
		{
			tileHeight = pTilemap->GetInt(1);
		}
		else if(pTilemap->IsSection("Terrain"))
		{
			MFIniLine *pTerrain = pTilemap->Sub();
			while(pTerrain)
			{
				if(!pTerrain->IsString(0, "section"))
					terrainTypes.resize(MFMax(terrainTypes.size(), (size_t)pTerrain->GetInt(0, 16) + 1));

				pTerrain = pTerrain->Next();
			}

			pTerrain = pTilemap->Sub();
			while(pTerrain)
			{
				if(!pTerrain->IsString(0, "section"))
				{
					terrainTypes[pTerrain->GetInt(0, 16)].name = pTerrain->GetString(1);
				}
				else if(pTerrain->IsSection("Transitions"))
				{
					MFIniLine *pTransitions = pTerrain->Sub();

					while(pTransitions)
					{
						MFDebug_Assert(pTransitions->GetStringCount()-1 == terrainTypes.size(), "Invalid transition table dimensions!");
						for(size_t a=0; a<terrainTypes.size(); ++a)
							terrainTransitions[pTransitions->GetInt(0, 16)][a] = pTransitions->GetInt(a + 1);

						pTransitions = pTransitions->Next();
					}
				}
				else if(pTerrain->IsSection("MapColours"))
				{
					MFIniLine *pColours = pTerrain->Sub();

					while(pColours)
					{
						uint32 colour = MFString_AsciiToInteger(pColours->GetString(1), true) | 0xFF000000;
						terrainTypes[pColours->GetInt(0, 16)].mapColour.Set(((colour >> 16) & 0xFF) * (1.f/255.f), ((colour >> 8) & 0xFF) * (1.f/255.f), (colour & 0xFF) * (1.f/255.f));
						pColours = pColours->Next();
					}
				}

				pTerrain = pTerrain->Next();
			}
		}
		else if(pTilemap->IsSection("Tiles1") || pTilemap->IsSection("Tiles2"))
		{
			const char *pPage = pTilemap->GetString(1) + 5;
			int page = MFString_AsciiToInteger(pPage, false, 16) - 1;

			MFIniLine *pTile = pTilemap->Sub();
			while(pTile)
			{
				int x = pTile->GetInt(0);
				int y = pTile->GetInt(1);
				MFDebug_Assert(x < 16 && y < 16, "Tile sheets may have a maximum of 16x16 tiles.");
				MFDebug_Assert(pTile->IsString(2, "="), "Expected '='.");

				int i = (x & 0xF) | ((y & 0xF) << 4);
				Tile &t = tiles[page][i];

				t.x = x;
				t.y = y;
				t.terrain = EncodeTile(pTile->GetInt(3, 16), pTile->GetInt(4, 16), pTile->GetInt(5, 16), pTile->GetInt(6, 16));
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
			MFIniLine *pRoad = pTilemap->Sub();
			while(pRoad)
			{
				int x = pRoad->GetInt(0);
				int y = pRoad->GetInt(1);
				MFDebug_Assert(x < 16 && y < 16, "Tile sheets may have a maximum of 16x16 tiles.");
				MFDebug_Assert(pRoad->IsString(2, "="), "Expected '='.");

				Road &road = roads.push();
				road.x = x;
				road.y = y;
				road.directions = (pRoad->GetInt(3) << 3) | (pRoad->GetInt(4) << 2) | (pRoad->GetInt(5) << 1) | pRoad->GetInt(6);
				road.terrain = EncodeTile(pRoad->GetInt(7, 16), pRoad->GetInt(8, 16), pRoad->GetInt(9, 16), pRoad->GetInt(10, 16));

				pRoad = pRoad->Next();
			}
		}

		pTilemap = pTilemap->Next();
	}
}

Tileset::~Tileset()
{
	if(resourceRefCount > 0)
	{
		resourceRefCount = 1;
		ReleaseResources();
	}
}

void Tileset::DrawMap(int xTiles, int yTiles, uint16 *pTileData, int stride, int lineStride, float texelOffset) const
{
	stride /= sizeof(*pTileData);

	float xScale = (1.f / imageWidth) * tileWidth;
	float yScale = (1.f / imageHeight) * tileHeight;
	float halfX = texelOffset / imageWidth;
	float halfY = texelOffset / imageHeight;
	float waterOffset = texelOffset / 64.f;

	struct RenderTile
	{
		uint8 x, y, tx, ty, flags;
	} renderTiles[2][1024];

	int numTiles[2] = { 0, 0 };
	int numTerrainTiles[2] = { 0, 0 };
	int numWaterTiles = 0;

	// count the tiles to render
	for(int y=0; y<yTiles; ++y)
	{
		for(int x=0; x<xTiles; ++x)
		{
			int tile = pTileData[x*stride] & 0xFF;
			int page = pTileData[x*stride] >> 8;

			RenderTile &rt = renderTiles[page][numTiles[page]];
			++numTiles[page];

			rt.x = x;
			rt.y = y;

			const Tile &t = tiles[page][tile];
			rt.tx = t.x;
			rt.ty = t.y;

			int water = (int)TileContains(t.terrain, 1);
			rt.flags = water;
			numWaterTiles += water;

			if(t.terrain != 0x01010101)
			{
				++numTerrainTiles[page];
				rt.flags |= 2;
			}
		}
		pTileData += lineStride*stride;
	}

	// render any water tiles
	if(numWaterTiles)
	{
		MFMaterial_SetMaterial(pWater);

		MFPrimitive(PT_QuadList);
		MFBegin(numWaterTiles*2);

		for(int p = 0; p < 2; ++p)
		{
			for(int a=0; a<numTiles[p]; ++a)
			{
				RenderTile &rt = renderTiles[p][a];
				if(rt.flags & 1)
				{
					MFSetTexCoord1(0 + waterOffset, 0 + waterOffset);
					MFSetPosition((float)rt.x, (float)rt.y, 0);
					MFSetTexCoord1(1 + waterOffset, 1 + waterOffset);
					MFSetPosition((float)rt.x + 1.f, (float)rt.y + 1.f, 0);
				}
			}
		}

		MFEnd();
	}

	// render any terrain tiles
	for(int p = 0; p < 2; ++p)
	{
		if(numTerrainTiles[p])
		{
			MFMaterial_SetMaterial(pTileMap[p]);

			MFPrimitive(PT_QuadList);
			MFBegin(numTerrainTiles[p]*2);

			for(int a=0; a<numTiles[p]; ++a)
			{
				RenderTile &rt = renderTiles[p][a];
				if(rt.flags & 2)
				{
					MFSetTexCoord1(rt.tx*xScale + halfX, rt.ty*xScale + halfX);
					MFSetPosition((float)rt.x, (float)rt.y, 0);
					MFSetTexCoord1(((float)rt.tx + 1.f)*xScale + halfX, ((float)rt.ty + 1.f)*xScale + halfX);
					MFSetPosition((float)rt.x + 1.f, (float)rt.y + 1.f, 0);
				}
			}

			MFEnd();
		}
	}
}

int Tileset::FindBestTiles(int *pTiles, uint32 tile, uint32 mask, int maxMatches) const
{
	uint32 maskedTile = tile & mask;

	int found = 0;
	int error = 256;
	for(int p=0; p<2; ++p)
	{
		for(int a=0; a<256; ++a)
		{
			if((tiles[p][a].terrain & mask) == maskedTile)
			{
				int e = GetTileError(tiles[p][a].terrain, tile);
				if(e < error)
				{
					error = e;
					pTiles[0] = a | (p << 8);
					found = 1;
				}
				else if(e == error && found < maxMatches)
					pTiles[found++] = a | (p << 8);
			}
		}
	}

	return found;
}

int Tileset::FindBestRoads(int *pRoad, uint32 directions, uint32 terrain) const
{
	int numRoads = 0;
	int error = 4;

	for(size_t a=0; a<roads.size(); ++a)
	{
		if(roads[a].terrain == terrain)
		{
			uint32 d = roads[a].directions ^ directions;
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

MFRect Tileset::GetTileUVs(int tile, float texelOffset) const
{
	float fWidth = (float)imageWidth;
	float fHeight = (float)imageHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Tile &t = tiles[tile >> 8][tile & 0xFF];
	MFRect uvs;
	uvs.x = t.x*xScale + halfX;
	uvs.y = t.y*yScale + halfY;
	uvs.width = xScale;
	uvs.height = yScale;
	return uvs;
}

MFRect Tileset::GetRoadUVs(int index, float texelOffset) const
{
	float fWidth = (float)roadWidth;
	float fHeight = (float)roadHeight;
	float xScale = (1.f / fWidth) * tileWidth;
	float yScale = (1.f / fHeight) * tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	const Road &r = roads[index];
	MFRect uvs;
	uvs.x = r.x*xScale + halfX;
	uvs.y = r.y*yScale + halfY;
	uvs.width = xScale;
	uvs.height = yScale;
	return uvs;
}

MFRect Tileset::GetWaterUVs(float texelOffset) const
{
	float fWidth = (float)tileWidth;
	float fHeight = (float)tileHeight;
	float halfX = texelOffset / fWidth;
	float halfY = texelOffset / fHeight;

	MFRect uvs;
	uvs.x = halfX;
	uvs.y = halfY;
	uvs.width = 1.f;
	uvs.height = 1.f;
	return uvs;
}

int Tileset::FindRoad(uint32 directions, uint32 terrain) const
{
	for(size_t a=0; a<roads.size(); ++a)
	{
		if(roads[a].terrain == terrain && roads[a].directions == directions)
			return a;
	}

	return -1;
}
