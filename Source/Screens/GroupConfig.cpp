#include "Warlords.h"
#include "GroupConfig.h"
#include "MapScreen.h"

#include "Fuji/MFPrimitive.h"
#include "Fuji/MFRenderer.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/MFFont.h"

#include "stdio.h"

#include "Game.h"

void DrawHealthBar(int x, int y, int maxHealth, float currentHealth);

static float gPositions[5][5][2] =
{
	{ {.5f, .5f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .5f},  {.67f, .5f},  {0.f, 0.f},   {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .5f},  {.32f, .75f}, {0.f, 0.f},   {0.f, 0.f}  },
	{ {.32f, .25f}, {.67f, .25f}, {.32f, .75f}, {.67f, .75f}, {0.f, 0.f}  },
	{ {.16f, .25f}, {.84f, .25f}, {.5f, .5f},   {.16f, .75f}, {.84f, .75f} }
};

GroupConfig::GroupConfig()
: Window(true)
{
	pMelee = MFMaterial_Create("Melee");
	pRanged = MFMaterial_Create("Ranged");

	MFRect center, middle;
	DivideRect_Vert(window, window.height - 64 - margin*4, 0.f, &center, &lower, true);
	DivideRect_Vert(center, center.height - 64*2 - margin*8, 0.f, &top, &middle, true);
	DivideRect_Horiz(middle, 0.5f, 0.f, &rear, &front, false);
}

GroupConfig::~GroupConfig()
{
	MFMaterial_Destroy(pMelee);
	MFMaterial_Destroy(pRanged);
}

bool GroupConfig::DrawContent()
{
	if(!bVisible)
		return false;

	if(battleConfig.Draw())
		return true;

	float texelCenter = MFRenderer_GetTexelCenterOffset();

	// draw backgrounds
	float tc = texelCenter * (1.f/128.f);
	float xc = window.x + window.width*0.5f;
	float yc = window.y + window.height*0.5f;

	MFMaterial_SetMaterial(pRanged);
	MFPrimitive_DrawQuad(xc - 192.f + 16.f, yc - 80.f, 128.f, 128.f, MFVector::white, tc, tc, 1+tc, 1+tc);

	MFMaterial_SetMaterial(pMelee);
	MFPrimitive_DrawQuad(xc + 64.f - 16.f, yc - 80.f, 128.f, 128.f, MFVector::white, tc, tc, 1+tc, 1+tc);

	// draw separators
	Game *pGame = Game::GetCurrent();
	pGame->DrawLine(rear.x + 5.f, rear.y+rear.height, front.x + front.width - 5.f, rear.y+rear.height);

	for(int a = 1; a < numGroups; ++a)
	{
		MFRect rect;
		GetBottomPanel(a, &rect);
		pGame->DrawLine(rect.x + rect.width, rect.y + 5.f, rect.x + rect.width, rect.y + rect.height - 5.f);
	}

	// draw units
	for(int g = 0; g < numGroups; ++g)
	{
		for(int a=10; a>=0; --a)
		{
			MFRect rect;
			if(GetUnitPos(g, a, &rect))
				pGroups[g]->pUnits[a]->Draw(rect.x, rect.y);
		}
	}

	pDefs->DrawUnits(64.f, MFRenderer_GetTexelCenterOffset(), false, true);

	// get the font
	MFFont *pFont = Game::GetCurrent()->GetSmallNumbersFont();
	float height = MFFont_GetFontHeight(pFont);

	// detail the units
	int offset = 0;
	for(int g = 0; g < numGroups; ++g)
	{
/*
		char uuu[16];
		sprintf(uuu, "%d-%d-%d%s", pGroups[g]->totalUnits, pGroups[g]->numForward, pGroups[g]->numRear, pGroups[g]->pVehicle ? "-*" : "");
		float moveWidth = MFFont_GetStringWidth(pFont, uuu, height);

		MFFont_BlitText(pFont, (int)window.x, (int)window.y + offset, MFVector::white, uuu);
*/
		offset += 16;

		for(int a = 10; a >= 0; --a)
		{
			Unit *pUnit = pGroups[g]->pUnits[a];
			if(!pUnit)
				continue;

			MFRect rect;
			GetUnitPos(g, a, &rect);

			if(g == 0 && a < 10)
				DrawHealthBar((int)rect.x, (int)rect.y, pUnit->GetMaxHP(), pUnit->GetHealth());

			char move[8];
			sprintf(move, "%g", pUnit->MoveRemaining());

			float moveWidth = MFFont_GetStringWidth(pFont, move, height);

			MFFont_BlitText(pFont, (int)rect.x + 62 - (int)moveWidth, (int)rect.y + 62 - (int)height, MFVector::white, move);
		}
	}

	return true;
}

