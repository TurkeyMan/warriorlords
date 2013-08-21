#if !defined(_SESSION_H)
#define _SESSION_H

#include "ServerRequest.h"

class Profile;

class Session
{
public:
	Session(Profile *pUser, MFString key)
	: pUser(pUser)
	, sessionKey(key)
	{}

	~Session() {}

	static Session *Get() { return pCurrent; }
	static void Set(Session *pSession);

	Profile *User() const { return pUser; }
	MFString SessionKey() const { return sessionKey; }

	uint32 UserID() const;
	MFString Username() const;

protected:
	Profile *pUser;
	MFString sessionKey;

	static Session *pCurrent;
};

#endif
