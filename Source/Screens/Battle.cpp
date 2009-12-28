#include "Warlords.h"
#include "Battle.h"
#include "Unit.h"
#include "Path.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"

#include "MFRenderer.h"
#include "MFInput.h"
#include "Display.h"

// remove me!
#include "MFIni.h"

Battle::Battle(Game *_pGame)
{
	pGame = _pGame;

	// load lots of battle related stuff
	pIcons = MFMaterial_Create("BattleIcons");

	pTileSet = pGame->GetMap()->GetTileset();
	int numTerrains = pTileSet->GetNumTerrainTypes();
	ppForegrounds = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numTerrains);
	ppBackgrounds = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numTerrains);
	for(int a=0; a<numTerrains; ++a)
	{
		ppForegrounds[a] = MFMaterial_Create(MFStr("Battle/fg-%s", pTileSet->GetTerrainName(a)));
		ppBackgrounds[a] = MFMaterial_Create(MFStr("Battle/bg-%s", pTileSet->GetTerrainName(a)));
	}

	pUnitDefs = pGame->GetUnitDefs();
	int numRaces = pUnitDefs->GetNumRaces();
	ppCastles = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numRaces);
	for(int a=0; a<numRaces; ++a)
	{
		ppCastles[a] = MFMaterial_Create(MFStr("Battle/castle-%s", pUnitDefs->GetRaceName(a)));
	}
}

Battle::~Battle()
{
	MFMaterial_Destroy(pIcons);

	int numTerrains = pTileSet->GetNumTerrainTypes();
	for(int a=0; a<numTerrains; ++a)
	{
		MFMaterial_Destroy(ppForegrounds[a]);
		MFMaterial_Destroy(ppBackgrounds[a]);
	}
	MFHeap_Free(ppForegrounds);
	MFHeap_Free(ppBackgrounds);

	int numRaces = pUnitDefs->GetNumRaces();
	for(int a=0; a<numRaces; ++a)
	{
		MFMaterial_Destroy(ppCastles[a]);
	}
	MFHeap_Free(ppCastles);
}

