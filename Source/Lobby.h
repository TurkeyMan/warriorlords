#if !defined(_LOBBY_H)
#define _LOBBY_H

class MFJSONValue;
class MapTemplate;

class Lobby
{
public:
	struct Player
	{
		Profile *pUser;
		int race;
		int colour;
		int hero;
		bool bReady;
	};

	static Lobby *Get(uint32 id);
	static Lobby *FromJson(MFJSONValue *pJson);

	Lobby();
	Lobby(MFString map);
	~Lobby();

	uint32 ID() const { return id; }

	bool Online() const { return id != 0; }

	int NumPlayers() const { return players.size(); }
	MFArray<Player>& Players() { return players; }
	Player& Players(int player) { return players[player]; }

	void LoadMap();
	MapTemplate& Map() { LoadMap(); return *pMap; }

private:
	uint32 id;
	MFString map;

	MapTemplate *pMap;

	MFArray<Player> players;

	MFArray<MFString> conversation;
};

#endif
