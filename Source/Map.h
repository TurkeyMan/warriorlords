#if !defined(_MAP_H)
#define _MAP_H

#include "Tileset.h"
#include "Unit.h"
#include "InputHandler.h"
#include "Path.h"

#include "UI/HKWidget.h"

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

struct MapDetails
{
	char filename[64];
	char mapName[64];
	char tileSet[64];
	char unitSet[64];

	//TileSetDetails tileSetDetails;
	UnitSetDetails unitSetDetails;

	int width, height;

	int numPlayers;
	bool bPlayerPresent[16];

	bool bRacePresent[16];
};

class MapTile
{
	friend class Map;
public:
	static const int MaxUnitsOnTile = 20;

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
	Unit *GetUnit(int unit);
	int GetAvailableUnitSpace() { return MaxUnitsOnTile - GetNumUnits(); }

	Group *FindUnitGroup(Unit *pUnit);
	Unit *FindVehicle();

	Castle *GetCastle() { if(type == OT_Castle) return (Castle*)pObject; return NULL; }
	Ruin *GetRuin() { if(type == OT_Special) return (Ruin*)pObject; return NULL; }

	bool IsFriendlyTile(int player);
	bool IsEnemyTile(int player);
	bool CanMove(Group *pGroup);
	bool IsRoad();

	int GetPlayer();

protected:
	Group *pGroup;
	void *pObject;

	uint16 x, y;
	uint16 terrain;			// terrain tile

	uint8 type;				// type of detail on tile
	uint8 index;			// index of item on tile
	uint8 castleTile : 2;	// castle square: 0 = top left, 1 = top right, 2 = bottom left, 3 = bottom right
	uint8 region : 4;		// region
	uint8 flags : 2;		// tile flags
};

class Map : public InputReceiver
{
public:
	static bool GetMapDetails(const char *pMapFilename, MapDetails *pDetails);

	static Map *Create(Game *pGame, const char *pMapFilename, bool bEditable = false);
	static Map *CreateNew(Game *pGame, const char *pTileset, const char *pUnits);
	void Destroy();

	void Save();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);

	bool ReceiveInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);

	void Update();
	void Draw();

	void DrawDebug();

	void GetMapSize(int *pWidth, int *pHeight) { if(pWidth) *pWidth = mapWidth; if(pHeight) *pHeight = mapHeight; }
	void GetCursor(float x, float y, float *pX, float *pY);
	void GetVisibleTileSize(float *pWidth, float *pHeight);

	void SetOffset(float x, float y);
	void GetOffset(float *pX, float *pY);
	void SetZoom(float zoom, float centerX = -1.f, float centerY = -1.f);
	void CenterView(float x, float y);

	void ClaimFlags(int x, int y, int player);

	void SetMapOrtho(int *pXTiles = NULL, int *pYTiles = NULL);

	MapTile *GetTile(int x, int y) const { return pMap + y*mapWidth + x; }
	const Tile *GetTerrainTile(int terrainTile) const { return pTiles->GetTile(terrainTile); }
	const Tile *GetTerrainTileAt(int x, int y) const { return pTiles->GetTile(pMap[y*mapWidth + x].terrain); }
	uint32 GetTerrain(int terrainTile) const { return pTiles->GetTile(terrainTile)->terrain; }
	uint32 GetTerrainAt(int x, int y) const { return pTiles->GetTile(pMap[y*mapWidth + x].terrain)->terrain; }

	int GetNumCastles() const { return numCastles; }
	Castle *GetCastle(int id) { return &pCastles[id]; }

	int GetNumRuins() const { return numRuins; }
	Ruin *GetRuin(int id) { return &pRuins[id]; }

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

	Path *FindPath(Group *pGroup, int destX, int destY);

	void ConstructMap(int race = -1);

	MFMaterial *GetMinimap(int *pMapWidth, int *pMapHeight);

	CastleDetails *GetCastleTemplate(int x, int y);

	static int GetMovementPenalty(MapTile *pTile, int *pTerrainPenalties, int player, bool bRoadWalk, int *pTerrainType = NULL);

protected:
	void CreateRenderTarget();

	char filename[32];
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
	float zoom, maxZoom;

	int dragContact;
	float lastX, lastY;
	float xVelocity, yVelocity;
	bool isDragging;

	MFTexture *pRenderTarget;
	MFMaterial *pRenderTargetMaterial;
	MFMaterial *pCloud;

	MFMaterial *pMinimapMaterial;
	int minimapPixelScale;
	uint32 *pMiniMapImage;

	MapTile *pMap;
	Castle pCastles[256];
	int numCastles;
	Ruin pRuins[256];
	int numRuins;

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
			uint16 terrain;
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
