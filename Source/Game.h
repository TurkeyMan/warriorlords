#pragma once
#if !defined(_GAME_H)
#define _GAME_H

#include "GameState.h"

#include "Menu/Game/GameUI.h"
#include "MapView.h"

#include "ServerRequest.h"

#include "Fuji/MFObjectPool.h"

// *** REMOVE ME ***
#include "Screens/GroupConfig.h"

struct MFFont;
class MFJSONValue;

class Profile;
class Lobby;

class MapScreen;
class Battle;

class Game
{
	friend class GameUI;
	friend class MapScreen;
	friend class History;
public:
	static Game *CreateEditor(MFString map);

	Game(GameState &state);
	~Game();

	uint32 ID() const							{ return state.ID(); }

	int NumPlayers() const						{ return state.Players().size(); }
	MFArray<::Player>& Players()				{ return state.Players(); }
	::Player& Player(int player)				{ return state.Player(player); }
	const ::Player& Player(int player) const	{ return state.Player(player); }

	void Show();

	void Update();
	void Draw();

	void SetInputSource(HKWidget *pWidget);
	GameUI *GetUI()								{ return pGameUI; }

	void ShowMiniMap();

	MapScreen *GetMapScreen()					{ return pMapScreen; }
	Battle *GetBattleScreen()					{ return pBattle; }

	void BeginTurn(int player);
	void EndTurn();
	void BeginBattle(Group *pGroup, MapTile *pTarget);
	void EndBattle(Group *pGroup, MapTile *pTarget);

	GameState &State()							{ return state; }
	Map& Map()									{ return state.Map(); }

//	bool IsOnline() const						{ return state.IsOnline(); }

	int CurrentRound() const					{ return state.CurrentRound(); }
	int CurrentPlayer() const					{ return state.CurrentPlayer(); }
//	bool IsCurrentPlayer(int player) const		{ return state.IsCurrentPlayer(player); }
	bool IsMyTurn() const						{ return state.IsMyTurn(); }

	int GetPlayerRace(int player)				{ return state.PlayerRace(player); }
	MFVector GetPlayerColour(int player)		{ return state.PlayerColour(player); }
//	int GetPlayerHeroCount(int player)			{ return Player(player).numHeroes; }
	Unit *GetPlayerHero(int player, int hero)	{ return state.PlayerHero(player, hero); }
	bool PlayerHasHero(int player, int hero)	{ return state.PlayerHasHero(player, hero); }

	bool MoveGroupToTile(Group *pGroup, MapTile *pTile);

	void SelectGroup(Group *pGroup);
	Group *GetSelected()						{ return pSelection; }

	void PushMoveAction(Group *pGroup);
	void UpdateMoveAction(Group *pGroup);
	void PushRegroup(PendingAction::Regroup *pRegroup);

	Group *RevertAction(Group *pGroup);
	void CommitPending(Group *pGroup);
	void CommitAllActions();
	void DestroyAction(Action *pAction);

	inline void CaptureUnits(Group *pUnits)		{ state.History().PushCaptureUnits(pUnits); }

	void DrawWindow(const MFRect &rect);
	void DrawLine(float sx, float sy, float dx, float dy);

	MFFont *GetTextFont()						{ return pText; }
	MFFont *GetBattleNumbersFont()				{ return pBattleNumbersFont; }
	MFFont *GetSmallNumbersFont()				{ return pSmallNumbersFont; }

	void ShowRequest(const char *pMessage, GameUI::MsgBoxDelegate callback, bool bNotification);

	void DrawUnits(const MFArray<UnitRender> &units, float scale, float texelOffset, bool bHead = false, bool bRank = false);

protected:
	int GetHeroForRace(int race, int heroIndex);

	bool HandleInputEvent(HKInputManager &manager, const HKInputManager::EventInfo &ev);

	void UpdateUndoButton();

	int NextPlayer();

//	::Player* GetPlayer(uint32 user, int *pPlayer);
//	void ReceivePeerMessage(uint32 user, const char *pMessage);

	void CommitAction(PendingAction *pAction);
	void PopPending(PendingAction *pAction);
	void DisconnectAction(PendingAction *pAction, PendingAction *pFrom);

	// rendering
	int DrawCastle(int race);
	int DrawFlag(int race);
	int DrawSpecial(int index);

	// game resources
	MFFont *pText;
	MFFont *pBattleNumbersFont;
	MFFont *pSmallNumbersFont;

	MFMaterial *pWindow;
	MFMaterial *pHorizLine;

	// game state
	GameState &state;

	// rendering
	MFArray<UnitRender> renderUnits;

	// UI
	MapView mapView;

	GameUI *pGameUI;

	GroupConfig groupConfig;

	// UI data
	Group *pSelection;

	bool bMoving;
	float countdown;

	MFMaterial *pIcons;

	// screens
	MapScreen *pMapScreen;
	Battle *pBattle;

	// pending (undo) action pool
	MFObjectPool pendingPool;
};

#endif
