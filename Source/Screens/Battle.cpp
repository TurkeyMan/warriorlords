#include "Warlords.h"
#include "Battle.h"
#include "Unit.h"
#include "Group.h"
#include "Path.h"
#include "Display.h"

#include "Fuji/MFInput.h"
#include "Fuji/MFSystem.h"
#include "Fuji/MFPrimitive.h"
#include "Fuji/MFFont.h"
#include "Fuji/MFMaterial.h"
#include "Fuji/Materials/MFMat_Standard.h"
#include "Fuji/MFView.h"
#include "Fuji/MFRenderer.h"
#include "Fuji/MFTexture.h"

// remove me!
#include "Fuji/MFIni.h"

#include <stdio.h>

const float Battle::introLength = 4.f;

static bool gbBattleTest = false;
static int gTestWins[2] = {0, 0};
static int gBattle = 0;

void BeginTest(Game *pGame)
{
	// create groups and an artificial tile to fight on...
	Group *pGroup1 = Group::Create(0);
	Group *pGroup2 = Group::Create(1);
	MapTile tile[2];
	MFZeroMemory(&tile, sizeof(tile));
	tile[0].AddGroup(pGroup1);
	tile[1].AddGroup(pGroup2);

	// load units into the groups from the ini file
	MFIni *pIni = MFIni::Create("battle_test");
	MFIniLine *pLine = pIni->GetFirstLine();

	UnitDefinitions *pDefs = pGame->Map().UnitDefs();
	while(pLine)
	{
		if(pLine->IsSection("Group1") || pLine->IsSection("Group2"))
		{
			Group *pGroup = pLine->IsSection(gBattle ? "Group2" : "Group1") ? pGroup1 : pGroup2;

			MFIniLine *pG = pLine->Sub();
			while(pG)
			{
				if(pG->IsSection("Front") || pG->IsSection("Back"))
				{
					bool bFront = pG->IsSection("Front");

					MFIniLine *pUnits = pG->Sub();
					while(pUnits)
					{
						int unit = pDefs->FindUnit(pUnits->GetString(0));
						if(unit >= 0)
						{
							Unit *pUnit = pDefs->CreateUnit(unit, pGroup == pGroup1 ? gBattle : 1-gBattle, &pGame->State());

							int levels = pUnits->GetInt(1);
							for(int a=0; a<levels; ++a)
							{
								pUnit->AddVictory();
								pUnit->AddVictory();
							}

							if(bFront)
								pGroup->AddForwardUnit(pUnit);
							else
								pGroup->AddRearUnit(pUnit);
						}

						pUnits = pUnits->Next();
					}
				}

				pG = pG->Next();
			}
		}

		pLine = pLine->Next();
	}

	pGame->BeginBattle(pGroup1, &tile[1]);

	gBattle = 1 - gBattle;
}

void BattleTest()
{
	MFDebug_Assert(false, "!");

	gbBattleTest = true;
/*
	// create a game
	GameParams params;
	params.bEditMap = false;
	params.bOnline = false;
	params.gameID = 0;
	params.pMap = "Map_50x30x2";
	params.numPlayers = 2;
	params.players[0].id = 0;
	params.players[0].race = 1;
	params.players[0].colour = 1;
	params.players[0].hero = 0;
	params.players[1].id = 0;
	params.players[1].race = 2;
	params.players[1].colour = 2;
	params.players[1].hero = 0;
*/
	// start game
	Game *pGame = new Game(*(GameState*)NULL);
	BeginTest(pGame);
}

Battle::Battle(Game *pGame)
: pGame(pGame)
{
	// load lots of battle related stuff
	pIcons = MFMaterial_Create("BattleIcons");
	pAttack = MFMaterial_Create("Attack");
	pCloud = MFMaterial_Create("Cloud");
}

Battle::~Battle()
{
	MFMaterial_Release(pIcons);
	MFMaterial_Release(pAttack);
	MFMaterial_Release(pCloud);
}

