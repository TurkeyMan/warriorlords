#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Session
{
public:
	Session();
	~Session();

	ServerError Login(const char *pUsername, const char *pPassword);
	void BeginOffline();

	void LoadSession(uint32 user);
	void SaveSession();

	bool IsActive() { return bLoggedIn || bOffline; }
	bool IsOffline() { return bOffline; }

	uint32 GetUserID() { return user.id; }
	const char *GetUsername() { return user.userName; }

	static Session *SetCurrent(Session *pNew) { Session *pOld = pCurrent; pCurrent = pNew; return pOld; }
	static Session *GetCurrent() { return pCurrent; }

protected:
	UserDetails user;

	bool bLoggedIn;
	bool bOffline;

	uint32 currentGames[64];
	int numCurrentGames;
	uint32 pastGames[1024];
	int numPastGames;
	uint32 pendingGames[32];
	int numPendingGames;

	// manage these locally
	uint32 localGames[64];
	int numLocalGames;

	static Session *pCurrent;
};

#endif
