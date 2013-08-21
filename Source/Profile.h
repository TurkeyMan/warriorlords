#if !defined(_PROFILE_H)
#define _PROFILE_H

//#include "ServerRequest.h"

class MFJSONValue;

class Lobby;
class Game;

class Profile
{
public:
	static Profile *Get(uint32 id);
	static Profile *FromJson(MFJSONValue *pJson);

	uint32 ID() const { return id; }
	MFString Name() const { return name; }

	MFArray<Lobby*> &Pending() { return pending; }
	MFArray<GameState*> &Games() { return games; }

private:
	struct FriendRequest
	{
		uint32 id;
		MFString name;
	};

	struct Invite
	{
		uint32 game, player;
	};

	uint32 id;
	MFString name;

	uint32 created;

	int wins, losses;

	MFArray<Lobby*> pending;
	MFArray<GameState*> games;
	MFArray<uint32> finished;

	MFArray<Profile*> friends;
	MFArray<FriendRequest> friendRequesets;

	MFArray<Invite> invites;
};

#endif
