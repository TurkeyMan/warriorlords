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
	uint8 speed;
	uint8 canBuild : 1;
	uint8 canWalk  : 1;
	uint8 bias     : 6;
};

class Tileset
{
public:
	static Tileset *Create(const char *pFilename);
	void Destroy();

	void DrawMap(int xTiles, int yTiles, uint8 *pTileData, int stride, int lineStride, float texelOffset = 0.5f);

	int FindBestTiles(int *pTiles, uint32 tile, uint32 mask = 0xFFFFFFFF, int maxMatches = 8);

	inline void GetTileSize(int *pWidth, int *pHeight) { *pWidth = tileWidth; *pHeight = tileHeight; }
	inline Tile *GetTile(int tile) { return &tiles[tile]; }
	int GetTileSpeed(uint32 terrain);

	inline int GetNumTerrainTypes() { return terrainCount; }
	inline const char *GetTerrainName(int type) { return pTerrainTypes[type].name; }

	inline MFMaterial *GetMaterial() { return pTileMap; }
	void GetTileUVs(int tile, MFRect *pUVs);

	const MFVector &GetTerrainColour(int terrain) { return pTerrainTypes[terrain].mapColour; }

protected:
	struct TerrainType
	{
		char name[28];
		int speed;
		MFVector mapColour;
	};

	char name[64];

	int tileWidth, tileHeight;
	int imageWidth, imageHeight;

	MFMaterial *pTileMap;

	TerrainType *pTerrainTypes;
	int terrainCount;

	Tile tiles[256];

	// editor stuff
	uint8 terrainTransitions[16][16];

	// private functions
	inline int GetTileError(uint32 t1, uint32 t2)
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

#endif