void Battle::Begin(Group *pGroup, MapTile *pTarget, int foreground, int background, int castle)
{
	pForeground = ppForegrounds[foreground];
	pBackground = ppBackgrounds[background];
	pCastle = castle != -1 ? ppCastles[castle] : NULL;

	pCooldownHead = pCooldownTail = NULL;
	pActionHead = pActionTail = NULL;
	cooldownCount = 0;

#if 0
	MFIni *pIni = MFIni::Create("TestBattle");
	if(!pIni)
		return;

	int numUnits = pUnitDefs->GetNumUnitTypes();
	groups[0].player = 0;
	groups[1].player = 1;
	groups[2].player = 2;
	groups[3].player = 3;
	groups[4].player = 4;
	groups[5].player = 5;
	groups[6].player = 6;
	groups[7].player = 7;

	MFIniLine *pLine = pIni->GetFirstLine();

	while(pLine)
	{
		if(pLine->IsSection("Battle"))
		{
			MFIniLine *pBattle = pLine->Sub();

			while(pBattle)
			{
				if(pBattle->IsString(0, "background"))
				{
					for(int a=0; a<pTileSet->GetNumTerrainTypes(); ++a)
					{
						if(!MFString_CaseCmp(pBattle->GetString(1), pTileSet->GetTerrainName(a)))
						{
							pBackground = ppBackgrounds[a];
							break;
						}
					}
				}
				else if(pBattle->IsString(0, "foreground"))
				{
					for(int a=0; a<pTileSet->GetNumTerrainTypes(); ++a)
					{
						if(!MFString_CaseCmp(pBattle->GetString(1), pTileSet->GetTerrainName(a)))
						{
							pForeground = ppForegrounds[a];
							break;
						}
					}
				}
				else if(pBattle->IsString(0, "castle"))
				{
					//          pCastle = MFMaterial_Create(MFStr("Battle/castle-%s", pBattle->GetString(1)));
				}
				else if(pBattle->IsString(0, "army1"))
				{
					races[0] = pBattle->GetInt(1) - 1;
				}
				else if(pBattle->IsString(0, "army2"))
				{
					races[1] = pBattle->GetInt(1) - 1;
				}

				for(int g=0; g<8; ++g)
				{
					if(pBattle->IsSection(MFStr("Group%d", g+1)))
					{
						MFIniLine *pUnits = pBattle->Sub();
						while(pUnits)
						{
							const char *pName = pUnits->GetString(0);

							for(int a=0; a<numUnits; ++a)
							{
								if(!MFString_CaseCmp(pUnitDefs->GetUnitTypeName(a), pName))
								{
									Unit *pU = pUnitDefs->CreateUnit(a, g+1);
									if(!pU->IsHero() && pU->IsRanged() && groups[g].numRearUnits < 5)
										groups[g].pRearUnits[groups[g].numRearUnits++] = pU;
									else if(groups[g].numForwardUnits < 5)
										groups[g].pForwardUnits[groups[g].numForwardUnits++] = pU;
									break;
								}
							}

							pUnits = pUnits->Next();
						}
					}
				}

				pBattle = pBattle->Next();
			}
		}
		pLine = pLine->Next();
	}

	MFIni::Destroy(pIni);

	// initialise the battle units
	armies[0].pGroup = &groups[races[0]];
	armies[1].pGroup = &groups[races[1]];
#endif

	// initialise the battle units
	MFZeroMemory(armies, sizeof(armies));
	MFZeroMemory(armies, sizeof(armies));

	MFRect rect;
	MFDisplay_GetDisplayRect(&rect);

	int centerLine = (int)(rect.height / 2);
	int centerField = centerLine + centerLine / 2;
	int horizSpacing = (int)(rect.width / 32);

	// populate the armies
	for(int b=0; b<5; ++b)
	{
		if(b < pGroup->GetNumForwardUnits())
		{
			BattleUnit *pUnit = &armies[0].units[armies[0].numUnits++];
			pUnit->Init(pGroup->GetForwardUnit(b));
			pUnit->pGroup = pGroup;
			pUnit->army = 0;
			pUnit->row = 0;
			++armies[0].numForwardUnits;
		}
		if(b < pGroup->GetNumRearUnits())
		{
			BattleUnit *pUnit = &armies[0].units[armies[0].numUnits++];
			pUnit->Init(pGroup->GetRearUnit(b));
			pUnit->pGroup = pGroup;
			pUnit->army = 0;
			pUnit->row = 1;
			++armies[0].numRearUnits;
		}
	}
	armies[0].numUnitsAlive = armies[0].numUnits;
	armies[0].player = pGroup->GetPlayer();

	for(int a=0; a<pTarget->GetNumGroups(); ++a)
	{
		Group *pTileGroup = pTarget->GetGroup(a);

		for(int b=0; b<5; ++b)
		{
			if(b < pTileGroup->GetNumForwardUnits())
			{
				BattleUnit *pUnit = &armies[1].units[armies[1].numUnits++];
				pUnit->Init(pTileGroup->GetForwardUnit(b));
				pUnit->pGroup = pTileGroup;
				pUnit->army = 1;
				pUnit->row = 0;
				++armies[1].numForwardUnits;
			}
			if(b < pTileGroup->GetNumRearUnits())
			{
				BattleUnit *pUnit = &armies[1].units[armies[1].numUnits++];
				pUnit->Init(pTileGroup->GetRearUnit(b));
				pUnit->pGroup = pTileGroup;
				pUnit->army = 1;
				pUnit->row = 1;
				++armies[1].numRearUnits;
			}
		}
		armies[1].numUnitsAlive = armies[1].numUnits;
		armies[1].player = pTileGroup->GetPlayer();
	}

	// unit the units
	int rowSeparation[2][2];
	rowSeparation[0][0] = (centerLine + 64) / (armies[0].numForwardUnits + 1);
	rowSeparation[0][1] = (centerLine + 64) / (armies[0].numRearUnits + 1);
	rowSeparation[1][0] = (centerLine + 64) / (armies[1].numForwardUnits + 1);
	rowSeparation[1][1] = (centerLine + 64) / (armies[1].numRearUnits + 1);

	int scale[2][2] = { { 11, 4 }, { 21, 28 } };

	for(int a=0; a<2; ++a)
	{
		int index[2] = { 0, 0 };
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit *pUnit = &armies[a].units[b];
			int y = centerLine - 32 + (index[pUnit->row]++ + 1)*rowSeparation[a][pUnit->row];
			int offset = (a == 0 ? centerField - y : y - centerField);
			pUnit->SetPos(horizSpacing*scale[a][pUnit->row] + int(offset * (pUnit->row ? 0.3f : 0.1f)) - 32, y - 64 + 4);

			StartCooldown(pUnit);
		}
	}

	SetNext(this);
}

