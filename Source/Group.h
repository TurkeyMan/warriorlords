#pragma once
#if !defined(_GROUP_H)
#define _GROUP_H

struct PendingAction;
class MapTile;
class Path;

class Group
{
public:
	static Group *Create(int player);
	void Destroy();

	uint32 GetID() const						{ return id; }
	void SetID(uint32 _id)						{ id = _id; }

	bool AddUnit(Unit *pUnit);
	bool AddForwardUnit(Unit *pUnit);
	bool AddRearUnit(Unit *pUnit);
	void RemoveUnit(Unit *pUnit);

	int GetMovement() const;
	float MoveRemaining() const					{ return MFMax((float)GetMovement()*0.5f, 0.f); }
	bool SubtractMovementCost(MapTile *pTile);

	bool IsSelected() const						{ return bSelected; }
	bool IsInGroup(const Unit *pUnit) const;
	Unit *GetHero();

	int GetPlayer() const						{ return player; }
	void SetPlayer(int player);

	MapTile *GetTile() const					{ return pTile; }

	int GetNumUnits() const						{ return numForwardUnits + numRearUnits; }
	int GetNumForwardUnits() const				{ return numForwardUnits; }
	int GetNumRearUnits() const					{ return numRearUnits; }

	Unit *GetUnit(int unit) const				{ return (unit >= numForwardUnits) ? pRearUnits[unit - numForwardUnits] : pForwardUnits[unit]; }
	Unit *GetForwardUnit(int forwardUnit) const	{ return pForwardUnits[forwardUnit]; }
	Unit *GetRearUnit(int rearUnit) const		{ return pRearUnits[rearUnit]; }

	Unit *GetFeatureUnit() const				{ return GetUnit(0); }
	Unit *GetVehicle() const					{ return pVehicle; }

	void FindPath(int x, int y);
	Path *GetPath()								{ return pPath; }

	PendingAction *GetLastAction()				{ return pLastAction; }

	bool ValidateGroup() const;

//protected:
	void UpdateGroupStats();

	int id;

	int player;
	MapTile *pTile;
	int x, y;

	union
	{
		Unit *pUnits[11];
		struct
		{
			Unit *pForwardUnits[5];
			Unit *pRearUnits[5];
			Unit *pVehicle;
		};
	};

	int numForwardUnits;
	int numRearUnits;

	bool bSelected;
	int pathX, pathY;
	Path *pPath;

	PendingAction *pLastAction;

	Group *pNext;
};

#endif
