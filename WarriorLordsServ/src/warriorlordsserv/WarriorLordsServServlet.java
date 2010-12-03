package warriorlordsserv;

import java.io.IOException;
import java.io.PrintWriter;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.Map.Entry;

import javax.jdo.PersistenceManager;
import javax.servlet.http.*;

import com.google.gson.Gson;
import com.google.gson.GsonBuilder;
import com.google.gson.JsonArray;
import com.google.gson.JsonElement;
import com.google.gson.JsonObject;

public class WarriorLordsServServlet extends HttpServlet
{
	private static final long serialVersionUID = -5770711379674269748L;

	enum Request
	{
		ILLEGAL_REQUEST,

		CREATEACCOUNT,
		LOGIN,
		GETUSER,
		FORGOTACCOUNT,
		RESETPASSWORD,
		CHANGEPASSWORD,
		GETACTIVEGAMES,
		GETPASTGAMES,
		GETWAITINGGAMES,
		RESETPROFILE,
		CREATEGAME,
		DESTROYGAME,
		GETGAME,
		JOINGAME,
		FINDGAME,
		LEAVEGAME,
		SETRACE,
		SETCOLOUR,
		SETHERO,
		BEGINGAME,
		GETGAMEDETAILS,
		GETGAMESTATE,
		UPDATE,
		APPLYACTIONS
	}

	class Response
	{
		Request request;
		int error;
		Error message;

		Object response;

		transient ArrayList<String> removeFields;
		transient ArrayList<String> keepFields;

		public Response()
		{
			request = Request.ILLEGAL_REQUEST;
			error = Error.NO_ERROR.ordinal();
			message = Error.NO_ERROR;  
			response = null;
			
			removeFields = new ArrayList<String>();
			keepFields = new ArrayList<String>();
		}

		public void setRequest(Request req)
		{
			request = req;
		}

		public void setError(Error err)
		{
			error = err.ordinal();
			message = err;
		}

		public void setResponse(Object o)
		{
			response = o;
		}

		public void addRemoveField(String field)
		{
			removeFields.add(field);
		}

		public void addKeepField(String field)
		{
			keepFields.add(field);
		}

		public void RemoveFields(JsonObject obj, String[] parts, int offset)
		{
			for(int i = offset; i<parts.length; ++i)
			{
				if(!obj.has(parts[i]))
					return;

				if(i == parts.length - 1)
				{
						obj.remove(parts[i]);						
				}
				else
				{
					if(parts[i+1].equals("]"))
					{
						JsonArray arr = obj.getAsJsonArray(parts[i]);

						for(int j=0; j < arr.size(); ++j)
						{
							RemoveFields(arr.get(j).getAsJsonObject(), parts, i + 2);
						}
						return;
					}
					else
					{
						obj = obj.getAsJsonObject(parts[i]);
					}
				}
			}
		}

		public String getJson()
		{
			Gson gson = new GsonBuilder().setPrettyPrinting().create();

			JsonObject jo = gson.toJsonTree(this).getAsJsonObject();
			JsonObject r = jo.getAsJsonObject("response");

			if(removeFields.size() > 0)
			{
				for(int i=0; i<removeFields.size(); ++i)
				{
					String[] parts = removeFields.get(i).split("[\\[\\.]");
					RemoveFields(r, parts, 0);
				}
			}

			if(keepFields.size() > 0)
			{
				Iterator<Entry<String, JsonElement>> i = r.entrySet().iterator();
				while(i.hasNext())
				{
					Entry<String, JsonElement> e = i.next();
					if(!keepFields.contains(e.getKey()))
						i.remove();
				}
			}

			return gson.toJson(jo);
		}
	}

