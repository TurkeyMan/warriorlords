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

	void Begin(Group *pGroup, MapTile *pTarget, int foreground, int background, int castle = -1);

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

		Group *pGroup;
		Unit *pUnit;
		int army, row;

		int state;
		float stateTime;

		// engaging state
		BattleUnit *pTarget;
		bool bEngaged;
		bool bFiring;
		float damageIndicatorTime; // timeout for showing the damage indicator

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

	// members
	Game *pGame;

	UnitDefinitions *pUnitDefs;
	Tileset *pTileSet;

	Army armies[2];

	BattleUnit *pCooldownHead, *pCooldownTail;
	BattleUnit *pActionHead, *pActionTail;
	int cooldownCount;

	MFMaterial *pIcons;

	MFMaterial *pForeground;
	MFMaterial *pBackground;
	MFMaterial *pCastle;

	// library
	MFMaterial **ppForegrounds;
	MFMaterial **ppBackgrounds;
	MFMaterial **ppCastles;
};

#endif
