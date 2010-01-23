#include "Warlords.h"
#include "Battle.h"
#include "Unit.h"
#include "Path.h"
#include "Display.h"

#include "MFInput.h"
#include "MFSystem.h"
#include "MFPrimitive.h"
#include "MFFont.h"
#include "MFMaterial.h"
#include "MFView.h"
#include "MFRenderer.h"
#include "MFTexture.h"

// remove me!
#include "MFIni.h"

#include <stdio.h>

Battle::Battle(Game *_pGame)
{
	pGame = _pGame;

	// load lots of battle related stuff
	pIcons = MFMaterial_Create("BattleIcons");
	pAttack = MFMaterial_Create("Attack");

	pTileSet = pGame->GetMap()->GetTileset();
	int numTerrains = pTileSet->GetNumTerrainTypes();
	ppForegrounds = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numTerrains);
	ppBackgrounds = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numTerrains);
	for(int a=0; a<numTerrains; ++a)
	{
		ppForegrounds[a] = MFMaterial_Create(MFStr("fg-%s", pTileSet->GetTerrainName(a)));
		ppBackgrounds[a] = MFMaterial_Create(MFStr("bg-%s", pTileSet->GetTerrainName(a)));
	}

	pUnitDefs = pGame->GetUnitDefs();
	int numRaces = pUnitDefs->GetNumRaces();
	ppCastles = (MFMaterial**)MFHeap_AllocAndZero(sizeof(MFMaterial*)*numRaces);
	for(int a=0; a<numRaces; ++a)
	{
		ppCastles[a] = MFMaterial_Create(MFStr("castle-%s", pUnitDefs->GetRaceName(a)));
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

	// initialise the battle units
	MFZeroMemory(armies, sizeof(armies));
	MFZeroMemory(armies, sizeof(armies));

	MFRect rect;
	GetDisplayRect(&rect);

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
				if(armies[1].numForwardUnits < 5)
				{
					pUnit->row = 0;
					++armies[1].numForwardUnits;
				}
				else
				{
					pUnit->row = 1;
					++armies[1].numRearUnits;
				}
			}
			if(b < pTileGroup->GetNumRearUnits())
			{
				BattleUnit *pUnit = &armies[1].units[armies[1].numUnits++];
				pUnit->Init(pTileGroup->GetRearUnit(b));
				pUnit->pGroup = pTileGroup;
				pUnit->army = 1;
				if(armies[1].numRearUnits < 5)
				{
					pUnit->row = 1;
					++armies[1].numRearUnits;
				}
				else
				{
					pUnit->row = 0;
					++armies[1].numForwardUnits;
				}
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

void Battle::HitTarget(BattleUnit *pUnit, BattleUnit *pTarget)
{
	int damage = CalculateDamage(pUnit, pTarget);

	pTarget->damage = damage;
	pTarget->damageIndicatorTime = 1.f;
	pTarget->impactAnim = pUnit->pUnit->GetWeapon()->impactAnim;

	if(pTarget->pUnit->Damage(damage) == 0)
	{
		if(pTarget->state == US_Waiting)
			EndWaiting(pTarget);
		else if(pTarget->state == US_Cooldown)
			StopCooldown(pTarget);

		pTarget->state = US_Dying;
		pTarget->stateTime = 1.f;
	}

	pTarget->bEngaged = false;
}

int Battle::Update()
{
	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];

			unit.damageIndicatorTime -= MFSystem_TimeDelta();
			if(unit.damageIndicatorTime < 0.f)
				unit.damageIndicatorTime = 0.f;

			if(unit.stateTime)
			{
				unit.stateTime -= MFSystem_TimeDelta();

				if(unit.state == US_Engaging)
				{
					if(unit.stateTime <= 0.f)
					{
						if(unit.bFiring)
							HitTarget(&unit, unit.pTarget);

						unit.curX = unit.posX;
						unit.curY = unit.posY;
						unit.bFiring = false;

						unit.pTarget = NULL;

						StartCooldown(&unit);
					}
					else
					{
						float speed = unit.pUnit->GetDetails()->attackSpeed;

						Weapon *pWeapon = unit.pUnit->GetWeapon();

						if(!pWeapon->bIsProjectile)
						{
							float phaseFactor = speed * 0.25f;
							int phase = (int)((speed - unit.stateTime) / phaseFactor);
							float phaseTime = fmodf(speed - unit.stateTime, phaseFactor) / phaseFactor;

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

									if(unit.pTarget->bEngaged)
										HitTarget(&unit, unit.pTarget);
									break;
								case 3:
									unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*(1.f-phaseTime));
									unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*(1.f-phaseTime));
									break;
							}
						}
						else
						{
							float phaseFactor = speed * 0.5f;
							int phase = (int)((speed - unit.stateTime) / phaseFactor);
							float phaseTime = fmodf(speed - unit.stateTime, phaseFactor) / phaseFactor;

							switch(phase)
							{
								case 0:
									unit.curX = unit.posX;
									unit.curY = unit.posY + (int)MFClamp(-10.f, sinf(phaseTime * 4.f * MFPI) * -10.f, 0.f);
									break;
								case 1:
									// work out the projectile angle 
									unit.curX = unit.posX + 32 + (int)((unit.pTarget->posX - unit.posX)*phaseTime);
									unit.curY = unit.posY + 32 + (int)((unit.pTarget->posY - unit.posY)*phaseTime);
									unit.bFiring = true;
									break;
							}
						}
					}
				}
				else if(unit.state == US_Dying)
				{
					if(unit.stateTime <= 0.f)
					{
						unit.state = US_Dead;
						--armies[a].numUnitsAlive;
						if(unit.row == 0)
							--armies[a].numForwardUnits;
						else
							--armies[a].numRearUnits;

					}
				}
				else if(unit.state == US_Migrating)
				{
					if(unit.stateTime <= 0.f)
					{
						// swap positions with target
						unit.curX = unit.posX = unit.pTarget->posX;
						unit.curY = unit.posY = unit.pTarget->posY;
						unit.row = 0;
						unit.pTarget = NULL;

						StartCooldown(&unit);
					}
					else
					{
						float phaseTime = 1.f - unit.stateTime*0.5f;
						unit.curX = unit.posX + (int)((unit.pTarget->posX - unit.posX)*phaseTime);
						unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*phaseTime);
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
			bool bIsRanged = pUnit->pUnit->IsRanged();
			if(bIsRanged || pUnit->row == 0 || armies[pUnit->army].numForwardUnits == 0)
			{
				Army &opponent = armies[1 - pUnit->army];
				bool bCanAttackBackRow = bIsRanged || opponent.numForwardUnits == 0;

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
			else if(!bIsRanged && pUnit->row == 1 && armies[pUnit->army].numForwardUnits < 5)
			{
				Army &army = armies[pUnit->army];
				for(int a=0; a<army.numUnits; ++a)
				{
					if(army.units[a].state == US_Dead && army.units[a].row == 0 && !army.units[a].bEngaged)
					{
						// migrate!
						pUnit->pTarget = &army.units[a];
						pUnit->pTarget->bEngaged = true;
						pUnit->state = US_Migrating;
						pUnit->stateTime = 2.f;

						// unlink
						EndWaiting(pUnit);
					}
				}
			}
		}

		pUnit = pUnit->pNext;
	}

	return 0;
}

