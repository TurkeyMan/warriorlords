#include "Warlords.h"
#include "Session.h"
#include "Profile.h"

#include <stdio.h>

Session *Session::pCurrent = NULL;

void Session::Set(Session *pSession)
{
	pCurrent = pSession;
}

uint32 Session::UserID() const
{
	return pUser->ID();
}

MFString Session::Username() const
{
	return pUser->Name();
}
