#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Session
{
public:
	typedef FastDelegate2<ServerError, Session *> SessionDelegate;

	static void InitSession();
	static void DeinitSession();

	Session();
	~Session();

	static void Update();

	void UpdateState();

	void Login(const char *pUsername, const char *pPassword);
	void Logout();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsLoggedIn() { return bLoggedIn; }
	uint32 GetUserID() { return user.id; }
	const char *GetUsername() { return user.userName; }

	static Session *Get() { return pCurrent; }

	bool IsIngame() { return bIngame; }
	GameState *GetActiveGame();
	GameDetails *GetActiveLobby();

	int GetNumCurrentGames() { return numCurrentGames; }
	int GetNumPendingGames() { return numPendingGames; }
	int GetNumPastGames() { return numPastGames; }

	void FindGames(MFString callback);

	GameState *GetCurrentGame(int game) { return &pCurrentGames[game]; }
	GameDetails *GetPendingGame(int game) { return &pPendingGames[game]; }
	GameDetails *GetPastGame(int game) { return &pPastGames[game]; }

	void SetLoginDelegate(SessionDelegate handler) { loginHandler = handler; }
	void SetUpdateDelegate(SessionDelegate handler) { updateHandler = handler; }

protected:
	UserDetails user;

	bool bLoggedIn;

	bool bLocalGame;
	bool bIngame;
	uint32 activeGame;

	int updating;

	SessionDelegate loginHandler;
	SessionDelegate updateHandler;

	uint32 currentGames[1024];
	GameState *pCurrentGames;
	int numCurrent;
	int numCurrentGames;
	uint32 pendingGames[1024];
	GameDetails *pPendingGames;
	int numPending;
	int numPendingGames;

	uint32 pastGames[1024];
	GameDetails *pPastGames;
	int numPastGames;

	// find game callback
	MFString findEvent;

	// manage these locally
	GameState offlineGames[64];
	uint32 localGames[64];
	int numLocalGames;

	// communication
	HTTPRequest login;
	HTTPRequest getCurrent;
	HTTPRequest getPending;
	HTTPRequest getPast;
	HTTPRequest search;

	static Session *pCurrent;

	void OnLogin(HTTPRequest::Status status);
	void OnGetCurrent(HTTPRequest::Status status);
	void OnGetPending(HTTPRequest::Status status);
	void OnGetPast(HTTPRequest::Status status);
	void OnGetCurrentGame(HTTPRequest::Status status);
	void OnGetPendingGame(HTTPRequest::Status status);
	void OnGamesFound(HTTPRequest::Status status);
};

#endif
