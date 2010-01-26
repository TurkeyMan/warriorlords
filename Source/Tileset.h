#if !defined(_TILESET_H)
#define _TILESET_H

struct MFMaterial;

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

class Tileset
{
public:
	static Tileset *Create(const char *pFilename);
	void Destroy();

	inline int GetNumTerrainTypes() const { return terrainCount; }
	inline const char *GetTerrainName(int type) const { return pTerrainTypes[type].name; }
	const MFVector &GetTerrainColour(int terrain) const { return pTerrainTypes[terrain].mapColour; }

	int FindBestTiles(int *pTiles, uint32 tile, uint32 mask = 0xFFFFFFFF, int maxMatches = 8) const;
	int FindBestRoads(int *pRoads, uint32 directions, uint32 terrain) const;
	int FindRoad(uint32 directions, uint32 terrain) const;
	uint32 GetRoadConnections(int road) const { return pRoads[road].directions; }

	inline void GetTileSize(int *pWidth, int *pHeight) const { *pWidth = tileWidth; *pHeight = tileHeight; }
	inline const Tile *GetTile(int tile) const { return &tiles[tile]; }

	void DrawMap(int xTiles, int yTiles, uint8 *pTileData, int stride, int lineStride, float texelOffset);

	void GetTileUVs(int tile, MFRect *pUVs, float texelOffset);
	void GetRoadUVs(int index, MFRect *pUVs, float texelOffset);
	void GetWaterUVs(MFRect *pUVs, float texelOffset);

	inline MFMaterial *GetTileMaterial() const { return pTileMap; }
	inline MFMaterial *GetWaterMaterial() const { return pWater; }
	inline MFMaterial *GetRoadMaterial() const { return pRoadMap; }

protected:
	struct TerrainType
	{
		char name[28];
		MFVector mapColour;
	};

	struct Road
	{
		uint8 x, y;
		uint8 directions;
		uint8 reserved;
		uint32 terrain;
	};

	char name[64];

	int tileWidth, tileHeight;
	int imageWidth, imageHeight;
	int roadWidth, roadHeight;

	MFMaterial *pTileMap;
	MFMaterial *pWater;
	MFMaterial *pRoadMap;

	TerrainType *pTerrainTypes;
	int terrainCount;

	Road *pRoads;
	int roadCount;

	Tile tiles[256];

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