void Battle::Select()
{
	pInputManager->ClearReceivers();
	pInputManager->PushReceiver(this);
}

int Battle::CalculateDamage(BattleUnit *pUnit, BattleUnit *pTarget)
{
	UnitDetails *pDetails = pUnit->pUnit->GetDetails();
	UnitDetails *pTargetDetails = pTarget->pUnit->GetDetails();
	float damageMod = pUnitDefs->GetDamageModifier(pDetails->attackClass, pTargetDetails->defenceClass);

	int damage = (MFRand() % (pDetails->attackMax - pDetails->attackMin + 1)) + pDetails->attackMin;

	return (int)(damage * damageMod);
}

int Battle::Update()
{
	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];
			if(unit.stateTime)
			{
				unit.stateTime -= MFSystem_TimeDelta();

				if(unit.state == US_Engaging)
				{
					if(unit.stateTime <= 0.f)
					{
						unit.curX = unit.posX;
						unit.curY = unit.posY;

						int damage = CalculateDamage(&unit, unit.pTarget);

						if(unit.pTarget->pUnit->Damage(damage) == 0)
						{
							if(unit.pTarget->state == US_Waiting)
								EndWaiting(unit.pTarget);
							else if(unit.pTarget->state == US_Cooldown)
								StopCooldown(unit.pTarget);

							unit.pTarget->state = US_Dying;
							unit.pTarget->stateTime = 1.f;
						}

						unit.pTarget->bEngaged = false;
						unit.pTarget = NULL;

						StartCooldown(&unit);
					}
					else
					{
						float speed = unit.pUnit->GetDetails()->attackSpeed;
						float phaseFactor = speed * 0.25f;
						int phase = (int)((speed - unit.stateTime) / phaseFactor);
						float phaseTime = fmodf(speed - unit.stateTime, phaseFactor) / phaseFactor;

						if(1) // melee
						{
							switch(phase)
							{
								case 0:
									unit.curX = unit.posX;
									unit.curY = unit.posY + (int)MFClamp(-10.f, sinf(phaseTime * 4.f * MFPI) * -10.f, 0.f);
									break;
								case 1:
									unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*phaseTime);
									unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*phaseTime);
									break;
								case 2:
									unit.curX = unit.pTarget->posX + (unit.army == 0 ? -64 : 64 );
									unit.curY = unit.pTarget->posY;
									break;
								case 3:
									unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*(1.f-phaseTime));
									unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*(1.f-phaseTime));
									break;
							}
						}
						else if(0) // ranged
						{
							// shoot a projectile
						}
					}
				}
				else if(unit.state == US_Dying)
				{
					if(unit.stateTime <= 0.f)
					{
						unit.state = US_Dead;
						--armies[a].numUnitsAlive;
					}
				}
			}
		}
	}

	BattleUnit *pUnit;
	while(pUnit = CheckCooldown())
		AddWaiting(pUnit);

	pUnit = pActionHead;
	while(pUnit)
	{
		if(!pUnit->bEngaged && pUnit->damageIndicatorTime == 0.f)
		{
			Army &opponent = armies[1 - pUnit->army];
			bool bCanAttackBackRow = pUnit->pUnit->IsRanged() || opponent.numForwardUnits == 0;

			// pick target...
			BattleUnit *pTarget = NULL;
			for(int a=0; a<opponent.numUnits; ++a)
			{
				BattleUnit *pT = &opponent.units[a];
				if(!pT->bEngaged && !pT->damageIndicatorTime && pT->pUnit->GetHealth() && (pT->state == US_Cooldown || pT->state == US_Waiting) && (bCanAttackBackRow || pT->row == 0))
				{
					pTarget = pT;
					break;
				}
			}

			if(pTarget)
			{
				// attack!
				pUnit->pTarget = pTarget;
				pTarget->bEngaged = true;
				pUnit->state = US_Engaging;
				pUnit->stateTime = pUnit->pUnit->GetDetails()->attackSpeed;

				// unlink
				EndWaiting(pUnit);
			}
		}

		pUnit = pUnit->pNext;
	}

	return 0;
}