bool GroupConfig::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	// HACK: right click returns
	if(info.buttonID == 1 && ev == IE_Tap)
		Hide();

	// only handle left clicks
	if(info.buttonID != 0)
		return true;

	switch(ev)
	{
		case IE_Tap:
		{
			// see if we have selected a unit
			int unit = GetUnitFromPoint(info.tap.x, info.tap.y);
			if(unit > -1)
			{
				if(unit < 10)
					battleConfig.Show(pGroups[0]->pUnits[unit]);
			}
			else
			{
				// see if we have selected another group
				int group = GetGroupFromPoint(info.tap.x, info.tap.y);
				if(group > 0 && group < numGroups)
				{
					UnitGroup *pT = pGroups[group];
					for(int a=group; a>0; --a)
						pGroups[a] = pGroups[a-1];
					pGroups[0] = pT;
				}
			}
			break;
		}
		case IE_Drag:
		{
			dragX = (int)info.drag.x;
			dragY = (int)info.drag.y;

			if(dragUnit == -1 && dragGroup == -1)
			{
				// check for unit drag
				dragUnit = GetUnitFromPoint(info.drag.startX, info.drag.startY);
				if(dragUnit > -1)
					break;

				// check for group drag
				dragGroup = GetGroupFromPoint(info.drag.startX, info.drag.startY);
				if(dragGroup >= numGroups)
					dragGroup = -1;
				if(dragGroup > 0)
					break;
			}
			break;
		}
		case IE_Up:
		{
			// find where we dropped it...
			if(dragUnit != -1)
			{
				UnitGroup *pActiveGroup = pGroups[0];
				Unit *pDragUnit = pActiveGroup->pUnits[dragUnit];
				bool bFound = false;

				if(dragUnit < 10)
				{
					// check if we're swapping with another unit
					int unit = GetUnitFromPoint(info.up.x, info.up.y);
					if(unit > -1 && unit < 10)
					{
						pActiveGroup->pUnits[dragUnit] = pActiveGroup->pUnits[unit];
						pActiveGroup->pUnits[unit] = pDragUnit;
						bFound = true;
					}
					else
					{
						// check dropping in the other file
						int dropped = GetFileFromPoint(info.up.x, info.up.y);
						if(dropped > -1)
						{
							if(dropped == 0 && dragUnit >= 5)
							{
								if(pActiveGroup->numForward < 5)
								{
									pActiveGroup->pForward[pActiveGroup->numForward++] = pDragUnit;

									for(int a=dragUnit-5; a<pActiveGroup->numRear; ++a)
										pActiveGroup->pRear[a] = pActiveGroup->pRear[a + 1];
									pActiveGroup->pRear[--pActiveGroup->numRear] = NULL;
								}
							}
							else if(dropped == 1 && dragUnit < 5)
							{
								if(pActiveGroup->numRear < 5)
								{
									pActiveGroup->pRear[pActiveGroup->numRear++] = pDragUnit;

									for(int a=dragUnit; a<pActiveGroup->numForward; ++a)
										pActiveGroup->pForward[a] = pActiveGroup->pForward[a + 1];
									pActiveGroup->pForward[--pActiveGroup->numForward] = NULL;
								}
							}

							bFound = true;
						}
					}
				}

				// check for dropping in other groups
				if(!bFound)
				{
					int group = GetGroupFromPoint(info.up.x, info.up.y);

					if(group > -1)
					{
						UnitGroup *pGroup = pGroups[group];

						// if we've created a new group
						if(group == numGroups)
						{
							MFZeroMemory(pGroup, sizeof(*pGroup));
							++numGroups;
						}

						if(dragUnit == 10 && pGroup->pVehicle && pGroup->totalUnits == 1)
						{
							// swap vehicles
							pActiveGroup->pVehicle = pGroup->pVehicle;
							pGroup->pVehicle = pDragUnit;
						}
						else
						{
							// move unit into group
							if(dragUnit == 10)
							{
								if(!pGroup->pVehicle)
								{
									pGroup->pVehicle = pDragUnit;
									++pGroup->totalUnits;

									pActiveGroup->pVehicle = NULL;
									--pActiveGroup->totalUnits;
								}
							}
							else if(pGroup->numForward < 5 || pGroup->numRear < 5)
							{
								if(pDragUnit->IsRanged())
								{
									if(pGroup->numRear < 5)
										pGroup->pRear[pGroup->numRear++] = pDragUnit;
									else
										pGroup->pForward[pGroup->numForward++] = pDragUnit;
								}
								else
								{
									if(pGroup->numForward < 5)
										pGroup->pForward[pGroup->numForward++] = pDragUnit;
									else
										pGroup->pRear[pGroup->numRear++] = pDragUnit;
								}

								if(dragUnit < 5)
								{
									for(int a=dragUnit; a<pActiveGroup->numForward; ++a)
										pActiveGroup->pForward[a] = pActiveGroup->pForward[a + 1];
									pActiveGroup->pForward[--pActiveGroup->numForward] = NULL;
								}
								else
								{
									for(int a=dragUnit-5; a<pActiveGroup->numRear; ++a)
										pActiveGroup->pRear[a] = pActiveGroup->pRear[a + 1];
									pActiveGroup->pRear[--pActiveGroup->numRear] = NULL;
								}

								++pGroup->totalUnits;
								--pActiveGroup->totalUnits;
							}
						}

						bFound = true;
					}
				}

				// check if we have just emptied the group
				if(pActiveGroup->totalUnits == 0)
				{
					--numGroups;
					for(int a=0; a<numGroups; ++a)
						pGroups[a] = pGroups[a + 1];
					pGroups[numGroups] = pActiveGroup;
				}

				dragUnit = -1;
			}
			else if(dragGroup != -1)
			{
				UnitGroup *pDragGroup = pGroups[dragGroup];

				// check if we're swapping vehicles
				int unit = GetUnitFromPoint(info.up.x, info.up.y);
				if(unit == 10 && pDragGroup->pVehicle && pDragGroup->totalUnits == 1)
				{
					// swap vehicles
					Unit *pSwap = pGroups[0]->pVehicle;
					pGroups[0]->pVehicle = pDragGroup->pVehicle;
					pDragGroup->pVehicle = pSwap;
				}
				else
				{
					// check if we dropped the group on another group
					int group = GetGroupFromPoint(info.up.x, info.up.y);

					if(group == numGroups || group == dragGroup)
					{
						group = -1;
					}
					else if(group == -1)
					{
						// check if we dropped the group in one of the top files
						int file = GetFileFromPoint(info.up.x, info.up.y);
						if(file > -1)
							group = 0;
					}

					if(group > -1)
					{
						UnitGroup *pTargetGroup = pGroups[group];

						// move vehicle
						if(pDragGroup->pVehicle && !pTargetGroup->pVehicle)
						{
							pTargetGroup->pVehicle = pDragGroup->pVehicle;
							pDragGroup->pVehicle = NULL;
							++pTargetGroup->totalUnits;
							--pDragGroup->totalUnits;
						}

						// attempt to move each unit into their preferred file
						for(int unit=0; unit<10; ++unit)
						{
							if(pTargetGroup->numForward >= 5 && pTargetGroup->numRear >= 5)
								break;

							Unit *pUnit = pDragGroup->pUnits[unit];
							if(!pUnit)
								continue;

							// add to active group
							if(pUnit->IsRanged())
							{
								if(pTargetGroup->numRear < 5)
									pTargetGroup->pRear[pTargetGroup->numRear++] = pUnit;
								else
									pTargetGroup->pForward[pTargetGroup->numForward++] = pUnit;
							}
							else
							{
								if(pTargetGroup->numForward < 5)
									pTargetGroup->pForward[pTargetGroup->numForward++] = pUnit;
								else
									pTargetGroup->pRear[pTargetGroup->numRear++] = pUnit;
							}

							// remove from drag group
							if(unit < 5)
							{
								for(int a=unit; a<pDragGroup->numForward; ++a)
									pDragGroup->pForward[a] = pDragGroup->pForward[a + 1];
								pDragGroup->pForward[--pDragGroup->numForward] = NULL;
							}
							else
							{
								for(int a=unit-5; a<pDragGroup->numRear; ++a)
									pDragGroup->pRear[a] = pDragGroup->pRear[a + 1];
								pDragGroup->pRear[--pDragGroup->numRear] = NULL;
							}

							++pTargetGroup->totalUnits;
							--pDragGroup->totalUnits;

							// since we removed the unit, we need to re-test the same index
							--unit;
						}
					}
				}

				// check if we have just emptied the group
				if(pDragGroup->totalUnits == 0)
				{
					--numGroups;
					for(int a=dragGroup; a<numGroups; ++a)
						pGroups[a] = pGroups[a + 1];
					pGroups[numGroups] = pDragGroup;
				}

				dragGroup = -1;
			}
			break;
		}
	}

	return Window::HandleInputEvent(ev, info);
}

