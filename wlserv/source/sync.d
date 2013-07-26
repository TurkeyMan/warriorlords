module wlserv.sync;

import core.sync.mutex;

Mutex userMutex;
Mutex gameMutex;

static this()
{
    userMutex = new Mutex;
    gameMutex = new Mutex;
}