void Battle::Begin(Group *pGroup, MapTile *pTarget)
{
	MFDebug_Log(0, "\nBattle Begins:");

	UnitDefinitions *pUnitDefs = pGame->Map().UnitDefs();

	fg = bg = -1;

	// load portraits
	int left = pGame->GetPlayerRace(pGroup->GetPlayer());
	int right = pGame->GetPlayerRace(pTarget->GetPlayer());
	pPortraits[0] = MFMaterial_Create(MFStr("Portrait-%s", pUnitDefs->GetRaceName(left)));
	pPortraits[1] = MFMaterial_Create(MFStr("Portrait-%s", pUnitDefs->GetRaceName(right)));

	introTime = 0.f;

	// find boat
	Unit *pBoat = pGroup->GetVehicle();
	if(!pBoat)
	{
		for(int a=0; a<pTarget->GetNumGroups(); ++a)
		{
			Group *pTargetGroup = pTarget->GetGroup(a);
			pBoat = pTargetGroup->GetVehicle();
			if(pBoat)
				break;
		}
	}

	if(pBoat && pBoat->GetTerrainPenalty(0) == 0 && pBoat->GetTerrainPenalty(1) > 0)
	{
		pBackground = MFMaterial_Create("Battle-Ship");
	}
	else
	{
		int t0, t1 = -1;

		pCastle = NULL;
		const Castle *pC = pTarget->GetCastle();
		if(!pC)
			pC = pGroup->GetTile()->GetCastle();

		if(pC)
		{
			t0 = t1 = DecodeTL(pC->GetTile()->GetTerrain());

			// load castle
			int race = pGame->GetPlayerRace(pC->GetPlayer());
			pCastle = MFMaterial_Create(MFStr("Castle-%s", pUnitDefs->GetRaceName(race)));
		}
		else
		{
			// find involved terrains
			MapTile *pT0 = pGroup->GetTile();
			MapTile *pT1 = pTarget;
			int x0 = pT0->GetX();
			int y0 = pT0->GetY();
			int x1 = pT1->GetX();
			int y1 = pT1->GetY();
			int terrain0 = pT0->GetTerrain();
			int terrain1 = pT1->GetTerrain();

			// swap the tiles to make it easier to search
			if(x0 > x1 || y0 > y1)
			{
				MapTile *pT = pT0;
				pT0 = pT1;
				pT1 = pT;
				x0 = pT0->GetX();
				y0 = pT0->GetY();
				x1 = pT1->GetX();
				y1 = pT1->GetY();
			}

			// find joining edges
			int others[4];
			if(x0 == x1)
			{
				t0 = DecodeBL(terrain0);
				int t = DecodeBR(terrain0);
				if(t != t0)
				{
					if(t0 == 1) // water
					{
						t1 = t0;
						t0 = t;
					}
					else
						t1 = t;
				}

				others[0] = DecodeTL(terrain0);
				others[1] = DecodeTR(terrain0);
				others[2] = DecodeBL(terrain1);
				others[3] = DecodeBR(terrain1);
			}
			else if(y0 == y1)
			{
				t0 = DecodeTR(terrain0);
				int t = DecodeBR(terrain0);
				if(t != t0)
				{
					if(t0 == 1) // water
					{
						t1 = t0;
						t0 = t;
					}
					else
						t1 = t;
				}

				others[0] = DecodeTL(terrain0);
				others[1] = DecodeBL(terrain0);
				others[2] = DecodeTR(terrain1);
				others[3] = DecodeBR(terrain1);
			}
			else if(x0 < x1 && y0 < y1)
			{
				t0 = DecodeBR(terrain0);

				others[0] = DecodeTR(terrain0);
				others[1] = DecodeBL(terrain0);
				others[2] = DecodeTR(terrain1);
				others[3] = DecodeBL(terrain1);
			}
			else
			{
				if(x0 < x1)
					t0 = DecodeTR(terrain0);
				else
					t0 = DecodeBL(terrain0);

				others[0] = DecodeTL(terrain0);
				others[1] = DecodeBR(terrain0);
				others[2] = DecodeTL(terrain1);
				others[3] = DecodeBR(terrain1);
			}

			// if we didn't resolve t1
			if(t1 == -1)
			{
				// count occurances of other terrain
				int t[4], cnt[4];
				int numOthers = 0;
				for(int a=0; a<4; ++a)
				{
					bool bFound = false;

					for(int b=0; b<numOthers; ++b)
					{
						if(t[b] == others[a])
						{
							++cnt[b];
							bFound = true;
							break;
						}
					}

					if(!bFound)
					{
						t[numOthers] = others[a];
						cnt[numOthers++] = 1;
					}
				}

				// select the one that was the most common
				int sel = 0;
				for(int a=1; a<numOthers; ++a)
				{
					if(t[sel] == t0 || cnt[a] > cnt[sel])
						sel = a;
				}

				t1 = t[sel];
			}
		}

		const Tileset &tileSet = pGame->Map().Tileset();
		fg = t0;
		bg = t1;

		// check for a bridge
		if((t0 == 1 || t1 == 1) && pTarget->IsRoad())
		{
			pBackground = MFMaterial_Create("Battle-Bridge");
		}
		else
		{
			// load background
			pBackground = MFMaterial_Create(MFStr("Battle-%s-%s", tileSet.TerrainName(t0), tileSet.TerrainName(t1)));
		}
	}

	// init stuff
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
			if(b < pTileGroup->GetNumForwardUnits() && armies[1].numUnits < 10)
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
			if(b < pTileGroup->GetNumRearUnits() && armies[1].numUnits < 10)
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

	// position the units
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

	// make some clouds
	for(int a=0; a<numClouds; ++a)
	{
		clouds[a].x = MFRand_Range(-64.f, (float)battleScreenWidth);
		clouds[a].y = MFRand_Range(40.f, 100.f);
		clouds[a].speed = MFRand_Range(4.f, 6.f);
	}

	SetNext(this);
}