void DrawHealthBar(int x, int y, int maxHealth, float currentHealth)
{
	float healthSize = MFClamp(12, maxHealth, 32) * 2.f;
	float health = currentHealth * healthSize;
	MFPrimitive_DrawUntexturedQuad(float(x + 31 - healthSize*0.5f), float(y - 8), healthSize + 2.f, 4.f, MFVector::black);
	MFPrimitive_DrawUntexturedQuad(float(x + 32 - healthSize*0.5f), float(y - 7), health, 2.f, MFVector::red);
}

void Battle::Draw()
{
	MFView_Push();
	MFMatrix orthoMat;
	GetOrthoMatrix(&orthoMat, true);
	MFView_SetCustomProjection(orthoMat, false);

	// calculate the offsets
	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float texelOffsetW = 0.f, texelOffsetH = 0.f;

	MFTexture *pTex;
	int texWidth, texHeight;
	int diffuse = MFMaterial_GetParameterIndexFromName(pIcons, "diffuseMap");
	MFMaterial_GetParameter(pIcons, diffuse, 0, &pTex);
	if(pTex)
	{
		MFTexture_GetTextureDimensions(pTex, &texWidth, &texHeight);
		texelOffsetW = texelCenter / (float)texWidth;
		texelOffsetH = texelCenter / (float)texHeight;
	}

	float fTexWidth = (float)texWidth;
	float fTexHeight = (float)texHeight;
	float xTileScale = 32.f / fTexWidth;
	float yTileScale = 32.f / fTexHeight;

	// draw background+foreground
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
	GetOrthoMatrix(&orthoMat);
	MFView_SetCustomProjection(orthoMat, false);

	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];
			int unitX = unit.bFiring ? unit.posX : unit.curX;
			int unitY = unit.bFiring ? unit.posY : unit.curY;

			if(unit.state != US_Dead)
			{
				unit.pUnit->Draw((float)unitX, (float)unitY, a == 1, unit.state == US_Dying ? unit.stateTime : 1.f);
			}
		}
	}

	pUnitDefs->DrawUnits(64.f, texelCenter);

	// health bars + damage indicators
	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];
			int unitX = unit.bFiring ? unit.posX : unit.curX;
			int unitY = unit.bFiring ? unit.posY : unit.curY;

			if(unit.bFiring)
			{
				Weapon *pWeapon = unit.pUnit->GetWeapon();

				MFRect uvs;
				pWeapon->GetUVs(&uvs, fTexWidth, fTexHeight, unit.army == 1, texelOffsetW);

				MFMaterial_SetMaterial(pIcons);
				MFPrimitive_DrawQuad((float)(unit.curX - 16*pWeapon->width), (float)(unit.curY - 16*pWeapon->height), pWeapon->width*32.f, pWeapon->height*32.f, MFVector::one, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
			}

			if(unit.state != US_Dying && unit.state != US_Dead)
				DrawHealthBar(unitX, unitY, unit.pUnit->GetDetails()->life, unit.pUnit->GetHealth());

			if(unit.damageIndicatorTime > 0.f)
			{
				// draw attack animation
				float time = (1.f-unit.damageIndicatorTime) * 2.f;
				if(time < 1.f)
				{
					float frame = MFFloor(time * 8.f) * (1.f/8.f) + (texelCenter*(1.f/512.f));
					float anim = (float)unit.impactAnim * (1.f/8.f) + (texelCenter*(1.f/512.f));

					MFMaterial_SetMaterial(pAttack);
					MFPrimitive_DrawQuad((float)unit.curX, (float)unit.curY, 64.f, 64.f, MFVector::one, frame, anim, frame + (1.f/8.f), anim + (1.f/8.f));
				}

				// draw damage indicator with a little bouncey thing...
				char damage[8];
				sprintf(damage, "%d", unit.damage);

				float widths[4];
				float totalWidth = 0.f;
				int numDigits = MFString_Length(damage);

				// get the width of each character
				MFFont *pFont = Game::GetCurrent()->GetBattleNumbersFont();
				float height = MFFont_GetFontHeight(pFont);

				for(int a=0; a<numDigits; ++a)
				{
					widths[a] = MFFont_GetStringWidth(pFont, damage + a, height, 0.f, 1);
					totalWidth += widths[a];
				}

				// render each character separately
				int x = unit.curX + 32 - (int)(totalWidth*0.5f);

				for(int a=0; a<numDigits; ++a)
				{
					// stagger appearance of the characters
					time = unit.damageIndicatorTime + (float)a*0.04f;
					if(time > 1.f)
						break;

					float phase = (1.f - time)*MFPI*4;
					float bounce = MFAbs(MFSin(phase)) * MFMax(time - 0.5f, 0.f)*50.f;
					MFFont_BlitText(pFont, x, unit.curY - 7 - (int)height - (int)bounce, MakeVector(MFVector::red, MFMin(unit.damageIndicatorTime*5.f, 1.f)), damage + a, 1);

					x += (int)widths[a];
				}
			}

#if 0
			if(unit.state != US_Dead)
			{
				char move[16];
				sprintf(move, "%d %.2g", unit.state, unit.stateTime);

				MFFont *pFont = Game::GetCurrent()->GetSmallNumbersFont();
				float height = MFFont_GetFontHeight(pFont);

				MFFont_BlitText(pFont, unitX + 2, unitY + 62 - (int)height, MFVector::white, move);
			}
#endif
		}
	}

	// timeline
	MFRect rect;
	GetDisplayRect(&rect);
	float timelineY = MFFloor(rect.height / 6) - 16;

	float left = MFFloor(rect.width * 0.06f);
	float right = rect.width - left;
	float width = rect.width - left*2;

	MFMaterial_SetMaterial(pIcons);
	MFPrimitive_DrawQuad(left, timelineY, 32.f, 32.f, MFVector::one, texelOffsetW, texelOffsetH, 0.25f + texelOffsetW, 0.25f + texelOffsetH);
	MFPrimitive_DrawQuad(left + 32, timelineY, width - 64, 32.f, MFVector::one, 0.25f + texelOffsetW, texelOffsetH, 0.5f + texelOffsetW, 0.25f + texelOffsetH);
	MFPrimitive_DrawQuad(right - 32.f, timelineY, 32.f, 32.f, MFVector::one, 0.5f + texelOffsetW, texelOffsetH, 0.75f + texelOffsetW, 0.25f + texelOffsetH);

	// plot each unit on the timeline
	BattleUnit *pUnit = pCooldownTail;
	while(pUnit)
	{
		float offset = pUnit->pUnit->GetPlayer() == armies[0].player ? -1.f : 1.f;
		float x = left + MFFloor(width*(pUnit->stateTime/7.0f));
		float y = timelineY + pUnit->cooldownOffset + offset*20;

		MFPrimitive_DrawQuad(x, y, 32.f, 32.f, MakeVector(pUnit->colour, MFClamp(0.f, (-pUnit->stateTime * 3.f) + 21, 1.f)), 0.f, 0.25f, 0.25f, 0.5f);
		pUnit->pUnit->Draw(x-16.f, y-16.f);

		pUnit = pUnit->pPrev;
	}

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
