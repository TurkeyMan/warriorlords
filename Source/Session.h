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
	void BeginOffline();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsActive() { return bLoggedIn || bOffline; }
	bool IsOffline() { return bOffline; }

	uint32 GetUserID() { return user.id; }
	const char *GetUsername() { return user.userName; }

	static Session *Get() { return pCurrent; }

	int GetNumCurrentGames() { return numCurrentGames; }
	int GetNumPendingGames() { return numPendingGames; }
	int GetNumPastGames() { return numPastGames; }

	GameState *GetCurrentGame(int game) { return &pCurrentGames[game]; }
	GameDetails *GetPendingGame(int game) { return &pPendingGames[game]; }
	GameDetails *GetPastGame(int game) { return &pPastGames[game]; }

	void SetLoginDelegate(SessionDelegate handler) { loginHandler = handler; }
	void SetUpdateDelegate(SessionDelegate handler) { updateHandler = handler; }

protected:
	static MFString GetOnline(uiEntity *pEntity);
	static MFString GetLoggedIn(uiEntity *pEntity);

	static void LoginAction(uiEntity *pEntity, uiRuntimeArgs *pArguments);
	void LoginActionComplete(HTTPRequest::Status status);

	UserDetails user;

	bool bLoggedIn;
	bool bOffline;

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

	// manage these locally
	uint32 localGames[64];
	int numLocalGames;

	// communication
	HTTPRequest login;
	HTTPRequest getCurrent;
	HTTPRequest getPending;
	HTTPRequest getPast;

	MFString responseAction;

	static Session *pCurrent;

	void OnLogin(HTTPRequest::Status status);
	void OnGetCurrent(HTTPRequest::Status status);
	void OnGetPending(HTTPRequest::Status status);
	void OnGetPast(HTTPRequest::Status status);
	void OnGetCurrentGame(HTTPRequest::Status status);
	void OnGetPendingGame(HTTPRequest::Status status);
};

#endif
