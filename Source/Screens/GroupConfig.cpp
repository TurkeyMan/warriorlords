#include "Warlords.h"
#include "GroupConfig.h"
#include "MapScreen.h"

#include "MFPrimitive.h"
#include "MFRenderer.h"
#include "MFFont.h"

#include "stdio.h"

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
	MFRect center, middle;
	DivideRect_Vert(window, window.height - 64 - margin*4, 0.f, &center, &lower, true);
	DivideRect_Vert(center, center.height - 64*2 - margin*8, 0.f, &top, &middle, true);
	DivideRect_Horiz(middle, 0.5f, 0.f, &rear, &front, false);
}

GroupConfig::~GroupConfig()
{
}

bool GroupConfig::DrawContent()
{
	if(!bVisible)
		return false;

	if(battleConfig.Draw())
		return true;

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

	for(int g = 0; g < numGroups; ++g)
	{
		for(int a = 10; a >= 0; --a)
		{
			Unit *pUnit = pGroups[g]->pForward[a];
			if(!pUnit)
				continue;

			MFRect rect;
			GetUnitPos(g, a, &rect);

			if(g == 0 && a < 10)
				DrawHealthBar((int)rect.x, (int)rect.y, pUnit->GetMaxHP(), pUnit->GetHealth());

			char move[8];
			sprintf(move, "%g", pUnit->GetMovement() * 0.5f);

			MFFont *pFont = Game::GetCurrent()->GetSmallNumbersFont();
			float height = MFFont_GetFontHeight(pFont);
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
									--pActiveGroup->numRear;
								}
							}
							else if(dropped == 1 && dragUnit < 5)
							{
								if(pActiveGroup->numRear < 5)
								{
									pActiveGroup->pRear[pActiveGroup->numRear++] = pDragUnit;

									for(int a=dragUnit; a<pActiveGroup->numForward; ++a)
										pActiveGroup->pForward[a] = pActiveGroup->pForward[a + 1];
									--pActiveGroup->numForward;
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
									--pActiveGroup->numForward;
								}
								else
								{
									for(int a=dragUnit-5; a<pActiveGroup->numRear; ++a)
										pActiveGroup->pRear[a] = pActiveGroup->pRear[a + 1];
									--pActiveGroup->numRear;
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
				if(unit == 10 -1 && pDragGroup->pVehicle && pDragGroup->totalUnits == 1)
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
							Unit *pUnit = pDragGroup->pUnits[unit];
							if(!pUnit)
								continue;

							if(pTargetGroup->numForward >= 5 && pTargetGroup->numRear >= 5)
								break;

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
								--pDragGroup->numForward;
							}
							else
							{
								for(int a=unit-5; a<pDragGroup->numRear; ++a)
									pDragGroup->pRear[a] = pDragGroup->pRear[a + 1];
								--pDragGroup->numRear;
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
			pRect->x = MFFloor(dragX - pRect->width*.5f);
			pRect->y = MFFloor(dragY - pRect->height*.5f);
			return true;
		}

		if(unit == 10)
		{
			pRect->x = MFFloor((rear.x + rear.width + front.x)*0.5f - pRect->width*.5f);
			pRect->y = MFFloor(front.y + front.height*0.5f - pRect->height*.5f);
			return true;
		}
		else if(unit >= 5)
		{
			int numRear = pGroups[0]->numRear;
			pRect->x = MFFloor(rear.x + gPositions[numRear-1][unit-5][0]*rear.width - pRect->width*.5f);
			pRect->y = MFFloor(rear.y + gPositions[numRear-1][unit-5][1]*rear.height - pRect->height*.5f);
			return true;
		}
		else
		{
			int numForward = pGroups[0]->numForward;
			pRect->x = MFFloor(front.x + gPositions[numForward-1][unit][0]*front.width - pRect->width*.5f);
			pRect->y = MFFloor(front.y + gPositions[numForward-1][unit][1]*front.height - pRect->height*.5f);
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
			pRect->x = dragX - (pRect->width + groupWidth)*0.5f + (float)visUnit*32.f;
			pRect->y = dragY - pRect->height*0.5f;
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
	Window::Hide();

	Game *pGame = Game::GetCurrent();

	int numSourceGroups[MapTile::MaxUnitsOnTile * 2] = { 0 };
	Group *pSourceGroups[MapTile::MaxUnitsOnTile * 2][MapTile::MaxUnitsOnTile * 2];

	// find source groups for each group
	for(int a=0; a<numGroups; ++a)
	{
		UnitGroup *pGroup = pGroups[a];

		for(int u=0; u<11; ++u)
		{
			Unit *pUnit = pGroup->pUnits[u];
			if(pUnit)
			{
				bool bFound = false;
				for(int b=0; b<numSourceGroups[a]; ++b)
				{
					if(pUnit->GetGroup() == pSourceGroups[a][b])
					{
						bFound = true;
						break;
					}
				}

				if(!bFound)
					pSourceGroups[a][numSourceGroups[a]++] = pUnit->GetGroup();
			}
		}
	}

	// rearrange the groups
	for(int a=numGroups-1; a>=0; --a)
	{
		UnitGroup *pGroup = pGroups[a];

		// check if the group is only rearranged
		if(numSourceGroups[a] == 1 && pGroup->totalUnits == pSourceGroups[a][0]->GetNumUnits() + (pSourceGroups[a][0]->pVehicle ? 1 : 0))
		{
			Group *pSource = pSourceGroups[a][0];

			// check that a rearrange actually ocurred
			bool bWasRearranged = false;
			int i = 0;
			for(int b=0; b<10; ++b)
			{
				if(pGroup->pUnits[b] && pGroup->pUnits[b] != pSource->GetUnit(i++))
				{
					bWasRearranged = true;
					break;
				}
			}

			if(bWasRearranged)
			{
				// create rearrange action
				pGame->PushRearrange(pSource, pGroup->pUnits);

				// rearrange the group
				for(int b=0; b<10; ++b)
					pSource->pForwardUnits[b] = pGroup->pUnits[b];
				pSource->numForwardUnits = pGroup->numForward;
				pSource->numRearUnits = pGroup->numRear;				
			}

			// put the group on top
			pTile->BringGroupToFront(pSource);
		}
		else
		{
			// remove old groups from tile
			for(int b=0; b<numSourceGroups[a]; ++b)
				pTile->RemoveGroup(pSourceGroups[a][b]);

			// create new group
			Group *pNewGroup = Group::Create(pSourceGroups[a][0]->GetPlayer());
			for(int b=0; b<10; ++b)
			{
				if(pGroup->pUnits[b])
				{
					if(b < 5)
						pNewGroup->AddForwardUnit(pGroup->pUnits[b]);
					else
						pNewGroup->AddRearUnit(pGroup->pUnits[b]);
				}
			}

			pNewGroup->pVehicle = pGroup->pVehicle;
			if(pNewGroup->pVehicle)
				pNewGroup->pVehicle->SetGroup(pNewGroup);

			// and add the group to the map
			pTile->AddGroup(pNewGroup);

			// create regroup action
			pGame->PushRegroup(pSourceGroups[a], numSourceGroups[a], pNewGroup);
		}
	}

	pGame->GetMapScreen()->SelectGroup(pTile->GetGroup(0));
}