void Battle::End()
{
	MFMaterial_Release(pBackground);
	MFMaterial_Release(pPortraits[0]);
	MFMaterial_Release(pPortraits[1]);

	if(pCastle)
	{
		MFMaterial_Release(pCastle);
		pCastle = NULL;
	}
}

void Battle::Select()
{
	pInputManager->PushReceiver(this);
}

int Battle::CalculateTargetPreference(BattleUnit *pUnit, BattleUnit *pTarget)
{
	// if the unit is set to wait for preferred target
	BattlePlan *pPlan = pUnit->pUnit->GetBattlePlan();
	if(pPlan->bAttackAvailable && (pTarget->bEngaged || pTarget->state >= US_Engaging))
		return 0; // reject this unit

	const UnitDetails &details = pUnit->pUnit->GetDetails();
	const UnitDetails targetDetails = pTarget->pUnit->GetDetails();
	float damageMod = pGame->Map().UnitDefs()->GetDamageModifier(details.attack, targetDetails.armour);

	int maxHP = pTarget->pUnit->GetMaxHP();

	int key1 = (int)(pTarget->pUnit->IsRanged() ? pPlan->type != TT_Melee : pPlan->type != TT_Ranged) << 30; // primary key is the preferred target unit rank
	int key2 = (int)(damageMod * (1.f/3.f) * 31.f) << 24; // secondary key is the damage modifier
	int key3 = pPlan->strength == TS_Weakest ? (-maxHP & 0x3FF) : (maxHP & 0x3FF); // final key is the preferred unit strength (weak or strong)

	return key1 | key2 | key3;
}