int GroupConfig::GetUnitFromPoint(float x, float y)
{
	MFRect rect;

	int numForward = pGroups[0]->numForward;
	for(int a=0; a<numForward; ++a)
	{
		GetUnitPos(0, a, &rect);
		if(MFTypes_PointInRect(x, y, &rect))
		{
			if(a != dragUnit)
				return a;
		}
	}

	int numRear = pGroups[0]->numRear;
	for(int a=0; a<numRear; ++a)
	{
		GetUnitPos(0, a + 5, &rect);
		if(MFTypes_PointInRect(x, y, &rect))
		{
			if(a + 5 != dragUnit)
				return a + 5;
		}
	}

	if(pGroups[0]->pVehicle)
	{
		GetUnitPos(0, 10, &rect);
		if(MFTypes_PointInRect(x, y, &rect))
		{
			if(10 != dragUnit)
				return 10;
		}

	}

	return -1;
}

int GroupConfig::GetFileFromPoint(float x, float y)
{
	if(MFTypes_PointInRect(x, y, &front))
		return 0;
	else if(MFTypes_PointInRect(x, y, &rear))
		return 1;
	return -1;
}

int GroupConfig::GetGroupFromPoint(float x, float y)
{
	float bottomUsed = 0.f;
	const float margin = 5.f;

	for(int a=1; a<numGroups; ++a)
	{
		MFRect rect;
		rect.x = lower.x + bottomUsed + margin*2.f;
		rect.y = lower.y;
		rect.width = 32.f + (float)pGroups[a]->totalUnits*32.f;
		rect.height = lower.height;
		bottomUsed += rect.width + margin;

		if(MFTypes_PointInRect(x, y, &rect))
			return a;
	}

	if(bottomUsed < lower.x + lower.width - margin*4.f)
	{
		MFRect rect;
		rect.x = lower.x + bottomUsed + margin*2.f;
		rect.y = lower.y;
		rect.width = lower.x + lower.width - bottomUsed - margin*4.f;
		rect.height = lower.height;

		if(MFTypes_PointInRect(x, y, &rect))
			return numGroups;
	}

	return -1;
}

