module wlserv.game;

import std.datetime;

import wlserv.sync;
import wlserv.user;

class Lobby
{
    struct Player
    {
        User player;
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
        l.map = map;
        l.players = new Player[numPlayers];
        l.addPlayer(user);
        return l;
    }

    bool addPlayer(User user)
    {
        foreach(p; players)
        {
            if(p.player is null)
            {
                p = Player(user, 0, firstUnusedColour(), 0, false);
                return true;
            }
        }

        return false;
    }

    bool removePlayer(User user)
    {
        foreach(p; players)
        {
            if(p.player == user)
            {
                p.player = null;
                return true;
            }
        }
        return false;
    }


private:
    int firstUnusedColour()
    {
        foreach(colour; 0..12)
            if(std.algorithm.find!"a.colour == b"(players, colour).length == 0)
                return colour;
        return -1;
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

    DateTime lastAction;

    Player[] users;
    string[] actions;
}

__gshared Lobby[uint] lobbies;
__gshared Game[uint] games;
