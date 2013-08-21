#include "Warlords.h"
#include "Profile.h"
#include "Lobby.h"

#include "Fuji/MFDocumentJSON.h"

#include <map>

std::map<uint32, Profile*> userMap;

Profile *Profile::Get(uint32 id)
{
	return userMap[id];
}

Profile *Profile::FromJson(MFJSONValue *pJson)
{
	MFJSONValue *pID = pJson->Member("id");
	uint32 id = pID->Int();

	Profile *pProfile = Profile::Get(id);
	if(!pProfile)
	{
		pProfile = new Profile();
		pProfile->id = id;

		userMap[id] = pProfile;
	}

	MFJSONValue *pName = pJson->Member("name");
	MFJSONValue *pRegDate = pJson->Member("registrationdate");
	MFJSONValue *pLastSeen = pJson->Member("lastseen");
	MFJSONValue *pWon = pJson->Member("won");
	MFJSONValue *pLost = pJson->Member("lost");

	MFJSONValue *pFriends = pJson->Member("friends");
	MFJSONValue *pFriendRequests = pJson->Member("friendrequests");
	MFJSONValue *pInvites = pJson->Member("invites");

	MFJSONValue *pPending = pJson->Member("pending");
	MFJSONValue *pGames = pJson->Member("games");
	MFJSONValue *pFinished = pJson->Member("finished");

	pProfile->name = pName->String();
//	pProfile-> = pRegDate->String();
//	pProfile-> = pLastSeen->String();
	pProfile->wins = pWon->Int();
	pProfile->losses = pLost->Int();

	for(size_t a=0; a<pFriends->Length(); ++a)
	{
		Profile *pUser = Profile::FromJson(pFriends->At(a));
		pProfile->friends.push(pUser);
	}

	for(size_t a=0; a<pFriendRequests->Length(); ++a)
	{
		MFJSONValue *pFR = pFriendRequests->At(a);

		auto &f = pProfile->friendRequesets.push();
		f.id = pFR->Member("id")->Int();
		f.name = pFR->Member("name")->String();
	}

	for(size_t a=0; a<pInvites->Length(); ++a)
	{
		MFJSONValue *pInvite = pInvites->At(a);

		auto &i = pProfile->invites.push();
		i.game = pInvite->Member("game")->Int();
		i.player = pInvite->Member("friend")->Int();
	}

	for(size_t a=0; a<pPending->Length(); ++a)
	{
		Lobby *pLobby = Lobby::FromJson(pPending->At(a));
		pProfile->pending.push(pLobby);
	}

	for(size_t a=0; a<pGames->Length(); ++a)
	{
		GameState *pGame = GameState::FromJson(pGames->At(a));
		pProfile->games.push(pGame);
	}

	for(size_t a=0; a<pFinished->Length(); ++a)
	{
		pProfile->finished.push(pFinished->At(a)->Int());
	}

	return pProfile;
}
