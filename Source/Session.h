#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Session
{
public:
	typedef FastDelegate2<ServerError, Session *> SessionDelegate;
	typedef FastDelegate3<ServerError, Session *, GameDetails *> JoinDelegate;
	typedef FastDelegate4<ServerError, Session *, GameLobby *, int> FindDelegate;
	typedef FastDelegate2<uint32, const char *> PeerMessageDelegate;

	static void InitSession();
	static void DeinitSession();

	Session();
	~Session();

	void Update();

	void UpdateGames();
	void UpdatePastGames();

	void Login(const char *pUsername, const char *pPassword);
	void Resume(uint32 id);
	void Logout();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsLoggedIn() const { return bLoggedIn; }
	uint32 GetUserID() const { return user.id; }
	const char *GetUsername() const { return user.userName; }

	static Session *Get() { return pCurrent; }

	bool IsIngame() const { return bIngame; }
	uint32 GetGameID() const;
	bool IsCreator() const;

	const GameState *GetActiveGame() const;
	GameDetails *GetActiveLobby() const;
	GameDetails::Player *Session::GetLobbyPlayer(uint32 user = -1, int *pPlayer = NULL) const;

	int GetNumCurrentGames() const { return currentGames.size(); }
	int GetNumPendingGames() const { return pendingGames.size(); }
	int GetNumPastGames() const { return pastGames.size(); }

	void FindGames(FindDelegate callback);
	void CreateGame(const GameCreateDetails &details, JoinDelegate callback);
	void JoinGame(MFString game, JoinDelegate callback);
	void LeaveGame(uint32 game, SessionDelegate callback);
	void MakeCurrent(uint32 game);
	void BeginGame(uint32 game, uint32 *pPlayers, int numPlayers);

	void SetRace(uint32 game, int race, SessionDelegate callback);
	void SetColour(uint32 game, int colour, SessionDelegate callback);
	void SetHero(uint32 game, int hero, SessionDelegate callback);

	GameState *GetCurrentGame(int game);
	GameDetails *GetPendingGame(int game);
	GameDetails *GetPastGame(int game);

	void SetLoginDelegate(SessionDelegate handler) { loginHandler = handler; }
	void SetUpdateDelegate(SessionDelegate handler) { updateHandler = handler; }
	void SetBeginDelegate(SessionDelegate handler) { beginHandler = handler; }

	void EnableRealtimeConnection(bool bEnabled) { bKeepConnection = bEnabled; }
	void SendMessageToPeers(const char* pBuffer);
	void SetMessageCallback(PeerMessageDelegate handler) { peerMessageHandler = handler; }

protected:
	enum ConnectionStage
	{
		Disconnected = 0,
		Connecting,
		Authenticating,
		Connected
	};

	void CloseConnection();

	UserDetails user;

	bool bLoggedIn;
	int updating;

	SessionDelegate loginHandler;
	SessionDelegate updateHandler;
	SessionDelegate beginHandler;
	SessionDelegate leaveHandler;
	SessionDelegate setRaceHandler;
	SessionDelegate setColourHandler;
	SessionDelegate setHeroHandler;
	JoinDelegate createHandler;
	JoinDelegate joinHandler;
	FindDelegate findHandler;

	// game lists
	struct ActiveGame
	{
		uint32 id;
		bool bLoaded;
		GameState state;
	};
	struct PendingGame
	{
		uint32 id;
		bool bLoaded;
		GameDetails details;
	};

	bool bLocalGame;
	bool bIngame;
	int activeGame;

	MFArray<ActiveGame> currentGames;
	MFArray<PendingGame> pendingGames;
	MFArray<PendingGame> pastGames;

	uint32 localGames[64]; // is ID
	MFArray<ActiveGame> offlineGames;

	// find game callback
	int joiningStep;
	GameDetails joinGame;

	// realtime thread
	bool bKeepConnection;
	MFSocket connection;
	ConnectionStage connectionStage;

	MFSocketAddressInet serverAddress;
	bool bAddressKnown;

	PeerMessageDelegate peerMessageHandler;
	MFArray<MFString> pendingMessages;

	// communication
	HTTPRequest login;
	HTTPRequest getCurrent;
	HTTPRequest getPending;
	HTTPRequest getPast;
	HTTPRequest search;
	HTTPRequest create;
	HTTPRequest join;
	HTTPRequest setRace;
	HTTPRequest setColour;
	HTTPRequest setHero;
	HTTPRequest begin;
	HTTPRequest leave;

	int setRaceValue, setColourValue, setHeroValue;

	static Session *pCurrent;

	void OnLogin(HTTPRequest::Status status);
	void OnGetCurrent(HTTPRequest::Status status);
	void OnGetPending(HTTPRequest::Status status);
	void OnGetPast(HTTPRequest::Status status);
	void OnGetCurrentGame(HTTPRequest::Status status);
	void OnGetPendingGame(HTTPRequest::Status status);
	void OnGamesFound(HTTPRequest::Status status);
	void OnCreate(HTTPRequest::Status status);
	void OnJoined(HTTPRequest::Status status);
	void OnRaceSet(HTTPRequest::Status status);
	void OnColourSet(HTTPRequest::Status status);
	void OnHeroSet(HTTPRequest::Status status);
	void OnBegin(HTTPRequest::Status status);
	void OnLeave(HTTPRequest::Status status);
};

#endif