	protected void handleRequest(HttpServletRequest req, HttpServletResponse resp) throws IOException
	{
		Response response = new Response();

		// resolve the request
		Request requ = Request.ILLEGAL_REQUEST;
		try
		{
			// get the request
			String request = req.getParameter("request");
			request = request.toUpperCase();

			// convert it to an ordinal for processing
			requ = Request.valueOf(request);
		}
		catch(Exception ex)
		{
			// if anything went wrong, treat it as a bad request
			response.setError(Error.INVALID_REQUEST);
			return;
		}

		response.setRequest(requ);

		// we'll need a persistence manager
		PersistenceManager pm = null;
		try
		{
			pm = PMF.get().getPersistenceManager();
		}
		catch(Exception e)
		{
			response.setError(Error.SERVER_ERROR);
			return;
		}

		// handle the request
		try
		{
			switch(requ)
			{
				case CREATEACCOUNT:
				{
					// create a profile
					String userName = getStringParam(req, "username");
					String password = getStringParam(req, "password");
					String email = getStringParam(req, "email");

					PlayerAccount account = PlayerAccount.CreateAccount(pm, userName, password, email);
					if(account == null)
						throw new ServerException(Error.INVALID_USER);

					response.addRemoveField("password");
					response.addRemoveField("email");
					response.addRemoveField("pastGames");
					response.setResponse(account);
					break;
				}
				case LOGIN:
				{
					// login/get profile details
					String userName = getStringParam(req, "username");
					String password = getStringParam(req, "password");

					PlayerAccount account = PlayerAccount.Login(pm, userName, password);

					response.addRemoveField("password");
					response.addRemoveField("email");
					response.addRemoveField("pastGames");
					response.setResponse(account);
					break;
				}
				case GETUSER:
				{
					String userName = req.getParameter("username");
					String userID = req.getParameter("playerid");

					if(userName == null && userID == null)
						throw new ServerException(Error.INVALID_ARGUMENTS);

					PlayerAccount account = null;
					if(userID != null)
					{
						long id = getLongParam(req, "playerid");
						account = PlayerAccount.GetAccount(pm, id);
					}
					else
					{
						account = PlayerAccount.GetAccountByName(pm, userName);
					}

					response.addRemoveField("password");
					response.addRemoveField("email");
					response.addRemoveField("pastGames");
					response.setResponse(account);
					break;
				}
				case FORGOTACCOUNT:
				{
					// email the player their account name
					String email = getStringParam(req, "email");

					String accountName = PlayerAccount.ForgotAccountName(pm, email);
					response.setResponse(accountName);
					break;
				}
				case RESETPASSWORD:
				{
					String userName = getStringParam(req, "username");
					String email = getStringParam(req, "email");

					PlayerAccount player = PlayerAccount.GetAccountByName(pm, userName);
					player.ResetPassword(email);
					break;
				}
				case CHANGEPASSWORD:
				{
					String userName = getStringParam(req, "username");
					String password = getStringParam(req, "password");
					String newPassword = getStringParam(req, "newpassword");

					PlayerAccount player = PlayerAccount.GetAccountByName(pm, userName);

					Error r = player.ChangePassword(password, newPassword);
					if(r != Error.NO_ERROR)
					{
						response.setError(r);
						break;
					}
					break;
				}
				case GETACTIVEGAMES:
				{
					long id = getLongParam(req, "playerid");
					PlayerAccount player = PlayerAccount.GetAccount(pm, id);

					response.addKeepField("id");
					response.addKeepField("activeGames");
					response.setResponse(player);
					break;
				}
				case GETPASTGAMES:
				{
					long id = getLongParam(req, "playerid");
					PlayerAccount player = PlayerAccount.GetAccount(pm, id);

					response.addKeepField("id");
					response.addKeepField("pastGames");
					response.setResponse(player);
					break;
				}
				case GETWAITINGGAMES:
				{
					long id = getLongParam(req, "playerid");
					PlayerAccount player = PlayerAccount.GetAccount(pm, id);

					response.addKeepField("id");
					response.addKeepField("gamesWaiting");
					response.setResponse(player);
					break;
				}
				case RESETPROFILE:
				{
					long id = getLongParam(req, "playerid");
					PlayerAccount player = PlayerAccount.GetAccount(pm, id);
					player.ClearProfile();
					break;
				}
				case CREATEGAME:
				{
					// create a new game
					String name = getStringParam(req, "name");
					String map = getStringParam(req, "map");
					int maxPlayers = getIntParam(req, "players");
					long creator = getLongParam(req, "creator");
					int turnTime = getIntParam(req, "turntime");

					Lobby game = Lobby.CreateGame(pm, name, map, maxPlayers, creator, turnTime);

					response.setResponse(game);
					break;
				}
				case DESTROYGAME:
				{
					// create a new game
					long id = getLongParam(req, "game");

					Game game = Game.GetGame(pm, id);
					game.DestroyGame(pm);
					break;
				}
				case GETGAME:
				{
					String gameName = req.getParameter("gamename");
					String gameID = req.getParameter("gameid");

					if(gameName == null && gameID == null)
						throw new ServerException(Error.INVALID_ARGUMENTS);

					Lobby game = null;
					if(gameID != null)
					{
						long id = getLongParam(req, "gameid");
						game = Lobby.GetGame(pm, id);
					}
					else
					{
						game = Lobby.FindGame(pm, gameName);
					}

					// touch the players to pull them from the datastore...
					int numPlayers = game.getNumPlayers();
					for(int i=0; i<numPlayers; ++i)
						game.getPlayer(i).getName(pm);

					response.addRemoveField("players[].key");
					response.setResponse(game);
					break;
				}
				case JOINGAME:
				{
					// join a specific game
					long gameID = getLongParam(req, "game");
					long playerID = getLongParam(req, "playerid");

					Lobby lobby = Lobby.GetGame(pm, gameID);
					PlayerAccount player = PlayerAccount.GetAccount(pm, playerID);

					Error r = lobby.JoinGame(pm, player);
					if(r == Error.NO_ERROR)
					{
						response.addRemoveField("players[].key");
						response.setResponse(lobby);
					}
					else
						response.setError(r);
					break;
				}
				case FINDGAME:
				{
					// find a game to join matching users profile
					long playerID = getLongParam(req, "playerid");

					PlayerAccount player = PlayerAccount.GetAccount(pm, playerID);

					Lobby lobby = Lobby.FindRandomGame(player);
					if(lobby == null)
					{
						response.setError(Error.INVALID_GAME);
						break;
					}

					lobby.JoinGame(pm, player);

					// touch the players to pull them from the datastore...
					int numPlayers = lobby.getNumPlayers();
					for(int i=0; i<numPlayers; ++i)
						lobby.getPlayer(i).getName(pm);

					response.addRemoveField("players[].key");
					response.setResponse(lobby);
					break;
				}
				case LEAVEGAME:
				{
					// leave a game
					long gameID = getLongParam(req, "gameid");
					long playerID = getLongParam(req, "playerid");

					// remove the player from the game
					PlayerAccount player = PlayerAccount.GetAccount(pm, playerID);
					player.RemoveGame(gameID);

					Lobby lobby = Lobby.GetGame(pm, gameID);
					lobby.LeaveGame(pm, player);
					break;
				}
				case SETRACE:
				{
					// set player race
					long gameID = getLongParam(req, "gameid");
					long playerID = getLongParam(req, "playerid");
					int race = getIntParam(req, "race");

					Lobby lobby = Lobby.GetGame(pm, gameID);
					lobby.SetPlayerRace(playerID, race);
					break;
				}
				case SETCOLOUR:
				{
					// set player race
					long gameID = getLongParam(req, "gameid");
					long playerID = getLongParam(req, "playerid");
					int colour = getIntParam(req, "colour");

					Lobby lobby = Lobby.GetGame(pm, gameID);
					lobby.SetPlayerColour(playerID, colour);
					break;
				}
				case SETHERO:
				{
					// set player race
					long gameID = getLongParam(req, "gameid");
					long playerID = getLongParam(req, "playerid");
					int hero = getIntParam(req, "hero");

					Lobby lobby = Lobby.GetGame(pm, gameID);
					lobby.SetPlayerHero(playerID, hero);
					break;
				}
				case BEGINGAME:
				{
					// begin game
					long gameID = getLongParam(req, "game");
					String playersString = getStringParam(req, "players");

					Lobby lobby = Lobby.GetGame(pm, gameID);

					String playerStrings[] = playersString.split(",");
					if(playerStrings.length != lobby.getNumPlayers())
						throw new ServerException(Error.INVALID_ARGUMENTS);

					long players[] = new long[playerStrings.length];
					for(int a=0; a<playerStrings.length; ++a)
						players[a] = Long.parseLong(playerStrings[a]);

					Game game = lobby.BeginGame(pm, players);
					if(game == null)
					{
						response.setError(Error.UNABLE_TO_START_GAME);
						break;
					}

					response.addRemoveField("players[].key");
					response.setResponse(game);
					break;
				}
				case GETGAMEDETAILS:
				{
					// get game progress details
					long id = getLongParam(req, "game");

					Game game = Game.GetGame(pm, id);

					// touch the players to pull them from the datastore...
					int numPlayers = game.getNumPlayers();
					for(int i=0; i<numPlayers; ++i)
						game.getPlayer(i).getName(pm);

					response.addRemoveField("players[].key");
					response.setResponse(game);
					break;
				}
				case GETGAMESTATE:
				{
					// get the current game state

					// castle ownership + building info

					// ruin collection

					// units...

					// groups...

					// current action timestamp
					break;
				}
				case UPDATE:
				{
					// update the game state
					Game game = Game.GetGame(pm, getLongParam(req, "game"));
					int firstAction = getIntParam(req, "firstaction");

					class ActionList
					{
						@SuppressWarnings("unused")
						int totalActions;
						@SuppressWarnings("unused")
						int firstAction;
						@SuppressWarnings("unused")
						List<Action> actions;
					}

					ActionList list = new ActionList();
					list.totalActions = game.GetNumActions();
					list.firstAction = firstAction;
					list.actions =  game.GetActions(firstAction);

					response.addRemoveField("actions[].id");
					response.setResponse(list);
					break;
				}
				case APPLYACTIONS:
				{
					// apply a list of actions
					Game game = Game.GetGame(pm, getLongParam(req, "game"));
					String actionList = getStringParam(req, "actions");
					String actions[] = actionList.split("\n");

					// for each action in the list
					for(int a=0; a<actions.length; ++a)
					{
						// split the action from the args
						String actionDescription[] = actions[a].split(":");

						// parse the args into a list of integers
						int args[] = null;
						if(actionDescription.length == 2)
						{
							String argsString[] = actionDescription[1].split(",");

							args = new int[argsString.length];
							for(int b=0; b<argsString.length; ++b)
								args[b] = Integer.parseInt(argsString[b]);
						}

						// parse the action
						Action.ActionType action = Action.ActionType.UNKNOWN_ACTION;
						try
						{
							action = Action.ActionType.valueOf(actionDescription[0].toUpperCase());
						}
						catch(Exception ex)
						{
						}

						if(action != Action.ActionType.UNKNOWN_ACTION)
							game.AddAction(new Action(action, args));

						// perform the action
						switch(action)
						{
							case ENDTURN:
							{
								// hack to begin next players turn
								game.BeginTurn(args[0]);
								break;
							}
							case VICTORY:
							{
								// end the game
								game.EndGame(pm);
								break;
							}
							case UNKNOWN_ACTION:
							{
								// bad request? can we trust it at all? should we bail out entirely??
								throw new ServerException(Error.INVALID_ACTION);
							}
							default:
							{
								// most actions do not need any special handling...
								break;
							}
						}
					}
					break;
				}
				case ILLEGAL_REQUEST:
				default:
				{
					throw new ServerException(Error.INVALID_REQUEST);
				}
			}
		}
		catch(ServerException e)
		{
			response.setError(e.serverError);
		}
		finally
		{
			// write out response
			PrintWriter wr = resp.getWriter();		
			wr.print(response.getJson());

			// close and commit changes
			pm.close();
		}
	}