int Battle::CalculateDamage(BattleUnit *pUnit, BattleUnit *pTarget)
{
	const UnitDetails &details = pUnit->pUnit->GetDetails();
	const UnitDetails &targetDetails = pTarget->pUnit->GetDetails();

	// get armor class multiplier
	float damageMod = pGame->Map().UnitDefs()->GetDamageModifier(details.attack, targetDetails.armour);

	// get damage
	float damage = MFRand_Range(pUnit->pUnit->GetMinDamage(), pUnit->pUnit->GetMaxDamage()) * damageMod;

	// apply unit defence
	damage = pTarget->pUnit->GetDefence(damage, details.attack);

	return (int)damage;
}

void Battle::HitTarget(BattleUnit *pUnit, BattleUnit *pTarget)
{
	if(pUnit->army == pTarget->army)
	{
		float heal = pUnit->pUnit->GetSpecialAttack("heal");
		if(heal)
		{
			heal -= MFRand_Range(0.f, heal * 0.33f);

			pTarget->pUnit->Damage(-(int)heal);

			pTarget->damage = -(int)heal;
			pTarget->damageIndicatorTime = 1.f;
			pTarget->impactAnim = 4;

			MFDebug_Log(0, MFStr("%d:%s is healed %d life by %d:%s", pTarget->army, pTarget->pUnit->GetName(), (int)heal, pUnit->army, pUnit->pUnit->GetName()));

			pUnit->bHit = true;
			pTarget->bEngaged = false;
			return;
		}
	}

	int damage = CalculateDamage(pUnit, pTarget);

	pTarget->damage = damage;
	pTarget->damageIndicatorTime = 1.f;
	pTarget->impactAnim = pUnit->pUnit->GetWeapon().impactAnim;

	MFDebug_Log(0, MFStr("%d:%s is hit for %d damage by %d:%s", pTarget->army, pTarget->pUnit->GetName(), damage, pUnit->army, pUnit->pUnit->GetName()));

	if(pTarget->pUnit->Damage(damage) == 0)
	{
		if(pTarget->state == US_Waiting)
			EndWaiting(pTarget);
		else if(pTarget->state == US_Cooldown)
			StopCooldown(pTarget);

		pTarget->state = US_Dying;
		pTarget->stateTime = 1.f;

		MFDebug_Log(0, MFStr("%d:%s is dead", pTarget->army, pTarget->pUnit->GetName()));

		pUnit->pUnit->AddKill();
	}

	pUnit->bHit = true;
	pTarget->bEngaged = false;
}