void Battle::Draw()
{
	MFView_Push();
	MFRect rect = { 0, 0, 1, 1 };
	MFView_SetOrtho(&rect);

	MFMaterial_SetMaterial(pBackground);
	MFPrimitive_DrawQuad(0, 0, 1, 0.5, MFVector::one, 0, 0, 1, 1);

	if(pCastle)
	{
		MFMaterial_SetMaterial(pCastle);
		MFPrimitive_DrawQuad(0, 0, 1, 0.5, MFVector::one, 0, 0, 1, 1);
	}

	MFMaterial_SetMaterial(pForeground);
	MFPrimitive_DrawQuad(0, 0, 1, 1, MFVector::one, 0, 0, 1, 1);

	MFRenderer_ClearScreen(CS_ZBuffer | CS_Stencil);

	// render units
	MFDisplay_GetDisplayRect(&rect);
	MFView_SetOrtho(&rect);

	float texelCenter = MFRenderer_GetTexelCenterOffset();

	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];

			if(unit.state != US_Dead)
				unit.pUnit->Draw((float)unit.curX, (float)unit.curY, a == 1, unit.state == US_Dying ? unit.stateTime : 1.f);
		}
	}

	pUnitDefs->DrawUnits(64.f, texelCenter);

	// health bars + damage indicators
	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];

			if(unit.state != US_Dying && unit.state != US_Dead)
			{
				float healthSize = MFClamp(12, unit.pUnit->GetDetails()->life, 32) * 2.f;
				float health = unit.pUnit->GetHealth() * healthSize;
				MFPrimitive_DrawUntexturedQuad(float(unit.curX + 31 - healthSize*0.5f), float(unit.curY - 8), healthSize + 2.f, 4.f, MFVector::black);
				MFPrimitive_DrawUntexturedQuad(float(unit.curX + 32 - healthSize*0.5f), float(unit.curY - 7), health, 2.f, MFVector::red);
			}

			if(unit.damageIndicatorTime)
			{
				// draw damage indicator with a little bouncey thing...
				//...
			}
		}
	}

	// timeline
	MFMaterial_SetMaterial(pIcons);

	int numQuads = 3 + cooldownCount;
	MFPrimitive_BeginBlitter(numQuads);

	int timelineY = (int)(rect.height / 6) - 16;

	int left = (int)(rect.width * 0.06f);
	int right = (int)rect.width - left;
	int width = (int)rect.width - left*2;

	MFPrimitive_Blit(left, timelineY, 0, 0, 32, 32);
	MFPrimitive_StretchBlit(left + 32, timelineY, width - 64, 32, 32, 0, 32, 32);
	MFPrimitive_Blit(right - 32, timelineY, 64, 0, 32, 32);

	// plot each unit on the timeline
	BattleUnit *pUnit = pCooldownTail;
	while(pUnit)
	{
		int offset = pUnit->pUnit->GetPlayer() == armies[0].player ? -1 : 1;
		int x = left + (int)(width*(pUnit->stateTime/7.0f));
		int y = timelineY + pUnit->cooldownOffset + offset*20;

		MFSetColour(MakeVector(pUnit->colour, MFClamp(0.f, (-pUnit->stateTime * 3.f) + 21, 1.f)));
		MFPrimitive_Blit(x, y, 0, 32, 32, 32);
		pUnit->pUnit->Draw(x-16.f, y-16.f);

		pUnit = pUnit->pPrev;
	}

	MFPrimitive_EndBlitter();

	// draw deferred unit heads
	pUnitDefs->DrawUnits(64.f, texelCenter, true);

	MFView_Pop();
}

void Battle::Deselect()
{
	// clean up the battle units?
}

