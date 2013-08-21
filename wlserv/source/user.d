module wlserv.user;

import vibe.data.json;

import std.datetime;
import std.algorithm;
import std.string;
import std.digest.sha;

import wlserv.helper;
import wlserv.sync;
import wlserv.game;

std.random.Xorshift32 keygen;

__gshared User[uint] usersById;
__gshared User[string] usersByName;
__gshared uint biggestUser;

class User
{
    struct Invitation
    {
        Lobby game;
        User user;
    }

    string newSession()
    {
        sessionKey = format("%x", keygen.front());
        keygen.popFront();

        return sessionKey;
    }

    void friendRequest(User friend)
    {
        friendRequests ~= friend;
    }

    void acceptFriend(User friend)
    {
        // did this friend actually make a request?
        auto f = find(friendRequests, friend);
        wlAssert(f.length > 0, "no friend request");

        // remove it from the pending list
        friendRequests = friendRequests[0 .. $-f.length] ~ f[1 .. $];

        // connect friends
        friends ~= friend;
        friend.friends ~= this;
    }

    void invite(Lobby game, User friend)
    {
        invites ~= Invitation(game, friend);
    }

    Json toJson(bool bPrivate = false)
    {
        Json j = Json.EmptyObject;

        j.id = id;
        j.name = name;
        j.registrationdate = registration.toISOString();
        j.lastseen = lastSeen.toISOString();
        j.won = won;
        j.lost = lost;

        if(bPrivate)
        {
            j.key = sessionKey;

            // friends
            Json[] arr;
            foreach(f; friends)
                arr ~= f.toJson(false);
            j.friends = arr;

            // friend requests
            arr = null;
            foreach(fr; friendRequests)
            {
                Json req = Json.EmptyObject;
                req.id = fr.id;
                req.name = fr.name;
            }
            j.friendrequests = arr;

            // invites
            arr = null;
            foreach(i; invites)
            {
                Json invite = Json.EmptyObject;
                invite.friend = i.user.id;
                invite.game = i.game.id;
                arr ~= invite;
            }
            j.invites = arr;

            // games
            arr = null;
            foreach(p; pending)
                arr ~= p.toJson();
            j.pending = arr;

            // games
            arr = null;
            foreach(a; active)
                arr ~= a.toJson();
            j.games = arr;

            // finished
            arr = null;
            foreach(f; finished)
                arr ~= Json(f.id);
            j.finished = arr;
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

    Lobby[] pending;
    Game[] active;
    Game[] finished;

    User[] friends;

    User[] friendRequests;
    Invitation[] invites;

    // transient
    string sessionKey;
    SysTime lastSeen;
}

ubyte[20] passwordHash(string user, string password)
{
    return sha1Of("warrior", password, "lords!", user.toLower);
}

User createUser(string username, string password, string email)
{
    synchronized(userMutex)
    {
        if(username in usersByName)
            throw new WLServException("already exists");

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

User authenticate(string username, string password)
{
    User u = getUserByName(username);
    if(u.passwordHash[] != passwordHash(username, password)[])
        throw new WLServException("incorrect password");
    u.lastSeen = Clock.currTime();
    return u;
}

User getUserByName(string name)
{
    synchronized(userMutex)
    {
        if(name !in usersByName)
            throw new WLServException("user does not exist");
        return usersByName[name];
    }
}

User getUserById(uint id)
{
    synchronized(userMutex)
    {
        if(id !in usersById)
            throw new WLServException("user does not exist");
        return usersById[id];
    }
}

Json allUsers()
{
    Json[] users;
    foreach(u; usersById)
        users ~= u.toJson(true);
    return Json(users);
}

static this()
{
    keygen.seed(std.random.unpredictableSeed());
}
