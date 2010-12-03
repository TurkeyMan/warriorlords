package warriorlordsserv;

import com.google.appengine.api.datastore.Key;
import java.util.ArrayList;
import java.util.Date;
import java.util.List;

import javax.jdo.PersistenceManager;
import javax.jdo.annotations.*;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Game
{
	public enum GameState
	{
		Playing,
		Finished
	}

	@PersistenceCapable
	public static class Player
	{
		@SuppressWarnings("unused")
		@PrimaryKey
		@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
		private Key key;

		@Persistent
		Long id;
		@Persistent
		int race;
		@Persistent
		int colour;
		@Persistent
		int team;
		@Persistent
		int hero;

		String name;
		
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
	}

	@PrimaryKey
	@Persistent
	private Long id;
	@Persistent
	private String name;
	@Persistent
	private String map;
	@Persistent
	int turnTime;
	@Persistent
	private ArrayList<Player> players;

	// game state
	@Persistent
	private GameState state;
	@Persistent
	private int currentPlayer;
	@Persistent
	private int currentTurn;
	@Persistent
	private Date turnStartTime;

	// game info
	@Persistent
	private Date startTime;
	@Persistent
	private Date endTime;
	@Persistent
	private long winner;

	// action stack
	@Persistent
	private ArrayList<Action> actions;

	private Integer timeRemaining;

	// game entities
//	@Persistent
//	private ArrayList<Unit> units;
//	@Persistent
//	private ArrayList<Group> groups;
//	@Persistent
//	private ArrayList<Castle> castles;
//	@Persistent
//	private ArrayList<Ruin> ruins;

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

	public int getTurnTime()
	{
		return turnTime;
	}

	public int getTurnTimeRemaining()
	{
		Date now = new Date();
		long diff = now.getTime() - turnStartTime.getTime();
		timeRemaining = (int)(diff / 1000); 
		return timeRemaining;
	}

	public int getNumPlayers()
	{
		return players.size();
	}

	public Player getPlayer(int index)
	{
		return players.get(index);
	}

	public GameState getState()
	{
		return state;
	}

	public int getCurrentPlayer()
	{
		return currentPlayer;
	}

	public int getCurrentTurn()
	{
		return currentTurn;
	}

	public Date getStartTime()
	{
		return startTime;
	}

	public Date getEndTime()
	{
		return endTime;
	}

	public long getWinner()
	{
		return winner;
	}

	protected Game(long id)
	{
		this.id = id;

		actions = new ArrayList<Action>();
//		units = new ArrayList<Unit>();
//		groups = new ArrayList<Group>();
	}

	static Game CreateGame(PersistenceManager pm, Lobby lobby, long[] players)
	{
		// create game
		Game game = new Game(lobby.getId());
		game.startTime = new Date();
		game.name = lobby.getName();
		game.map = lobby.getMap();
		game.turnTime = lobby.getTurnLength();
		
		// assign players
		int numPlayers = lobby.getNumPlayers();
		game.players = new ArrayList<Player>(numPlayers);
		for(int i=0; i<numPlayers; ++i)
		{
			Lobby.Player p = lobby.findPlayer(players[i]);
			Player newPlayer = new Player();
			newPlayer.id = p.getPlayer();
			newPlayer.race = p.getRace();
			newPlayer.colour = p.getColour();
			newPlayer.hero = p.getHero();
			newPlayer.team = i;
			game.players.add(newPlayer);

			try
			{
				PlayerAccount player = PlayerAccount.GetAccount(pm, newPlayer.id);
				player.BeginGame(lobby.getId(), game.getId());
			}
			catch(ServerException e)
			{
				// player can not begin game...
				// this game is probably broken
			}
		}

		// init game state
		game.currentTurn = 0;
		game.currentPlayer = 0;
		game.state = GameState.Playing;

		// begin the first turn
		game.BeginTurn(0);
		game.getTurnTimeRemaining();

		pm.makePersistent(game);
		return game;
	}

	public static Game GetGame(PersistenceManager pm, long gameID)
	{
		Game game  = pm.getObjectById(Game.class, gameID);
		game.getTurnTimeRemaining();
		return game;
	}

	public void DestroyGame(PersistenceManager pm)
	{
		// remove game from the players lists
		for(int a=0; a<players.size(); ++a)
		{
			try
			{
				PlayerAccount player = PlayerAccount.GetAccount(pm, players.get(a).id);
				player.DestroyGame(id);
			}
			catch(ServerException e)
			{
				// player doesn't seem to exist...
			}
		}

		pm.deletePersistent(this);
	}

	void BeginTurn(int player)
	{
		if(player < currentPlayer)
			++currentTurn;

		currentPlayer = player;
		turnStartTime = new Date();

		// work out how to signal players that it's their turn
		//...

		// email?
	}

	void EndGame(PersistenceManager pm)
	{
		endTime = new Date();
		state = GameState.Finished;

		// find winning player
		Player winningPlayer = players.get(currentPlayer);
		winner = winningPlayer.id;

		// update players profiles
		int winningTeam = winningPlayer.team;
		for(int i=0; i<players.size(); ++i)
		{
			Player p = players.get(i);
			try
			{
				PlayerAccount player = PlayerAccount.GetAccount(pm, p.id);
				player.FinishGame(id, p.team == winningTeam);
			}
			catch(ServerException e)
			{
				// wtf?!
			}
		}
	}

	public void CheckTimeout(PersistenceManager pm)
	{
		// check if the player has taken too long and forefit his turn
		Date now = new Date();
		long diff = now.getTime() - turnStartTime.getTime();
		if(diff >= turnTime * 60000)
		{
			int nextPlayer = (currentPlayer + 1) % players.size();
//			while(players.get(nextPlayer).alive == false)
//				nextPlayer = (nextPlayer + 1) % players.size();
			
			if(nextPlayer == currentPlayer)
				EndGame(pm);
			else
				BeginTurn(nextPlayer);
		}
	}

	public void AddAction(Action action)
	{
		actions.add(action);
	}

	public int GetNumActions()
	{
		return actions.size();
	}

	public List<Action> GetActions(int firstAction)
	{
		return actions.subList(firstAction, actions.size());
	}

	// action handlers
/*
	public void CreateCastles(int[] owners)
	{
		castles = new ArrayList<Castle>(owners.length);
		for(int a=0; a<owners.length; ++a)
			castles.add(new Castle(owners[a]));
	}

	public void CreateRuins(int[] items)
	{
		ruins = new ArrayList<Ruin>(items.length);
		for(int a=0; a<items.length; ++a)
			ruins.add(new Ruin(items[a]));
	}

	public void CreateUnit(int player, int type, int move, int life)
	{
		Unit unit = new Unit(player, type, move, life);
		units.add(unit);
	}

	public void CreateGroup(int player, int x, int y, int units[])
	{
		Group group = new Group(player, x, y, units);
		groups.add(group);
	}

	public void ClaimCastle(int castle, int player)
	{
		Castle c = castles.get(castle);
		c.player = player;
		c.building = 0;
		c.buildTime = 0;
	}

	public void SetBuilding(int castle, int unit, int time)
	{
		Castle c = castles.get(castle);
		c.building = unit;
		c.buildTime = time;
	}

	public void SearchRuin(int unit, int ruin)
	{
		Ruin r = ruins.get(ruin);
		if(!r.bCollected)
		{
			Unit u = units.get(unit);
			u.items.add(new Integer(r.item));
			r.bCollected = true;
		}
	}
*/
}

