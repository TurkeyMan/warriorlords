#pragma once
#if !defined(_MAP_H)
#define _MAP_H

#include "MapTemplate.h"
#include "Unit.h"
#include "Castle.h"
#include "Path.h"

class Castle;

enum ObjectType
{
	OT_None = 0,

	OT_Terrain,
	OT_Castle,
	OT_Flag,
	OT_Place,
	OT_Road,
	OT_Region,

	OT_Max
};

class MapTile
{
	friend class Map;
	friend class EditableMap;
	friend class MapView;
public:
	static const int MaxUnitsOnTile = 20;

	::Map &Map() const					{ return *pMap; }

	int GetX() const					{ return x; }
	int GetY() const					{ return y; }

	int GetTile() const					{ return terrain; }
	uint32 GetTerrain() const;
	ObjectType GetType() const			{ return (ObjectType)type; }
	uint32 GetRoadDirections() const	{ return type == OT_Road ? index : 0; }
	int GetRegion() const				{ return region; }

	void AddGroup(Group *pGroup);
	void AddGroupToBack(Group *pGroup);
	void BringGroupToFront(Group *pGroup);
	void RemoveGroup(Group *pGroup);

	int GetNumGroups() const;
	Group *GetGroup(int group)			{ return (Group*)((const MapTile*)this)->GetGroup(group); }
	const Group *GetGroup(int group) const;

	int GetNumUnits() const;
	Unit *GetUnit(int unit)				{ return (Unit*)((const MapTile*)this)->GetUnit(unit); }
	const Unit *GetUnit(int unit) const;
	int GetAvailableUnitSpace() const	{ return MaxUnitsOnTile - GetNumUnits(); }

	Group *FindUnitGroup(Unit *pUnit);
	Unit *FindVehicle();

	Castle *GetCastle();
	const Castle *GetCastle() const;
	Place *GetPlace();
	const Place *GetPlace() const;

	bool IsFriendlyTile(int player) const;
	bool IsEnemyTile(int player) const;
	bool CanMove(Group *pGroup) const;
	bool IsRoad() const;

	int GetPlayer() const;

protected:
	::Map *pMap;
	Group *pGroup;

	uint16 x, y;
	uint16 terrain;			// terrain tile
	uint8 region;			// region

	uint8 type;				// type of detail on tile
	uint8 index;			// index of item on tile

	uint8 objectX, objectY;	// object square relative to its top left point
	uint8 flags;			// tile flags
};

class Map
{
	friend class MapView;
public:
	Map(MapTemplate &mapTemplate, ::GameState &gameState);
	virtual ~Map();

	void LoadResources();
	void ReleaseResources();

	::GameState& GameState()							{ return gameState; }

	const MapTemplate& Template() const					{ return mapTemplate; }
	MFString FileName() const							{ return mapTemplate.FileName(); }
	MFString Name() const								{ return mapTemplate.Name(); }

	int NumPlayersPresent() const						{ return mapTemplate.NumPlayersPresent(); }
	bool IsPlayerPresent(int region) const				{ return mapTemplate.IsPlayerPresent(region); }
	bool IsRacePresent(int race) const					{ return mapTemplate.IsRacePresent(race); }

	void GetMapSize(int *pWidth, int *pHeight) const	{ mapTemplate.GetMapSize(pWidth, pHeight); }

	void ConstructMap(int race = -1);

	void ClaimFlags(int x, int y, int player);

	MapTile *GetTile(int x, int y) const				{ return pMap + y*mapTemplate.mapWidth + x; }
	const Tile *GetTerrainTile(int terrainTile) const	{ return Tileset().GetTile(terrainTile); }
	const Tile *GetTerrainTileAt(int x, int y) const	{ return Tileset().GetTile(pMap[y*mapTemplate.mapWidth + x].terrain); }
	uint32 GetTerrain(int terrainTile) const			{ return Tileset().GetTile(terrainTile)->terrain; }
	uint32 GetTerrainAt(int x, int y) const				{ return Tileset().GetTile(pMap[y*mapTemplate.mapWidth + x].terrain)->terrain; }

	int GetNumCastles() const							{ return castles.size(); }
	Castle *GetCastle(int id)							{ return &castles[id]; }

	int GetNumPlaces() const							{ return places.size(); }
	Place *GetPlace(int id)								{ return &places[id]; }

	ObjectType GetDetailType(int x, int y) const;
	int GetDetail(int x, int y) const;

	const Tileset& Tileset() const						{ return *mapTemplate.pTiles; }
	const UnitDefinitions& UnitDefs() const				{ return *mapTemplate.pUnits; }

	Path *FindPath(Group *pGroup, int destX, int destY);

	static int GetMovementPenalty(const MapTile *pTile, int *pTerrainPenalties, int player, bool bRoadWalk, int *pTerrainType = NULL);

protected:
	Map();

	MapTemplate &mapTemplate;
	const int width, height;

	::GameState &gameState;

	MapTile *pMap;
	MFArray<Castle> castles;
	MFArray<Place> places;

	Path path;
};

#endif