	protected int getIntParam(HttpServletRequest req, String param) throws ServerException
	{
		String parameter = req.getParameter(param);
		if(parameter == null)
			throw new ServerException(Error.EXPECTED_ARGUMENTS);
		if(parameter.isEmpty())
			throw new ServerException(Error.INVALID_ARGUMENTS);

		int i = 0;
		try
		{
			i = Integer.parseInt(parameter);
		}
		catch(NumberFormatException e)
		{
			throw new ServerException(Error.INVALID_ARGUMENTS);
		}
		return i;
	}

	protected long getLongParam(HttpServletRequest req, String param) throws ServerException
	{
		String parameter = req.getParameter(param);
		if(parameter == null)
			throw new ServerException(Error.EXPECTED_ARGUMENTS);
		if(parameter.isEmpty())
			throw new ServerException(Error.INVALID_ARGUMENTS);

		long l = 0;
		try
		{
			l = Long.parseLong(parameter);
		}
		catch(NumberFormatException e)
		{
			throw new ServerException(Error.INVALID_ARGUMENTS);
		}
		return l;
	}

	protected String getStringParam(HttpServletRequest req, String param) throws ServerException
	{
		String parameter = req.getParameter(param);
		if(parameter == null)
			throw new ServerException(Error.EXPECTED_ARGUMENTS);
		if(parameter.isEmpty())
			throw new ServerException(Error.INVALID_ARGUMENTS);
		return parameter;
	}

	public void doGet(HttpServletRequest req, HttpServletResponse resp) throws IOException
	{
		resp.setContentType("application/json");
		handleRequest(req, resp);
	}

	public void doPost(HttpServletRequest req, HttpServletResponse resp) throws IOException
	{
		resp.setContentType("application/json");
		handleRequest(req, resp);
	}
}
