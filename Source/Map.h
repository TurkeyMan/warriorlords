#if !defined(_MAP_H)
#define _MAP_H

#include "Tileset.h"
#include "Castle.h"
#include "Unit.h"
#include "InputHandler.h"
#include "Path.h"

struct MFTexture;
class Game;

enum ObjectType
{
	OT_None = 0,

	OT_Terrain,
	OT_Castle,
	OT_Flag,
	OT_Special,
	OT_Road,
	OT_Region,

	OT_Max
};

class MapTile
{
	friend class Map;
public:
	int GetX() { return x; }
	int GetY() { return y; }

	int GetTile() const { return terrain; }
	uint32 GetTerrain() const;
	ObjectType GetType() const { return (ObjectType)type; }
	uint32 GetRoadDirections() const { return type == OT_Road ? index : 0; }
	int GetRegion() const { return region; }
	int GetRegionRace() const;

	void AddGroup(Group *pGroup);
	void AddGroupToBack(Group *pGroup);
	void BringGroupToFront(Group *pGroup);
	void RemoveGroup(Group *pGroup);

	int GetNumGroups();
	Group *GetGroup(int group);

	int GetNumUnits();
	int GetAvailableUnitSpace() { return 10 - GetNumUnits(); }

	Group *FindUnitGroup(Unit *pUnit);
	Unit *FindVehicle();

	Castle *GetCastle() { if(type == OT_Castle) return (Castle*)pObject; return NULL; }

	bool IsFriendlyTile(int player);
	bool IsEnemyTile(int player);
	bool CanMove(Group *pGroup);

protected:
	Group *pGroup;
	void *pObject;

	uint16 x, y;

	uint8 terrain;			// terrain tile
	uint8 type;				// type of detail on tile
	uint8 index;			// index of item on tile
	uint8 castleTile : 2;	// castle square: 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right
	uint8 region : 4;		// region
	uint8 flags : 2;		// tile flags
};

class Map : public InputReceiver
{
public:
	static Map *Create(Game *pGame, const char *pMapFilename, bool bEditable = false);
	static Map *CreateNew(Game *pGame, const char *pTileset, const char *pUnits);
	void Destroy();

	void Save(const char *pFilename);

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	void Update();
	void Draw();

	void DrawDebug();

	void GetMapSize(int *pWidth, int *pHeight) { if(pWidth) *pWidth = mapWidth; if(pHeight) *pHeight = mapHeight; }
	void GetCursor(float x, float y, int *pX, int *pY);
	void GetVisibleTileSize(float *pWidth, float *pHeight);

	void SetOffset(float x, float y);
	void GetOffset(float *pX, float *pY);
	void SetZoom(float zoom, float centerX = -1.f, float centerY = -1.f);
	void CenterView(int x, int y);

	void ClaimFlags(int x, int y, int player);

	void SetMapOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	MapTile *GetTile(int x, int y) const { return pMap + y*mapWidth + x; }
	const Tile *GetTerrainTile(int terrainTile) const { return pTiles->GetTile(terrainTile); }
	const Tile *GetTerrainTileAt(int x, int y) const { return pTiles->GetTile(pMap[y*mapWidth + x].terrain); }
	uint32 GetTerrain(int terrainTile) const { return pTiles->GetTile(terrainTile)->terrain; }
	uint32 GetTerrainAt(int x, int y) const { return pTiles->GetTile(pMap[y*mapWidth + x].terrain)->terrain; }

	int GetNumCastles() const { return numCastles; }
	Castle *GetCastle(int id) { return &pCastles[id]; }

	ObjectType GetDetailType(int x, int y) const;
	int GetDetail(int x, int y) const;

	Tileset *GetTileset() { return pTiles; }
	UnitDefinitions *GetUnitDefinitions() { return pUnits; }

	bool SetTerrain(int x, int y, int tl, int tr, int bl, int br, uint32 mask = 0xFFFFFFFF);
	void SetRegion(int x, int y, int region);

	bool PlaceCastle(int x, int y, int player);
	bool PlaceFlag(int x, int y, int race = 0);
	bool PlaceSpecial(int x, int y, int index);
	bool PlaceRoad(int x, int y);

	void ClearDetail(int x, int y);

	void SetMoveKey(bool bAlternate) { moveButton = bAlternate ? 1 : 0; }
	void SetEditRegion(int region) { editRegion = region; }
	void SetEditRace(int race) { editRace = race; }

	int UpdateChange(int a);

	Step *FindPath(Group *pGroup, int destX, int destY);
	Step *StripStep(Step *pPath);
	void DestroyPath(Step *pPath);

	void ConstructMap(int race = -1);

	CastleDetails *GetCastleTemplate(int x, int y);

protected:
	char name[32];
	char tileset[32];
	char unitset[32];

	Game *pGame;

	Tileset *pTiles;
	UnitDefinitions *pUnits;

	int mapWidth;
	int mapHeight;

	// runtime data
	float xOffset, yOffset;
	float zoom;

	MFTexture *pRenderTarget;
	MFMaterial *pRenderTargetMaterial;
	MFTexture *pMinimap;
	MFMaterial *pMinimapMaterial;
	MFMaterial *pCloud;

	MapTile *pMap;
	Castle pCastles[256];
	int numCastles;

	Path path;

	// details
	struct Cloud
	{
		float x, y;
	};

	static const int numClouds = 16;

	Cloud clouds[numClouds];

	// template
	struct MapRegionTemplate
	{
		struct TileDetails
		{
			uint8 terrain;
			uint8 type;
			uint8 index;
		};

		TileDetails *pMap;

		CastleDetails *pCastles;
		int numCastles;
	};

	uint8 *pMapRegions;
	MapRegionTemplate mapTemplate[16];

	// editor stuff
	struct MapCoord
	{
		uint16 x, y;
	};

	struct RevertTile
	{
		uint16 x, y;
		uint32 t;
	};

	uint8 *pTouched;
	MapCoord *pChangeList;
	int numChanges;
	RevertTile *pRevertList;
	int numReverts;

	bool bEditable;
	bool bRevert;

	int moveButton;

	int editRegion;
	int editRace;

	bool SetTile(int x, int y, uint32 tile, uint32 mask);
	int ChooseTile(int *pSelectedTiles, int numVariants);
	void SetRTOrtho(int *pXTiles = NULL, int *pYTiles = NULL);
};

#endif