bool GroupConfig::GetUnitPos(int group, int unit, MFRect *pRect)
{
	if(!pGroups[group]->pUnits[unit])
		return false;

	UnitDetails *pDetails = pGroups[group]->pUnits[unit]->GetDetails();
	pRect->width = (float)pDetails->width * 64.f;
	pRect->height = (float)pDetails->height * 64.f;

	if(group == 0)
	{
		if(unit == dragUnit)
		{
			pRect->x = MFFloor(dragX - 32.f);
			pRect->y = MFFloor(dragY - 32.f);
			return true;
		}

		if(unit == 10)
		{
			pRect->x = MFFloor((rear.x + rear.width + front.x)*0.5f - 32.f);
			pRect->y = MFFloor(front.y + front.height*0.5f - 32.f);
			return true;
		}
		else if(unit >= 5)
		{
			int numRear = pGroups[0]->numRear;
			pRect->x = MFFloor(rear.x + gPositions[numRear-1][unit-5][0]*rear.width - 32.f);
			pRect->y = MFFloor(rear.y + gPositions[numRear-1][unit-5][1]*rear.height - 32.f);
			return true;
		}
		else
		{
			int numForward = pGroups[0]->numForward;
			pRect->x = MFFloor(front.x + gPositions[numForward-1][unit][0]*front.width - 32.f);
			pRect->y = MFFloor(front.y + gPositions[numForward-1][unit][1]*front.height - 32.f);
			return true;
		}
	}
	else
	{
		float bottomUsed = 0.f;
		const float margin = 5.f;

		for(int a=1; a<group; ++a)
			bottomUsed += 32.f + (float)pGroups[a]->totalUnits*32.f + margin;

		int visUnit = 0;
		for(int a=0; a<unit; ++a)
		{
			if(pGroups[group]->pUnits[a])
				++visUnit;
		}

		if(group == dragGroup)
		{
			float groupWidth = ((float)pGroups[group]->totalUnits - 1.f)*32.f;
			pRect->x = dragX - (64.f + groupWidth)*0.5f + (float)visUnit*32.f;
			pRect->y = dragY - 32.f;
		}
		else
		{
			pRect->x = lower.x + bottomUsed + margin*2.f + (float)visUnit*32.f;
			pRect->y = lower.y + margin;
		}
		return true;
	}

	return false;
}