bool Battle::HandleInputEvent(InputEvent ev, InputInfo &info)
{
	if(info.device == IDD_Mouse && info.deviceID != 0)
		return false;

	if(ev == IE_Tap)
	{
		if(armies[0].numUnitsAlive == 0 || armies[1].numUnitsAlive == 0)
		{
			// battle is finished
			pGame->EndBattle(armies[0].units[0].pGroup, armies[1].units[0].pGroup->GetTile());
		}
	}

	return false;
}

int Battle::UpdateInput()
{
	return 0;
}

void Battle::StartCooldown(BattleUnit *pUnit)
{
	pUnit->state = US_Cooldown;
	pUnit->stateTime = pUnit->pUnit->GetDetails()->cooldown;

	pUnit->cooldownOffset = 0; // y offset

	BattleUnit *pU = pCooldownTail;
	while(pU)
	{
		if(pU->state == US_Cooldown && pU->stateTime <= pUnit->stateTime)
		{
			pUnit->pPrev = pU;
			pUnit->pNext = pU->pNext;
			if(pU->pNext)
				pU->pNext->pPrev = pUnit;
			else
				pCooldownTail = pUnit;
			pU->pNext = pUnit;
			goto done;
		}

		pU = pU->pPrev;
	}

	// we must be the first unit in the list
	pUnit->pPrev = NULL;
	pUnit->pNext = pCooldownHead;
	if(pCooldownHead)
		pCooldownHead->pPrev = pUnit;
	else
		pCooldownTail = pUnit;
	pCooldownHead = pUnit;

done:
	++cooldownCount;
}

void Battle::StopCooldown(BattleUnit *pUnit)
{
	if(pCooldownHead == pUnit)
		pCooldownHead = pUnit->pNext;
	if(pCooldownTail == pUnit)
		pCooldownTail = pUnit->pPrev;
	if(pUnit->pNext)
		pUnit->pNext->pPrev = pUnit->pPrev;
	if(pUnit->pPrev)
		pUnit->pPrev->pNext = pUnit->pNext;
	pUnit->pPrev = pUnit->pNext = NULL;
	--cooldownCount;
}

Battle::BattleUnit *Battle::CheckCooldown()
{
	BattleUnit *pGet = NULL;

	if(pCooldownHead && pCooldownHead->stateTime <= 0.f)
	{
		pGet = pCooldownHead;

		pCooldownHead = pCooldownHead->pNext;
		if(pCooldownHead)
			pCooldownHead->pPrev = NULL;
		else
			pCooldownTail = NULL;

		pGet->pNext = pGet->pPrev = NULL;
		pGet->stateTime = 0.f;

		--cooldownCount;
	}

	return pGet;
}

void Battle::AddWaiting(BattleUnit *pUnit)
{
	pUnit->state = US_Waiting;
	pUnit->stateTime = 0.f;

	pUnit->pPrev = pActionTail;
	pUnit->pNext = NULL;
	if(pActionTail)
		pActionTail->pNext = pUnit;
	else
		pActionHead = pUnit;
	pActionTail = pUnit;
}

void Battle::EndWaiting(BattleUnit *pUnit)
{
	if(pActionHead == pUnit)
		pActionHead = pUnit->pNext;
	if(pActionTail == pUnit)
		pActionTail = pUnit->pPrev;
	if(pUnit->pNext)
		pUnit->pNext->pPrev = pUnit->pPrev;
	if(pUnit->pPrev)
		pUnit->pPrev->pNext = pUnit->pNext;
	pUnit->pPrev = pUnit->pNext = NULL;
}

void Battle::BattleUnit::Init(Unit *_pUnit)
{
	pUnit = _pUnit;

	state = US_Cooldown;
	stateTime = pUnit->GetDetails()->cooldown;

	pTarget = NULL;
	bEngaged = false;
	damageIndicatorTime = 0.f;

	posX = posY = 0;
	curX = curY = 0;
	cooldownOffset = 0;

	colour = pUnit->GetColour();

	pPrev = pNext = NULL;
}

void Battle::BattleUnit::SetPos(int x, int y)
{
	posX = curX = x;
	posY = curY = y;
}
