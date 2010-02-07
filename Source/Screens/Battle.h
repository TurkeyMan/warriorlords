#if !defined(_BATTLE_H)
#define _BATTLE_H

#include "MFMaterial.h"

#include "Screen.h"
#include "Unit.h"

class Game;
class Group;
class Tileset;

class Battle : public Screen
{
public:
	Battle(Game *pGame);
	virtual ~Battle();

	void Begin(Group *pGroup, MapTile *pTarget);
	void End();

	virtual int Update();
	virtual void Draw();
	virtual void Deselect();

	virtual bool HandleInputEvent(InputEvent ev, InputInfo &info);
	virtual int UpdateInput();

protected:
	enum UnitState
	{
		US_Cooldown,        // unit is waiting for cooldown to expire
		US_Waiting,         // unit is waiting for target to be available
		US_Engaging,        // unit is engaging a unit
		US_Migrating,		// unit is migrating to the front row
		US_Dying,           // unit is dying
		US_Dead				// unit is dead
	};

	struct BattleUnit
	{
		void Init(Unit *pUnit);
		void SetPos(int x, int y);

		bool CanBeAttacked() { return !bEngaged && !pUnit->IsDead() && (state == US_Cooldown || state == US_Waiting); }

		Group *pGroup;
		Unit *pUnit;
		int army, row;

		int state;
		float stateTime;

		// engaging state
		BattleUnit *pTarget;
		bool bEngaged;
		bool bFiring;
		bool bHit;

		float damageIndicatorTime; // timeout for showing the damage indicator
		int damage;
		int impactAnim;

		int posX, posY;
		int curX, curY;
		int cooldownOffset;

		MFVector colour;

		BattleUnit *pPrev, *pNext;
	};

	struct Army
	{
		BattleUnit units[10];
		int player;
		int numUnits;
		int numForwardUnits, numRearUnits;
		int numUnitsAlive;
	};

	// member functions
	virtual void Select();

	void StartCooldown(BattleUnit *pUnit);
	void StopCooldown(BattleUnit *pUnit);
	BattleUnit *CheckCooldown();

	void AddWaiting(BattleUnit *pUnit);
	void EndWaiting(BattleUnit *pUnit);

	int CalculateDamage(BattleUnit *pUnit, BattleUnit *pTarget);
	void HitTarget(BattleUnit *pUnit, BattleUnit *pTarget);

	// members
	static const int battleScreenWidth = 800;
	static const int battleScreenHeight = 480;

	Game *pGame;

	UnitDefinitions *pUnitDefs;
	Tileset *pTileSet;

	Army armies[2];

	BattleUnit *pCooldownHead, *pCooldownTail;
	BattleUnit *pActionHead, *pActionTail;
	int cooldownCount;

	MFMaterial *pIcons;
	MFMaterial *pAttack;
	MFMaterial *pBackground;
	MFMaterial *pCastle;
	MFMaterial *pCloud;

	int fg, bg;

	static const int numClouds = 4;
	struct Cloud
	{
		float x, y;
		float speed;
	} clouds[numClouds];
};

#endif
