#if !defined(_MAP_H)
#define _MAP_H

#include "Tileset.h"
#include "Castle.h"
#include "Unit.h"
#include "InputHandler.h"
#include "Path.h"

struct MFTexture;

enum ObjectType
{
	OT_None = 0,

	OT_Terrain,
	OT_Castle,
	OT_Flag,
	OT_Special,
	OT_Road,

	OT_Max
};

class Map : public InputHandler
{
public:
	static Map *Create(const char *pMapFilename);
	static Map *CreateNew(const char *pTileset, const char *pUnits);
	void Destroy();

	void Save(const char *pFilename);

	virtual int UpdateInput();

	void Update();
	void Draw();

	void DrawDebug();

	void GetMapSize(int *pWidth, int *pHeight) { *pWidth = mapWidth; *pHeight = mapHeight; }
	void GetCursor(int *pX, int *pY);
	void GetVisibleTileSize(float *pWidth, float *pHeight);

	void SetOffset(float x, float y);
	void CenterView(int x, int y);
	void SetZoom(float zoom, float centerX = -1.f, float centerY = -1.f);

	void SetMapOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	inline Tile *GetTile(int x, int y) { return pTiles->GetTile(pMap[y*mapWidth + x].terrain); }
	inline uint32 GetTerrain(int x, int y) { return pTiles->GetTile(pMap[y*mapWidth + x].terrain)->terrain; }

	ObjectType GetDetailType(int x, int y);
	int GetDetail(int x, int y);

	bool SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask = 0xFFFFFFFF);

	bool PlaceCastle(int x, int y, int race);
	bool PlaceFlag(int x, int y, int race = 0);
	bool PlaceSpecial(int x, int y, int index);
	bool PlaceRoad(int x, int y);

	void ClearDetail(int x, int y);

	Tileset *GetTileset() { return pTiles; }
  UnitDefinitions *GetUnitDefinitions() { return pUnits; }

	void SetMoveKey(bool bAlternate) { bRightMove = bAlternate; }

	int UpdateChange(int a);

	// TODO: MOVE ME!!!
	Path path;

protected:
	struct MapTile
	{
		Unit *pUnits;
		uint8 terrain;
		uint8 type;
		uint8 index;
		uint8 reserved;
	};

	char name[32];
	char tileset[32];
	char units[32];

	Tileset *pTiles;
  UnitDefinitions *pUnits;

	int mapWidth;
	int mapHeight;

	MapTile *pMap;

	// runtime data
	float xOffset, yOffset;
	float zoom;

	bool bIsDragging;

	MFTexture *pRenderTarget;
	MFMaterial *pRenderTargetMaterial;
	MFTexture *pMinimap;
	MFMaterial *pMinimapMaterial;

	// editor stuff
	struct MapCoord
	{
		uint16 x, y;
	};

	uint8 *pTouched;
	MapCoord *pChangeList;
	int numChanges;

	bool bRightMove;

	bool SetTile(int x, int y, uint32 tile, uint32 mask);
	int ChooseTile(int *pSelectedTiles, int numVariants);
};

#endif
