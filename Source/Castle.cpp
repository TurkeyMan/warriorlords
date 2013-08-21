#include "Warlords.h"
#include "Castle.h"
#include "Group.h"

void Castle::Init(int _id, const CastleDetails &_details, int _player)
{
	pTile = NULL;

	id = _id;
	details = _details;
	player = _player;

	building = -1;
	buildTime = 0;
	nextBuild = -1;
}

MapTile *Castle::GetTile(int index) const
{
	if(index >= 2)
	{
		int width;
		pTile->Map().GetMapSize(&width, NULL);
		return pTile + width + (index & 1);
	}
	else
	{
		return pTile + index;
	}
}

bool Castle::IsEmpty() const
{
	if(pTile[0].GetNumUnits() != 0 || pTile[1].GetNumUnits() != 0)
		return false;

	// check the second row... very painfully mind you.
	int width;
	pTile->Map().GetMapSize(&width, NULL);
	MapTile *pT = pTile + width;
	return pT[0].GetNumUnits() == 0 && pT[1].GetNumUnits() == 0;
}

Group *Castle::GetMercGroup()
{
	static const int odds[16] = { 1, 1, 1, 1, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4 };

	GameState &gameState = pTile->Map().GameState();

	Group *pGroup = Group::Create(-1);

	int choices[4];
	int numChoices = 0;
	for(int a=0; a<details.numBuildUnits; ++a)
	{
		int unit = details.buildUnits[a].unit;
		if(UnitDefs()->GetUnitDetails(unit).type == UT_Unit)
			choices[numChoices++] = unit;
	}

	MFDebug_Assert(numChoices > 0, "Castle doesn't produce any units! What to fight?");

	int range = MFMin(15, 10 + gameState.CurrentRound());
	int numUnits = odds[MFRand() % range];
	for(int a=0; a<numUnits; ++a)
	{
		pGroup->AddUnit(UnitDefs()->CreateUnit(choices[MFRand()%numChoices], -1, &gameState));
	}

	return pGroup;
}

void Castle::Capture(Group *pGroup)
{
	ClearBuildUnit(); // this must happen before we reassign the player, since it may need to clear a hero
	player = pGroup->GetPlayer();

	for(int a=0; a<4; ++a)
	{
		MapTile *pCastleTile = GetTile(a);

		for(int b=0; b<pCastleTile->GetNumGroups(); ++b)
		{
			Group *pUnits = pCastleTile->GetGroup(b);
			pTile->Map().GameState().History().PushCaptureUnits(pUnits);
			pUnits->SetPlayer(player);
		}
	}
}

void Castle::ClearBuildUnit()
{
	SetBuildUnit(-1);
	building = -1;
	buildTime = 0;
}

void Castle::SetBuildUnit(int slot)
{
	// if we're currently building a hero, clear the castle binding
	if(nextBuild >= 4)
		pTile->Map().GameState().SetHeroRebuildCastle(player, nextBuild - 4, NULL);

	// if we have selected to build a hero, bind it to the castle
	if(slot >= 4)
		pTile->Map().GameState().SetHeroRebuildCastle(player, slot - 4, this);

	nextBuild = slot;
}

int Castle::GetBuildUnit() const
{
	if(nextBuild == -1)
		return -1;
	return nextBuild < 4 ? details.buildUnits[nextBuild].unit : pTile->Map().GameState().PlayerHero(player, nextBuild - 4)->GetType();
}

int Castle::BuildTimeRemaining() const
{
	if(nextBuild == building)
		return buildTime;

	return GetUnitBuildTime();
}

int Castle::GetUnitBuildTime() const
{
	int unit = nextBuild != -1 ? nextBuild : building;
	if(unit == -1)
		return 0;

	return unit < 4 ? details.buildUnits[unit].buildTime : pTile->Map().GameState().PlayerHero(player, unit - 4)->GetDetails().buildTime;
}

const UnitDefinitions *Castle::UnitDefs() const
{
	return pTile->Map().UnitDefs();
}
