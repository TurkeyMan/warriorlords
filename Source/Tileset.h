#if !defined(_TILESET_H)
#define _TILESET_H

#include "Fuji/MFResource.h"

struct MFMaterial;
class MFIniLine;

struct TileSize
{
	int width, height;
};

struct Tile
{
	enum Type
	{
		Regular = 0,
		Road = 1,
		Special = 2,
	};

	uint32 terrain;
	uint8 x, y;
	uint8 bias;
	uint8 canBuild : 1;
	uint8 flags    : 7;
};

class Tileset : public MFResource
{
public:
	static void Init();

	static Tileset *Create(MFString filename);

	int AddRef();
	int Release();

	void LoadResources();
	void ReleaseResources();

	inline int NumTerrainTypes() const							{ return terrainTypes.size(); }
	inline MFString TerrainName(int type) const					{ return terrainTypes[type].name; }
	inline const MFVector& TerrainColour(int terrain) const		{ return terrainTypes[terrain].mapColour; }

	int FindBestTiles(int *pTiles, uint32 tile, uint32 mask = 0xFFFFFFFF, int maxMatches = 8) const;
	int FindBestRoads(int *pRoads, uint32 directions, uint32 terrain) const;
	int FindRoad(uint32 directions, uint32 terrain) const;
	uint32 GetRoadConnections(int road) const					{ return roads[road].directions; }

	inline TileSize GetTileSize() const							{ TileSize ts; ts.width = tileWidth; ts.height = tileHeight; return ts; }
	inline const Tile *GetTile(int tile) const					{ return &tiles[tile >> 8][tile & 0xFF]; }

	void DrawMap(int xTiles, int yTiles, uint16 *pTileData, int stride, int lineStride, float texelOffset) const;

	MFRect GetTileUVs(int tile, float texelOffset) const;
	MFRect GetRoadUVs(int index, float texelOffset) const;
	MFRect GetWaterUVs(float texelOffset) const;

	inline MFMaterial* GetTileMaterial(int page = 0) const		{ return pTileMap[page]; }
	inline MFMaterial* GetWaterMaterial() const					{ return pWater; }
	inline MFMaterial* GetRoadMaterial() const					{ return pRoadMap; }

protected:
	Tileset(MFIniLine *pLine);
	~Tileset();

	static void Destroy(MFResource *pRes);

	struct TerrainType
	{
		MFString name;
		MFVector mapColour;
	};

	struct Road
	{
		uint8 x, y;
		uint8 directions;
		uint8 reserved;
		uint32 terrain;
	};

	MFString name;

	MFArray<TerrainType> terrainTypes;
	MFArray<Road> roads;

	Tile tiles[2][256];

	int tileWidth, tileHeight;
	int imageWidth, imageHeight;
	int roadWidth, roadHeight;

	MFString tileMap[2];
	MFString waterMat;
	MFString roadMat;

	MFMaterial *pTileMap[2];
	MFMaterial *pWater;
	MFMaterial *pRoadMap;

	int resourceRefCount;

	// editor stuff
	uint8 terrainTransitions[16][16];

	// private functions
	inline int GetTileError(uint32 t1, uint32 t2) const
	{
		int error = 0;
		error += terrainTransitions[t1&0xFF][t2&0xFF];
		error += terrainTransitions[(t1>>8)&0xFF][(t2>>8)&0xFF];
		error += terrainTransitions[(t1>>16)&0xFF][(t2>>16)&0xFF];
		error += terrainTransitions[(t1>>24)&0xFF][(t2>>24)&0xFF];
		return error;
	}
};

// helpers for encoding and decoding terrain
union EncodedTerrain
{
	uint32 e;
	struct { uint8 tl, tr, bl, br; };
};

inline uint32 EncodeTile(int tl, int tr, int bl, int br)
{
	EncodedTerrain t;
	t.tl = tl;
	t.tr = tr;
	t.bl = bl;
	t.br = br;
	return t.e;
}

inline void DecodeTile(uint32 tile, int *tl, int *tr, int *bl, int *br)
{
	EncodedTerrain t = { tile };
	*tl = t.tl;
	*tr = t.tr;
	*bl = t.bl;
	*br = t.br;
}

inline int DecodeTL(uint32 tile)
{
	EncodedTerrain t = { tile };
	return t.tl;
}

inline int DecodeTR(uint32 tile)
{
	EncodedTerrain t = { tile };
	return t.tr;
}

inline int DecodeBL(uint32 tile)
{
	EncodedTerrain t = { tile };
	return t.bl;
}

inline int DecodeBR(uint32 tile)
{
	EncodedTerrain t = { tile };
	return t.br;
}

inline bool TileContains(uint32 tile, int terrain)
{
	tile ^= 0x01010101 * terrain;
	uint32 hasZeroByte = (tile - 0x01010101) & ~tile & 0x80808080;
	return !!hasZeroByte;
}

#endif
