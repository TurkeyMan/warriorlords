#include "Warlords.h"
#include "Group.h"

Group *Group::Create(int _player)
{
	Group *pGroup = new Group;
	pGroup->player = _player;
	pGroup->numForwardUnits = pGroup->numRearUnits = 0;
	pGroup->bSelected = false;
	pGroup->pVehicle = NULL;
	pGroup->pPath = NULL;
	pGroup->x = pGroup->y = -1;
	pGroup->pathX = pGroup->pathY = -1;
	pGroup->pLastAction = NULL;
	pGroup->pNext = NULL;
	return pGroup;
}

void Group::Destroy()
{
	delete this;
}

bool Group::AddUnit(Unit *pUnit)
{
	if(pUnit->IsVehicle())
	{
		if(pVehicle)
			return false;

		pVehicle = pUnit;
		pUnit->pGroup = this;
		UpdateGroupStats(); // it could be a galleon
		return true;
	}

	if(numForwardUnits + numRearUnits >= 10)
		return false;

	bool bRear = pUnit->IsRanged();

	if(bRear)
	{
		if(numRearUnits < 5)
			pRearUnits[numRearUnits++] = pUnit;
		else
			pForwardUnits[numForwardUnits++] = pUnit;
	}
	else
	{
		if(numForwardUnits < 5)
			pForwardUnits[numForwardUnits++] = pUnit;
		else
			pRearUnits[numRearUnits++] = pUnit;
	}

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

bool Group::AddForwardUnit(Unit *pUnit)
{
	if(numForwardUnits >= 5)
		return false;

	pForwardUnits[numForwardUnits++] = pUnit;

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

bool Group::AddRearUnit(Unit *pUnit)
{
	if(numRearUnits >= 5)
		return false;

	pRearUnits[numRearUnits++] = pUnit;

	pUnit->pGroup = this;
	if(pUnit->IsHero())
		UpdateGroupStats();
	else
		pUnit->UpdateStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
	return true;
}

void Group::RemoveUnit(Unit *pUnit)
{
	if(pUnit->pGroup == this)
	{
		pUnit->pGroup = NULL;
		pUnit->UpdateStats();
	}

	if(pUnit->IsVehicle() && pVehicle == pUnit)
	{
		pVehicle = NULL;
		UpdateGroupStats(); // we may have lost a galleon
		return;
	}

	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pUnit == pForwardUnits[a])
		{
			--numForwardUnits;
			for(int b=a; b<numForwardUnits; ++b)
				pForwardUnits[b] = pForwardUnits[b+1];
			pForwardUnits[numForwardUnits] = NULL;
			break;
		}
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pUnit == pRearUnits[a])
		{
			--numRearUnits;
			for(int b=a; b<numRearUnits; ++b)
				pRearUnits[b] = pRearUnits[b+1];
			pRearUnits[numRearUnits] = NULL;
			break;
		}
	}

	if(pUnit->IsHero())
		UpdateGroupStats();

	MFDebug_Assert(ValidateGroup(), "EEK!");
}

void Group::UpdateGroupStats()
{
	for(int a=0; a<numForwardUnits; ++a)
		pForwardUnits[a]->UpdateStats();

	for(int a=0; a<numRearUnits; ++a)
		pRearUnits[a]->UpdateStats();

	if(pVehicle)
		pVehicle->UpdateStats();
}

int Group::GetMovement() const
{
	int movement = 0x7FFFFFFF;

	if(pVehicle)
	{
		return pVehicle->GetMovement();
	}
	else
	{
		for(int a=0; a<numForwardUnits; ++a)
		{
			movement = MFMin(movement, pForwardUnits[a]->GetMovement());
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			movement = MFMin(movement, pRearUnits[a]->GetMovement());
		}
	}

	return movement;
}

bool Group::SubtractMovementCost(MapTile *pTile)
{
	if(pVehicle)
	{
		int penalty = pVehicle->GetMovementPenalty(pTile);
		if(!penalty || pVehicle->GetMovement() < penalty)
			return false;

		pVehicle->Move(penalty);
	}
	else
	{
		for(int a=0; a<numForwardUnits; ++a)
		{
			int penalty = pForwardUnits[a]->GetMovementPenalty(pTile);
			if(!penalty || pForwardUnits[a]->GetMovement() < penalty)
				return false;
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			int penalty = pRearUnits[a]->GetMovementPenalty(pTile);
			if(!penalty || pRearUnits[a]->GetMovement() < penalty)
				return false;
		}

		// all units can move, lets go!
		for(int a=0; a<numForwardUnits; ++a)
		{
			int penalty = pForwardUnits[a]->GetMovementPenalty(pTile);
			pForwardUnits[a]->Move(penalty);
		}
		for(int a=0; a<numRearUnits; ++a)
		{
			int penalty = pRearUnits[a]->GetMovementPenalty(pTile);
			pRearUnits[a]->Move(penalty);
		}
	}

	return true;
}

bool Group::IsInGroup(const Unit *pUnit) const
{
	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pUnit == pForwardUnits[a])
			return true;
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pUnit == pRearUnits[a])
			return true;
	}
	return false;
}

Unit *Group::GetHero()
{
	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pForwardUnits[a]->IsHero())
			return pForwardUnits[a];
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		if(pRearUnits[a]->IsHero())
			return pRearUnits[a];
	}
	return NULL;
}

void Group::SetPlayer(int _player)
{
	player = _player;

	if(pVehicle)
		pVehicle->SetPlayer(_player);

	for(int a=0; a<numForwardUnits; ++a)
	{
		pForwardUnits[a]->SetPlayer(_player);
	}
	for(int a=0; a<numRearUnits; ++a)
	{
		pRearUnits[a]->SetPlayer(_player);
	}
}

void Group::FindPath(int x, int y)
{
	pathX = x;
	pathY = y;

	if(bSelected)
		pPath = pTile->Map().FindPath(this, x, y);
}

bool Group::ValidateGroup() const
{
	if(pVehicle && pVehicle->pGroup != this)
		return false;

	for(int a=0; a<numForwardUnits; ++a)
	{
		if(pForwardUnits[a]->pGroup != this)
			return false;
	}

	for(int a=0; a<numRearUnits; ++a)
	{
		if(pRearUnits[a]->pGroup != this)
			return false;
	}

	return true;
}
