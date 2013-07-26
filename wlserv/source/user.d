module wlserv.user;

import vibe.data.json;

import std.digest.sha;
import std.datetime;
import std.string;

import wlserv.sync;
import wlserv.game;

std.random.Xorshift32 keygen;

__gshared User[uint] usersById;
__gshared User[string] usersByName;
__gshared uint biggestUser;

class User
{
    string newSession()
    {
        sessionKey = format("%x", keygen.front());
        keygen.popFront();

        return sessionKey;
    }

    Json toJson(bool bPrivate)
    {
        Json j = Json.EmptyObject;

        j.id = id;
        j.name = name;
        j.registrationDate = registration.toISOString();
        j.lastSeen = lastSeen.toISOString();
        j.won = won;
        j.lost = lost;

        if(bPrivate)
        {
            j.key = sessionKey;

            // games

            // friends
        }

        return j;
    }

    // static
    uint id;

    string name;
    ubyte[20] passwordHash;

    DateTime registration;
    int lost;
    int won;

    Game[] active;
    Game[] finished;

    User[] friends;
    User[] friendRequests;

    // transient
    string sessionKey;
    SysTime lastSeen;

    Lobby[] waiting;
}

ubyte[20] passwordHash(string user, string password)
{
    return sha1Of("warrior", password, "lords!", user.toLower);
}

User createUser(string username, string password)
{
    synchronized(userMutex)
    {
        if(getUserByName(username) !is null)
            return null;

        User user = new User();
        user.id = biggestUser++;
        user.name = username;
        user.passwordHash = passwordHash(username, password);

        user.registration = cast(DateTime)Clock.currTime();
        user.lastSeen = Clock.currTime();

        usersById[user.id] = user;
        usersByName[user.name] = user;

        return user;
    }
}

User getUser(string username, string password)
{
    User u = getUserByName(username);
    if(!u || u.passwordHash[] != passwordHash(username, password)[])
        return null;

    return u;
}

User getUserByName(string name)
{
    synchronized(userMutex)
        return usersByName.get(name, null);
}

User getUserById(uint id)
{
    synchronized(userMutex)
        return usersById.get(id, null);
}

static this()
{
    keygen.seed(std.random.unpredictableSeed());
}
