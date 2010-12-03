package warriorlordsserv;

import com.google.appengine.api.datastore.Key;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;
import javax.jdo.PersistenceManager;
import javax.jdo.annotations.*;
import javax.jdo.Query;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Lobby
{
	@PersistenceCapable
	public static class Player
	{
		@SuppressWarnings("unused")
		@PrimaryKey
		@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
		private Key key;

		@Persistent
		private Long id;

		private String name;

		@Persistent
		private int race;
		@Persistent
		private int colour;
		@Persistent
		private int hero;

		public long getPlayer()
		{
			return id;
		}

		public int getRace()
		{
			return race;
		}

		public int getColour()
		{
			return colour;
		}

		public int getHero()
		{
			return hero;
		}

		public String getName(PersistenceManager pm)
		{
			try
			{
				PlayerAccount account = PlayerAccount.GetAccount(pm, id);
				name = account.getAccountName();
			}
			catch(ServerException e)
			{
				name = null;
			}
			return name;
		}

		public Player(long id)
		{
			this.id = id;
			race = 1;
			colour = 1;
			hero = 0;
		}

		public void SetRace(int race)
		{
			this.race = race;
		}

		public void SetColour(int colour)
		{
			this.colour = colour;
		}

		public void SetHero(int hero)
		{
			this.hero = hero;
		}
	}

	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Long id;
	@Persistent
	private Date dateCreated;
	@Persistent
	private String name;
	@Persistent
	private long creator;
	@Persistent
	private String map;
	@Persistent
	private int maxPlayers;
	@Persistent
	private int turnTime;
	@Persistent
	private ArrayList<Player> players;

	public long getId()
	{
		return id;
	}

	public String getName()
	{
		return name;
	}

	public String getMap()
	{
		return map;
	}

	public String getAccountCreationDate()
	{
		return dateCreated.toString();
	}

	public int getMaxPlayers()
	{
		return maxPlayers;
	}

	public int getNumPlayers()
	{
		return players.size();
	}

	public Player getPlayer(int index)
	{
		return players.get(index);
	}
	
	public Player findPlayer(long playerId)
	{
		for(int a=0; a<players.size(); ++a)
		{
			Player player = players.get(a);
			if(playerId == player.getPlayer())
				return player;
		}

		return null;
	}

	public int getTurnLength()
	{
		return turnTime;
	}

	protected Lobby()
	{
		dateCreated = new Date();
		players = new ArrayList<Player>();
	}

	public static Lobby CreateGame(PersistenceManager pm, String name, String map, int maxPlayers, long creator, int turnTime) throws ServerException
	{
		if(name == null || map == null)
			throw new ServerException(Error.INVALID_ARGUMENTS);

		if(FindGame(pm, name) != null)
			throw new ServerException(Error.INVALID_GAME);

		PlayerAccount player = PlayerAccount.GetAccount(pm, creator);

		Lobby game = new Lobby();
		game.name = name;
		game.map = map;
		game.maxPlayers = maxPlayers;
		game.creator = creator;
		game.turnTime = turnTime;

		game.players.add(new Player(creator));

		pm.makePersistent(game);

		// add the game to the player 
		player.AddGame(game.getId());

		return game;
	}

	public static Lobby GetGame(PersistenceManager pm, long game) throws ServerException
	{
		Lobby lobby = pm.getObjectById(Lobby.class, game);
		if(lobby == null)
			throw new ServerException(Error.INVALID_GAME);
		return lobby;
	}

	@SuppressWarnings("unchecked")
	public static Lobby FindGame(PersistenceManager pm, String gameName)
	{
		Query query = pm.newQuery("SELECT FROM " + Lobby.class.getName() + " WHERE name=='" + gameName + "'");

		List<Lobby> lobby = new ArrayList<Lobby>();
		lobby = (List<Lobby>)query.execute();

		Lobby game = null;
		if(lobby.size() == 1)
			game = lobby.get(0);	

		return game;
	}

	public static Lobby FindRandomGame(PlayerAccount player)
	{

		return null;
	}

	public Game BeginGame(PersistenceManager pm, long[] players)
	{
		Game game = Game.CreateGame(pm, this, players);

		if(game != null)
			Delete(pm);

		return game;
	}

	public Error JoinGame(PersistenceManager pm, PlayerAccount player)
	{
		if(players.size() >= maxPlayers)
			return Error.GAME_FULL;

		long playerID = player.getId();

		// find if player is already in the game...
		for(int i=0; i<players.size(); ++i)
		{
			Player p = players.get(i);

			if(p.getPlayer() == playerID)
				return Error.ALREADY_PRESENT;
		}

		// add the player to the game
		players.add(new Player(playerID));
		player.AddGame(id);

		return Error.NO_ERROR;
	}

	public Error LeaveGame(PersistenceManager pm, PlayerAccount player)
	{
		long playerID = player.getId();

		// if the host leaves the game, the game is destroyed
		if(playerID == creator)
		{
			Delete(pm);
			return Error.NO_ERROR;
		}

		// find the player
		for(int i=0; i<players.size(); ++i)
		{
			Player p = players.get(i);

			// if the player is in the game
			if(p.getPlayer() == playerID)
			{
				// remove from the game
				players.remove(i);
				player.RemoveGame(id);

				return Error.NO_ERROR;
			}
		}

		return Error.NOT_IN_GAME;
	}

	public Player FindPlayer(long playerID)
	{
		// find the player
		for(int i=0; i<players.size(); ++i)
		{
			Player player = players.get(i);

			// if the player is in the game
			if(player.getPlayer() == playerID)
				return player;
		}
		return null;
	}

	public Error SetPlayerRace(long playerID, int race)
	{
		Player player = FindPlayer(playerID);

		if(player == null)
			return Error.NOT_IN_GAME;

		// set the player race
		player.SetRace(race);
		return Error.NO_ERROR;
	}

	public Error SetPlayerColour(long playerID, int colour)
	{
		Player player = FindPlayer(playerID);
		
		if(player == null)
			return Error.NOT_IN_GAME;

		// set the player race
		player.SetColour(colour);
		return Error.NO_ERROR;
	}

	public Error SetPlayerHero(long playerID, int hero)
	{
		Player player = FindPlayer(playerID);
		
		if(player == null)
			return Error.NOT_IN_GAME;

		// set the player race
		player.SetHero(hero);
		return Error.NO_ERROR;
	}

	public void Delete(PersistenceManager pm)
	{
		pm.deletePersistent(this);
	}
}
