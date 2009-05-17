#if !defined(_MAP_H)
#define _MAP_H

#include "Tileset.h"
#include "InputHandler.h"

struct MFTexture;

class Map : public InputHandler
{
public:
	static Map *Create(const char *pMapFilename);
	static Map *CreateNew(const char *pTileset);
	void Destroy();

	virtual int UpdateInput();

	void Update();
	void Draw();

	void DrawDebug();

	void GetCursor(int *pX, int *pY);
	void GetVisibleTileSize(float *pWidth, float *pHeight);

	void SetOffset(float x, float y);
	void CenterView(int x, int y);
	void SetZoom(float zoom, float centerX = -1.f, float centerY = -1.f);

	void SetMapOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	inline Tile *GetTile(int x, int y) { return pTiles->GetTile(pMap[y*mapWidth + x]); }
	inline uint32 GetTerrain(int x, int y) { return pTiles->GetTile(pMap[y*mapWidth + x])->terrain; }
	bool SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask = 0xFFFFFFFF);

	Tileset *GetTileset() { return pTiles; }

	int Map::UpdateChange(int a);

protected:
	char name[32];

	int mapWidth;
	int mapHeight;

	uint8 *pMap;

	Tileset *pTiles;

	float xOffset, yOffset;
	float zoom;

	bool bIsDragging;

	MFTexture *pRenderTarget;
	MFTexture *pMinimap;

	// editor stuff
	struct MapCoord
	{
		uint16 x, y;
	};

	uint8 *pTouched;
	MapCoord *pChangeList;
	int numChanges;

	bool Map::SetTile(int x, int y, uint32 tile, uint32 mask);
};

#endif
