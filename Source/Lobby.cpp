#include "Warlords.h"
#include "Lobby.h"

#include "Fuji/MFDocumentJSON.h"

#include <map>

std::map<uint32, Lobby*> lobbyMap;

Lobby *Lobby::Get(uint32 id)
{
	return lobbyMap[id];
}

Lobby *Lobby::FromJson(MFJSONValue *pJson)
{
	MFJSONValue *pID = pJson->Member("id");
	uint32 id = pID->Int();

	Lobby *pLobby = Lobby::Get(id);
	if(!pLobby)
	{
		pLobby = new Lobby();
		pLobby->id = id;

		lobbyMap[id] = pLobby;
	}

	MFJSONValue *pMap = pJson->Member("map");
	MFJSONValue *pPlayers = pJson->Member("players");
	MFJSONValue *pConversation = pJson->Member("conversation");

/*
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
*/

	return pLobby;
}

Lobby::Lobby()
: id(0)
, pMap(NULL)
{
}

Lobby::Lobby(MFString map)
: id(0)
, map(map)
{
	pMap = MapTemplate::Create(map);

	for(int a=0; a<pMap->NumPlayersPresent(); ++a)
	{
		Player &player = players.push();
		player.pUser = NULL;
		player.colour = 1 + a;
		player.race = 0;
		player.hero = -1;
		player.bReady = true;
	}
}

Lobby::~Lobby()
{
	if(pMap)
		pMap->Release();
}

void Lobby::LoadMap()
{
	if(!pMap)
		pMap = MapTemplate::Create(map);
}