int Battle::Update()
{
	float timeDelta = MFSystem_TimeDelta();
	if(gbBattleTest)
		timeDelta *= 4;

	introTime += timeDelta;

	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];

			unit.damageIndicatorTime -= timeDelta;
			if(unit.damageIndicatorTime < 0.f)
				unit.damageIndicatorTime = 0.f;

			if(unit.stateTime)
			{
				// this will pause the attacking player until their damage indicator disappears
				// remove this and let units attack while showing damage???
				if(unit.state != US_Engaging || unit.damageIndicatorTime == 0.f)
					unit.stateTime -= timeDelta;

				if(unit.state == US_Engaging)
				{
					if(unit.stateTime <= 0.f)
					{
						if(!unit.bHit)
							HitTarget(&unit, unit.pTarget);

						unit.curX = unit.posX;
						unit.curY = unit.posY;
						unit.bFiring = false;

						unit.pTarget = NULL;

						StartCooldown(&unit);
					}
					else
					{
						float speed = unit.pUnit->GetDetails().attackSpeed;

						const Weapon &weapon = unit.pUnit->GetWeapon();

						if(!weapon.bIsProjectile)
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
									break;
								case 3:
									unit.curX = unit.posX + (int)((unit.pTarget->posX + (unit.army == 0 ? -64 : 64 ) - unit.posX)*(1.f-phaseTime));
									unit.curY = unit.posY + (int)((unit.pTarget->posY - unit.posY)*(1.f-phaseTime));
									break;
							}

							if(phase >= 2 && !unit.bHit)
								HitTarget(&unit, unit.pTarget);
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
		if(!pUnit->bEngaged)
		{
			bool bIsRanged = pUnit->pUnit->IsRanged();
			if(bIsRanged || pUnit->row == 0 || armies[pUnit->army].numForwardUnits == 0)
			{
				Army &opponent = armies[1 - pUnit->army];
				bool bCanAttackBackRow = bIsRanged || opponent.numForwardUnits == 0;

				BattleUnit *pTarget = NULL;

				Army &army = armies[pUnit->army];
				float heal = pUnit->pUnit->GetSpecialAttack("heal");
				if(heal)
				{
					// find wounded
					float health = 1.f;
					int healTarget = -1;
					for(int a=0; a<army.numUnits; ++a)
					{
						BattleUnit &t = army.units[a];
						if(&t == pUnit || t.pUnit->IsDead())
							continue;

						float hp = t.pUnit->GetHealth();
						if(hp < health)
						{
							healTarget = a;
							health = hp;
						}
					}

					if(healTarget > -1 && health < 0.8f)
						pTarget = &army.units[healTarget];
				}

				if(!pTarget)
				{
					// pick attack target...
					int preference = 0;

					for(int a=0; a<opponent.numUnits; ++a)
					{
						BattleUnit &t = opponent.units[a];

						// check if we can attack this target
						if(t.pUnit->IsDead() || (t.row == 1 && !bCanAttackBackRow))
							continue;

						int pref = CalculateTargetPreference(pUnit, &t);

						if(!pTarget || pref > preference)
						{
							pTarget = &t;
							preference = pref;
						}
					}
				}

				if(pTarget && pTarget->CanBeAttacked())
				{
					// attack!
					pUnit->pTarget = pTarget;
					pTarget->bEngaged = true;
					pUnit->state = US_Engaging;
					pUnit->stateTime = pUnit->pUnit->GetDetails().attackSpeed;
					pUnit->bHit = false;

					// unlink
					EndWaiting(pUnit);

					const char *pTechnique = pUnit->army == pTarget->army ? "Heals" : "Attacks";
					MFDebug_Log(0, MFStr("%d:%s %s %d:%s", pUnit->army, pUnit->pUnit->GetName(), pTechnique, pTarget->army, pTarget->pUnit->GetName()));
				}
			}
			else if(!bIsRanged && pUnit->row == 1 && armies[pUnit->army].numForwardUnits < 5 && armies[pUnit->army].numForwardUnits > 0)
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
						pUnit->stateTime = 1.f;

						// unlink
						EndWaiting(pUnit);
					}
				}
			}
		}

		pUnit = pUnit->pNext;
	}

	for(int a=0; a<numClouds; ++a)
	{
		clouds[a].x -= timeDelta * clouds[a].speed;

		if(clouds[a].x < -64)
		{
			clouds[a].x = (float)battleScreenWidth;
			clouds[a].y = MFRand_Range(60.f, 180.f);
		}
	}

	if(gbBattleTest && (armies[0].numUnitsAlive == 0 || armies[1].numUnitsAlive == 0))
	{
		int victor = armies[0].numUnitsAlive ? 0 : 1;
		++gTestWins[victor ^ (1-gBattle)];
		BeginTest(pGame);
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
	UnitDefinitions *pUnitDefs = pGame->Map().UnitDefs();

	MFView_Push();
	MFMatrix orthoMat, screenMat;
	GetOrthoMatrix(&orthoMat, true);
	GetOrthoMatrix(&screenMat);
	MFView_SetCustomProjection(orthoMat, false);

	// calculate the offsets
	float texelCenter = MFRenderer_GetTexelCenterOffset();
	float texelOffsetW = 0.f, texelOffsetH = 0.f;

	
	int texWidth, texHeight;
	MFTexture *pTex = MFMaterial_GetParameterT(pIcons, MFMatStandard_Texture, MFMatStandard_Tex_DifuseMap);
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

	float screenWidth = (1.f / screenMat.m[0]) * 2.f;
	float screenHeight = (1.f / -screenMat.m[5]) * 2.f;

	// draw the water
	float waterW = 64.f/screenWidth;
	float waterH = 64.f/screenHeight;
	int waterX = (int)(screenWidth * (1.f/64.f)) + 1;
	int waterY = (int)(screenWidth * (0.75f/64.f)) + 1;
	float waterOffset = texelCenter / 64.f;

	int numWaterTiles = waterX * waterY;

	MFMaterial_SetMaterial(pGame->Map().Tileset().GetWaterMaterial());

	MFPrimitive(PT_QuadList);
	MFBegin(numWaterTiles*2);

	float wy = 0.25f;
	for(int a=0; a<waterY; ++a)
	{
		float wx = 0.f;
		for(int b=0; b<waterX; ++b)
		{
			MFSetTexCoord1(0 + waterOffset, 0 + waterOffset);
			MFSetPosition(wx, wy, 0);
			MFSetTexCoord1(1 + waterOffset, 1 + waterOffset);
			MFSetPosition(wx + waterW, wy + waterH, 0);
			wx += waterW;
		}
		wy += waterH;
	}

	MFEnd();

	// draw background+foreground
	MFMaterial_SetMaterial(pBackground);
	MFPrimitive_DrawQuad(0, 0, 1, 1, MFVector::white, 0.f + (.5f/800.f), 0.f + (0.5f/480.f), 1 + (.5f/800.f), 1 + (0.5f/480.f));

	// render units
	MFView_SetCustomProjection(screenMat, false);

	// draw clouds
	MFMaterial_SetMaterial(pCloud);

	MFPrimitive(PT_QuadList);
	MFBegin(numClouds*2);

	for(int a=0; a<numClouds; ++a)
	{
		MFSetTexCoord1(0.f, 0.f);
		MFSetPosition(clouds[a].x, clouds[a].y, 0);
		MFSetTexCoord1(1.f, 1.f);
		MFSetPosition(clouds[a].x + 64.f, clouds[a].y + 128.f, 0);
	}

	MFEnd();

	// draw the castle
	if(pCastle)
	{
		float hx = (1.f/512.f)*texelCenter;
		float hy = (1.f/256.f)*texelCenter;

		MFVector castleColour = armies[1].player > 0 ? pGame->GetPlayerColour(armies[1].player) : MFVector::white;

		MFMaterial_SetMaterial(pCastle);
		MFPrimitive_DrawQuad(screenWidth*0.5f - 256.f, screenHeight*0.5f - 256.f, 512.f, 256.f, castleColour, 0+hx, 0+hy, 1+hx, 1+hy);
	}

	// draw units
	renderUnits.clear();
	for(int a=0; a<2; ++a)
	{
		for(int b=0; b<armies[a].numUnits; ++b)
		{
			BattleUnit &unit = armies[a].units[b];
			int unitX = unit.bFiring ? unit.posX : unit.curX;
			int unitY = unit.bFiring ? unit.posY : unit.curY;

			if(unit.state != US_Dead)
			{
				renderUnits.push() = unit.pUnit->Render((float)unitX, (float)unitY, a == 1, unit.state == US_Dying ? unit.stateTime : 1.f);
			}
		}
	}

	pGame->DrawUnits(renderUnits, 64.f, texelCenter, false, true);

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
				const Weapon &weapon = unit.army == unit.pTarget->army ? pUnitDefs->GetWeapon(12) : unit.pUnit->GetWeapon();

				MFRect uvs;
				weapon.GetUVs(&uvs, fTexWidth, fTexHeight, unit.army == 1, texelOffsetW);

				MFMaterial_SetMaterial(pIcons);
				MFPrimitive_DrawQuad((float)(unit.curX - 16*weapon.width), (float)(unit.curY - 16*weapon.height), weapon.width*32.f, weapon.height*32.f, MFVector::one, uvs.x, uvs.y, uvs.x+uvs.width, uvs.y+uvs.height);
			}

			if(unit.state != US_Dying && unit.state != US_Dead)
				DrawHealthBar(unitX, unitY, unit.pUnit->GetMaxHP(), unit.pUnit->GetHealth());

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
				char damage[12];
				sprintf(damage, "%d", MFAbs(unit.damage));

				MFVector damageColour = unit.damage < 0 ? MFVector::blue : MFVector::red;
				damageColour.w = MFMin(unit.damageIndicatorTime*5.f, 1.f);

				float widths[12];
				float totalWidth = 0.f;
				int numDigits = MFString_Length(damage);

				// get the width of each character
				MFFont *pFont = pGame->GetBattleNumbersFont();
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
					MFFont_BlitText(pFont, x, unit.curY - 7 - (int)height - (int)bounce, damageColour, damage + a, 1);

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

	// draw intro
	if(introTime < introLength && !gbBattleTest)
	{
		MFView_Push();
		GetOrthoMatrix(&orthoMat, true);
		MFView_SetCustomProjection(orthoMat, false);

		const float slideTime = 0.5f;

		const float slideLength = introLength - 1.f;

		float t = MFMax(introTime > slideLength*0.5f ? slideLength - introTime : introTime, 0.f);
		float slide = MFMin(t, slideTime) / slideTime;

		MFVector c0 = pGame->GetPlayerColour(armies[0].player);
		MFVector c1 = armies[1].player > 0 ? pGame->GetPlayerColour(armies[1].player) : MFVector::white;

		const float halfTexel = 1.f/512.f * texelCenter;
		MFMaterial_SetMaterial(pPortraits[0]);
		MFPrimitive_DrawQuad((-448.f + slide*448.f) * (1.f/800.f), (480.f - 512.f) * (1.f/480.f), 448.f * (1.f/800.f), 512.f * (1.f/480.f), c0, 0.f+halfTexel, 0.f+halfTexel, 0.875f+halfTexel, 1.f+halfTexel);
		MFMaterial_SetMaterial(pPortraits[1]);
		MFPrimitive_DrawQuad((800.f - slide*448.f) * (1.f/800.f), (480.f - 512.f) * (1.f/480.f), 448.f * (1.f/800.f), 512.f * (1.f/480.f), c1, 0.875f-halfTexel, 0.f+halfTexel, 0.f-halfTexel, 1.f+halfTexel);

		MFView_Pop();
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
	renderUnits.clear();
	BattleUnit *pUnit = pCooldownTail;
	while(pUnit)
	{
		float offset = pUnit->pUnit->GetPlayer() == armies[0].player ? -1.f : 1.f;
		float x = left + MFFloor(width*(pUnit->stateTime/7.0f));
		float y = timelineY + pUnit->cooldownOffset + offset*20;

		MFPrimitive_DrawQuad(x, y, 32.f, 32.f, MakeVector(pUnit->colour, MFClamp(0.f, (-pUnit->stateTime * 3.f) + 21, 1.f)), 0.f, 0.25f, 0.25f, 0.5f);
		renderUnits.push() = pUnit->pUnit->Render(x-16.f, y-16.f);

		pUnit = pUnit->pPrev;
	}

	// draw deferred unit heads
	pGame->DrawUnits(renderUnits, 64.f, texelCenter, true);

	if(gbBattleTest)
		MFFont_BlitText(MFFont_GetDebugFont(), 10, 100, MFVector::white, MFStr("Wins: %d/%d %g%%", gTestWins[0], gTestWins[1], (float)gTestWins[0] / (float)(gTestWins[0] + gTestWins[1]) * 100.f));

	MFView_Pop();
}

void Battle::Deselect()
{
	pInputManager->PopReceiver(this);

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
			// add victories to battle units
			int victor = armies[0].numUnitsAlive ? 0 : 1;
			for(int a=0; a<armies[victor].numUnits; ++a)
			{
				if(!armies[victor].units[a].pUnit->IsDead())
					armies[victor].units[a].pUnit->AddVictory();
			}

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
	pUnit->stateTime = pUnit->pUnit->GetCooldown();

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
	stateTime = pUnit->GetCooldown();

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