void GroupConfig::GetBottomPanel(int i, MFRect *pRect)
{
	float bottomUsed = 0.f;
	const float margin = 5.f;

	for(int a=1; a<numGroups; ++a)
	{
		pRect->x = lower.x + bottomUsed + margin*2.f;
		pRect->y = lower.y;
		pRect->width = 32.f + (float)pGroups[a]->totalUnits*32.f;
		pRect->height = lower.height;
		bottomUsed += pRect->width;
	}
}

void GroupConfig::Show(MapTile *_pTile)
{
	Game *pGame = Game::GetCurrent();
	pGame->GetUI()->ShowWindowContainer(true);

	Window::Show();

	pTile = _pTile;
	pDefs = NULL;

	dragUnit = -1;
	dragGroup = -1;

	MFZeroMemory(groups, sizeof(groups));

	for(int a=0; a<MapTile::MaxUnitsOnTile * 2; ++a)
		pGroups[a] = &groups[a];

	numGroups = pTile->GetNumGroups();
	for(int g=0; g<numGroups; ++g)
	{
		Group *pGroup = pTile->GetGroup(g);
		MFDebug_Assert(pGroup->ValidateGroup(), "EEK!");

		groups[g].numForward = pGroup->GetNumForwardUnits();
		for(int a=0; a<groups[g].numForward; ++a)
			groups[g].pForward[a] = pGroup->GetForwardUnit(a);

		groups[g].numRear = pGroup->GetNumRearUnits();
		for(int a=0; a<groups[g].numRear; ++a)
			groups[g].pRear[a] = pGroup->GetRearUnit(a);

		groups[g].pVehicle = pGroup->GetVehicle();

		groups[g].totalUnits = groups[g].numForward + groups[g].numRear + (groups[g].pVehicle ? 1 : 0);
	}

	for(int a=0; a<11; ++a)
	{
		if(groups[0].pUnits[a])
		{
			pDefs = groups[0].pUnits[a]->GetDefs();
			break;
		}
	}
}

