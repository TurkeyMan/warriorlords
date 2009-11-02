#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"

class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int race;
	int gold;
};

class Game
{
public:
	Game(const char *pMap);
	~Game();

	void BeginGame();
	void BeginTurn(int player);

	Map *GetMap() { return pMap; }
	UnitDefinitions *GetUnitDefs() { return pUnitDefs; }

	int CurrentPlayer() { return currentPlayer; }
	int GetPlayerRace(int player) { return players[player].race; }
	MFVector GetPlayerColour(int player) { return players[player].colour; }

protected:
	// game data
	Map *pMap;
	UnitDefinitions *pUnitDefs;

	Player players[8];
	int numPlayers;

	// screens
	MapScreen *pMapScreen;
	Battle *pBattle;

	// game state data
	int currentPlayer;

	// castles

	// units
};

#endif
