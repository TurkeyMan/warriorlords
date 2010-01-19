#if !defined(_GAME_H)
#define _GAME_H

#include "Map.h"
#include "Unit.h"

struct MFFont;

class MapScreen;
class Battle;

struct Player
{
	MFVector colour;
	int race;
	int gold;

	int cursorX, cursorY;
};

class Game
{
public:
	Game(const char *pMap, bool bEditable = false);
	~Game();

	MapScreen *GetMapScreen() { return pMapScreen; }
	Battle *GetBattleScreen() { return pBattle; }

	void BeginGame();
	void BeginTurn(int player);
	void EndTurn();
	void BeginBattle(Group *pGroup, MapTile *pTarget);
	void EndBattle(Group *pGroup, MapTile *pTarget);

	Map *GetMap() { return pMap; }
	UnitDefinitions *GetUnitDefs() { return pUnitDefs; }

	int GetCurrentTurn() { return currentTurn; }
	int CurrentPlayer() { return currentPlayer; }
	int GetPlayerRace(int player) { return player == -1 ? 0 : players[player].race; }
	MFVector GetPlayerColour(int player) { return player == -1 ? pUnitDefs->GetRaceColour(0) : players[player].colour; }

	static void SetCurrent(Game *pGame) { pCurrent = pGame; }
	static Game *GetCurrent() { return pCurrent; }

	Group *CreateUnit(int unit, Castle *pCastle);

	MFFont *GetTextFont() { return pText; }
	MFFont *GetBattleNumbersFont() { return pBattleNumbersFont; }
	MFFont *GetSmallNumbersFont() { return pSmallNumbersFont; }

protected:
	// game resources
	MFFont *pText;
	MFFont *pBattleNumbersFont;
	MFFont *pSmallNumbersFont;

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
	int currentTurn;

	// castles

	// units

	static Game *pCurrent;
};

#endif