void GroupConfig::Hide()
{
	Game *pGame = Game::GetCurrent();
	pGame->GetUI()->ShowWindowContainer(false);

	Window::Hide();

	struct SubRegroup
	{
		MFArray<Group*> before;
		MFArray<Group*> after;
	};

	PendingAction::Regroup::Before before[MapTile::MaxUnitsOnTile * 2];
	PendingAction::Regroup::After after[MapTile::MaxUnitsOnTile * 2];
	SubRegroup subs[MapTile::MaxUnitsOnTile * 2];
	int numBefore = 0, numAfter = 0, numSubs = 0, numSubGroups = 0;

	// populate the 'before' state
	numBefore = pTile->GetNumGroups();
	for(int i=0; i<numBefore; ++i)
	{
		PendingAction::Regroup::Before &g = before[i];
		g.pGroup = pTile->GetGroup(0);
		g.pPreviousAction = g.pGroup->pLastAction;
		pTile->RemoveGroup(g.pGroup);
	}

	// popoulate the afters, and build sub-regroups
	for(int a=0; a<numGroups; ++a)
	{
		UnitGroup &group = *pGroups[a];

		PendingAction::Regroup::After &aft = after[numAfter++];

		// first find if it matches one of our 'before' groups verbatim
		int b=0;
		for(; b<numBefore; ++b)
		{
			Group *pBefore = before[b].pGroup;
			if(pBefore->GetNumForwardUnits() != group.numForward || pBefore->GetNumRearUnits() != group.numRear || pBefore->GetVehicle() != group.pVehicle)
				continue;

			// compare front
			int i=0;
			for(; i<group.numForward; ++i)
			{
				if(pBefore->GetForwardUnit(i) != group.pForward[i])
					break;
			}
			if(i<group.numForward)
				continue;

			// compare back
			for(i=0; i<group.numRear; ++i)
			{
				if(pBefore->GetRearUnit(i) != group.pRear[i])
					break;
			}
			if(i<group.numRear)
				continue;

			// we have an unmodified group!
			aft.pGroup = pBefore;
			aft.sub = numSubs++;

			SubRegroup &sub = subs[aft.sub];
			sub.before.push(pBefore);
			sub.after.push(pBefore);
			numSubGroups += 2;
			break;
		}
		if(b < numBefore)
			continue;

		// we have a new group...
		// find an existing sub-regroup it belongs in
		for(b=0; b<numSubs; ++b)
		{
			for(int i=0; i<11; ++i)
			{
				if(group.pUnits[i] && subs[b].before.contains(group.pUnits[i]->GetGroup()))
					goto bail;
			}
		}
bail:
		if(b == numSubs)
			aft.sub = numSubs++; // create new sub-regroup
		else
			aft.sub = b; // add to existing sub-regroup

		// find source groups and build new group
		SubRegroup &sub = subs[aft.sub];
		Group *pNewGroup = Group::Create(pGame->CurrentPlayer());

		for(int i=0; i<11; ++i)
		{
			if(group.pUnits[i])
			{
				Unit *pUnit = group.pUnits[i];

				// add the unit's old group to the sub-regroup if not already present...
				Group *pSourceGroup = pUnit->GetGroup();
				for(int j=0; j<sub.before.size(); ++j)
				{
					if(sub.before[j] == pSourceGroup)
						goto add_to_new_group;
				}

				// it's not already present, so add it
				sub.before.push(pSourceGroup);
				++numSubGroups;

add_to_new_group:
				// add the unit to it's new group
				if(i < 5)
					pNewGroup->AddForwardUnit(pUnit);
				else if(i < 10)
					pNewGroup->AddRearUnit(pUnit);
				else
					pNewGroup->AddUnit(pUnit);
			}
		}

		// update the groups stats
		pNewGroup->UpdateGroupStats();

		// and set it to the regroup action
		aft.pGroup = pNewGroup;
		sub.after.push(pNewGroup);
		++numSubGroups;
	}

	// put new groups on tile (in reverse order)
	for(int a=numAfter-1; a>=0; --a)
		pTile->AddGroup(after[a].pGroup);

	// select the top group
	pGame->SelectGroup(pTile->GetGroup(0));

	// check there was actually some activity
	if(numBefore == numAfter)
	{
		int a=0;
		for(; a<numBefore; ++a)
		{
			if(before[a].pGroup != after[a].pGroup)
				break;
		}
		if(a == numBefore)
			return;
	}

	// create regroup action
	PendingAction::Regroup regroup;
	regroup.x = pTile->GetX();
	regroup.y = pTile->GetY();
	regroup.pMem = MFHeap_AllocAndZero(sizeof(PendingAction::Regroup::Before)*numBefore + sizeof(PendingAction::Regroup::After)*numAfter + sizeof(PendingAction::Regroup::SubRegroup)*numSubs + sizeof(Group*)*numSubGroups);
	regroup.pBefore = (PendingAction::Regroup::Before*)regroup.pMem;
	regroup.numBefore = numBefore;
	regroup.pAfter = (PendingAction::Regroup::After*)&regroup.pBefore[numBefore];
	regroup.numAfter = numAfter;
	regroup.pSubRegroups = (PendingAction::Regroup::SubRegroup*)&regroup.pAfter[numAfter];
	regroup.numSubRegroups = numSubs;
	Group **ppSubGroups = (Group**)&regroup.pSubRegroups[numSubs];
	MFCopyMemory(regroup.pBefore, before, sizeof(PendingAction::Regroup::Before) * numBefore);
	MFCopyMemory(regroup.pAfter, after, sizeof(PendingAction::Regroup::After) * numAfter);
	for(int a=0; a<numSubs; ++a)
	{
		regroup.pSubRegroups[a].numBefore = subs[a].before.size();
		regroup.pSubRegroups[a].ppBefore = ppSubGroups;
		for(int b=0; b<subs[a].before.size(); ++b)
			*ppSubGroups++ = subs[a].before[b];
		regroup.pSubRegroups[a].numAfter = subs[a].after.size();
		regroup.pSubRegroups[a].ppAfter = ppSubGroups;
		for(int b=0; b<subs[a].after.size(); ++b)
			*ppSubGroups++ = subs[a].after[b];
	}
	pGame->PushRegroup(&regroup);
}
