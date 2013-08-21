#pragma once
#if !defined(_MAPTEMPLATE_H)
#define _MAPTEMPLATE_H

#include "Fuji/MFResource.h"

#include "Tileset.h"
//#include "Unit.h"

class UnitDefinitions;

struct BuildUnit
{
	int unit;
	int buildTime;
};

struct CastleDetails
{
	MFString name;
	int x, y;
	bool bCapital;

	BuildUnit buildUnits[4];
	int numBuildUnits;
};

class MapTemplate : public MFResource
{
	friend class Map;
	friend class EditableMap;
public:
	static void Init();

	static MapTemplate *Create(MFString mapFilename);

	int AddRef();
	int Release();

	MFString FileName() const							{ return filename; }
	MFString Name() const								{ return name; }

	int NumPlayersPresent() const						{ return numPlayers; }
	bool IsPlayerPresent(int region) const				{ return bRegionsPresent[region]; }
	bool IsRacePresent(int race) const					{ return templates[race].pMap != 0; }

	void GetMapSize(int *pWidth, int *pHeight) const	{ if(pWidth) *pWidth = mapWidth; if(pHeight) *pHeight = mapHeight; }

	const Tileset& Tileset() const						{ return *pTiles; }
	const UnitDefinitions& UnitDefs() const				{ return *pUnits; }

	// editor
	MapTemplate(MFString tileset, MFString unitset, int width, int height);
	void Save();

protected:
	MapTemplate(MFString mapFilename);
	~MapTemplate();

	static void Destroy(MFResource *pRes);

	MFString filename;
	MFString name;
	MFString tileset;
	MFString unitset;

	::Tileset *pTiles;
	UnitDefinitions *pUnits;

	int mapWidth;
	int mapHeight;

	int numPlayers;

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

		MFArray<CastleDetails> castles;
	};

	uint8 *pRegions;
	bool bRegionsPresent[16];
	MapRegionTemplate templates[16];
};

#endif
