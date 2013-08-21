#pragma once
#if !defined(_CASTLE_H)
#define _CASTLE_H

#include "MapTemplate.h"

class MapTile;

class Castle
{
public:
	void Init(int id, const CastleDetails &details, int player);

	MapTile *GetTile(int index = 0) const;
	bool IsEmpty() const;

	Group *GetMercGroup();
	void Capture(Group *pGroup);

	void ClearBuildUnit();
	void SetBuildUnit(int slot);
	int GetBuildUnit() const;
	int GetUnitBuildTime() const;
	int BuildTimeRemaining() const;

	MFString GetName() const	{ return details.name; }
	void SetName(MFString name)	{ details.name = name; }

	int GetPlayer() const		{ return player; }

	const UnitDefinitions& UnitDefs() const;

//protected:
	MapTile *pTile;
//	Group *pMercGroup;

	CastleDetails details;

	int id;
	int player;

	int building;   // the current unit being built
	int buildTime;  // how many turns this unit has been in production
	int nextBuild;
};

#endif
