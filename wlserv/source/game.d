module wlserv.game;

import vibe.data.json;

import std.datetime;
import std.algorithm;

import wlserv.helper;
import wlserv.sync;
import wlserv.user;

__gshared Lobby[uint] lobbies;
__gshared Game[uint] games;
__gshared uint biggestGame;

class Lobby
{
    struct Player
    {
        User user;
        int race;
        int colour;
        int hero;
        bool bReady;
    }

    uint id;

    string map;
    Player[] players;

    string[] conversation;

    static Lobby create(string map, int numPlayers, User user)
    {
        Lobby l = new Lobby();
        l.id = biggestGame++;
        l.map = map;
        l.players = new Player[numPlayers];
        l.addPlayer(user);

        lobbies[l.id] = l;

        return l;
    }

    void addPlayer(User user)
    {
        wlAssert(!isInGame(user), "already in game");

        // check if there is room
        auto f = find!"a.user == b"(players, null);
        wlAssert(f.length > 0, "game full");

        // add player to game
        f[0] = Player(user, 0, firstUnusedColour(), 0, false);
        user.pending ~= this;

        // if the player was invited to this game, remove the invitation
        auto f2 = find!"a.game == b"(user.invites, this);
        if(f2.length > 0)
            user.invites = user.invites[0 .. $-f2.length] ~ f2[1 .. $];
    }

    void removePlayer(User user)
    {
        auto f = find!"a.user == b"(players, user);
        wlAssert(f.length > 0, "not in game");

        f[0].user = null;

        auto f2 = find(user.pending, this);
        if(f2.length > 0)
            user.pending = user.pending[0 .. $-f2.length] ~ f2[1 .. $];
    }

    Game configPlayer(User user, int race, int colour, int hero, bool bReady)
    {
        auto f = find!"a.user == b"(players, user);
        wlAssert(f.length > 0, "not in game");

        f[0].race = race;
        if(!anyoneIsColour(colour))
            f[0].colour = colour;
        f[0].hero = hero;
        f[0].bReady = bReady;

        // check begin
        foreach(p2; players)
            if(!p2.user || !p2.bReady)
                return null;

        // all players are ready
        return beginGame();
    }

    Game beginGame()
    {
        Game game = new Game();
        game.id = id;
        game.lastAction = Clock.currTime();

        foreach(p; players)
        {
            Game.Player player;
            player.user = p.user;
            game.players ~= player;

            // remove lobby from pending list
            auto f = find(p.user.pending, this);
            if(f.length > 0)
                p.user.pending = p.user.pending[0 .. $-f.length] ~ f[1 .. $];

            // add game to active list
            p.user.active ~= game;
        }

        games[id] = game;
        lobbies.remove(id);

        return game;
    }

    Json toJson()
    {
        Json j = Json.EmptyObject;
        j.id = id;
        j.map = map;

        Json[] arr;
        foreach(p; players)
        {
            Json player = Json.EmptyObject;
            player.id = p.user.id;
            player.name = p.user.name;
            player.race = p.race;
            player.colour = p.colour;
            player.hero = p.hero;
            player.ready = p.bReady;
            arr ~= player;
        }
        j.players = arr;

        arr = null;
        foreach(line; conversation)
            arr ~= Json(line);
        j.conversation = arr;

        return j;
    }

private:
    bool isInGame(User user)
    {
        return canFind!"a.user == b"(players, user);
    }

    int firstUnusedColour()
    {
        foreach(colour; 0..12)
            if(!anyoneIsColour(colour))
                return colour;
        return -1;
    }

    int anyoneIsColour(int colour)
    {
        return !canFind!"a.colour == b"(players, colour);
    }
}

class Game
{
    struct Player
    {
        User user;
        uint lastAction;
    }

    uint id;

    SysTime lastAction;

    Player[] players;
    string[] actions;

    void commit(string actions)
    {
        string[] lines = std.string.splitLines(actions);
        this.actions ~= lines;
    }

    Json toJson(int start = 0, int end = -1, bool onlyActions = false)
    {
        Json j = Json.EmptyObject;
        j.id = id;

        Json[] arr;
        if(!onlyActions)
        {
            foreach(p; players)
            {
                Json player = Json.EmptyObject;
                player.id = p.user.id;
                player.name = p.user.name;
                arr ~= player;
            }
            j.players = arr;
        }

        if(end == -1)
            end = actions.length;
        arr = null;
        foreach(line; actions[start .. end])
            arr ~= Json(line);
        j.actions = arr;

        return j;
    }
}

Lobby getLobby(uint id)
{
    wlAssert(!!(id in lobbies), "game does not exist");
    return lobbies[id];
}

Game getGame(uint id)
{
    wlAssert(!!(id in games), "game does not exist");
    return games[id];
}

bool isPlayerInGame(Game game, User player)
{
    return canFind!"a.user == b"(game.players, player);
}

Json allLobbies()
{
    Json[] arr;
    foreach(l; lobbies)
        arr ~= l.toJson();
    return Json(arr);
}

Json allGames()
{
    Json[] arr;
    foreach(g; games)
        arr ~= g.toJson();
    return Json(arr);
}
