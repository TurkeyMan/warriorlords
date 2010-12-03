package warriorlordsserv;

import java.util.Date;
import java.util.List;
import java.util.ArrayList;
import javax.jdo.PersistenceManager;
import javax.jdo.annotations.*;
import javax.jdo.Query;
import com.google.appengine.api.datastore.*;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class PlayerAccount
{
	// account details
	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Long id;
	@Persistent
	private Date dateCreated;
	@Persistent
	private String accountName;
	@Persistent
	private String password;
	@Persistent
	private Email email;
	@Persistent
	private PhoneNumber phoneNumber;
	@Persistent
	private ArrayList<Long> gamesWaiting;
	@Persistent
	private ArrayList<Long> activeGames;
	@Persistent
	private ArrayList<Long> pastGames;

	// profile
	@Persistent
	private int gamesPlayed;
	@Persistent
	private int gamesWon;
	@Persistent
	private int gamesLost;

	public long getId()
	{
		return id;
	}

	public String getAccountName()
	{
		return accountName;
	}
	
	public String getAccountCreationDate()
	{
		return dateCreated.toString();
	}

	public int getGamesPlayed()
	{
		return gamesPlayed;
	}

	public int getGamesWon()
	{
		return gamesWon;
	}

	public int getGamesLost()
	{
		return gamesLost;
	}

	public ArrayList<Long> getActiveGames()
	{
		return activeGames;
	}

	public ArrayList<Long> getWaitingGames()
	{
		return gamesWaiting;
	}

	public ArrayList<Long> getPastGames()
	{
		return pastGames;
	}

	protected PlayerAccount()
	{
		dateCreated = new Date();
		activeGames = new ArrayList<Long>();
		pastGames = new ArrayList<Long>();
	}

	public static PlayerAccount Login(PersistenceManager pm, String accountName, String password) throws ServerException
	{
		PlayerAccount account = GetAccountByName(pm, accountName);
		String hash = account.HashPassword(password);

		if(account.password.compareToIgnoreCase(hash) != 0)
			throw new ServerException(Error.INVALID_LOGIN);

		return account;
	}

	public static PlayerAccount CreateAccount(PersistenceManager pm, String accountName, String password, String email) throws ServerException
	{
		try
		{
			GetAccountByName(pm, accountName);
			return null;
		}
		catch(ServerException e)
		{
			if(e.serverError != Error.INVALID_USER)
				throw e;

			PlayerAccount account = new PlayerAccount();
			account.accountName = accountName;
			account.email = new Email(email);
			account.password = account.HashPassword(password);
	
			// email the user regarding account activation
			String message = "Thank you " + account.accountName + " for registering to play WarriorLords!\n\n";
			message += "We hope you enjoy playing the game, and check back for updates regularly!\n";
	
			EmailService.SendEmail("noreply@warriorlords.com", "WarriorLords Admin", email, accountName, "Welcome to WarriorLords!", message);
	
			pm.makePersistent(account);

			return account;
		}
	}

	public static PlayerAccount GetAccount(PersistenceManager pm, long accountID) throws ServerException
	{
		PlayerAccount player = pm.getObjectById(PlayerAccount.class, accountID);
		if(player == null)
			throw new ServerException(Error.INVALID_USER);

		return player;
	}

	@SuppressWarnings("unchecked")
	public static PlayerAccount GetAccountByName(PersistenceManager pm, String accountName) throws ServerException
	{
		Query query = pm.newQuery("SELECT FROM " + PlayerAccount.class.getName() + " WHERE accountName=='" + accountName + "'");

		List<PlayerAccount> accounts = new ArrayList<PlayerAccount>();
		accounts = (List<PlayerAccount>)query.execute();

		PlayerAccount player = null;
		if(accounts.size() == 1)
			player = accounts.get(0);	

		if(player == null)
			throw new ServerException(Error.INVALID_USER);

		return player;
	}

	@SuppressWarnings("unchecked")
	public static String ForgotAccountName(PersistenceManager pm, String email) throws ServerException
	{
		Query query = pm.newQuery("SELECT FROM " + PlayerAccount.class.getName() + " WHERE email=='" + email + "'");

		List<PlayerAccount> accounts = new ArrayList<PlayerAccount>();
		accounts = (List<PlayerAccount>)query.execute();

		PlayerAccount player = null;
		if(accounts.size() == 1)
			player = accounts.get(0);	

		if(player == null)
			throw new ServerException(Error.INVALID_USER);

		// send an email to remind the player their account name
		String message = "Your WarriorLords account name is: " + player.accountName + "\n";
		EmailService.SendEmail("noreply@warriorlords.com", "WarriorLords Admin", player.email.getEmail(), player.accountName, "WarriorLords - Forgotten Account Name", message);

		return player.accountName;
	}

	public String ResetPassword(String email)
	{
		if(email.compareToIgnoreCase(this.email.getEmail()) != 0)
			return null;

		String newPassword = "monday";

		this.password = HashPassword(newPassword);

		// email the new password to the user
		String message = "The password for your account '" + accountName + "' has been reset.\n";
		message += "Your new password is: " + newPassword + "\n\n";
		message += "You should change your password next time you log in.\n";

		EmailService.SendEmail("noreply@warriorlords.com", "WarriorLords Admin", email, accountName, "WarriorLords - Password Reset", message);

		return newPassword;
	}

	public Error ChangePassword(String password, String newPassword)
	{
		String hash = HashPassword(password);

		if(this.password.compareToIgnoreCase(hash) != 0)
			return Error.INVALID_LOGIN;

		this.password = HashPassword(newPassword);

		return Error.NO_ERROR;
	}

	public void ClearProfile()
	{
		gamesWaiting.clear();
		activeGames.clear();
		pastGames.clear();

		gamesPlayed = 0;
		gamesWon = 0;
		gamesLost = 0;
	}
	
	public void AddGame(long game)
	{
		gamesWaiting.add(game);
	}

	public void RemoveGame(long game)
	{
		gamesWaiting.remove(game);
	}

	public void DestroyGame(long game)
	{
		activeGames.remove(game);
	}

	public void BeginGame(long lobby, long game)
	{
		gamesWaiting.remove(lobby);
		activeGames.add(game);
	}

	public void FinishGame(long game, boolean didWin)
	{
		++gamesPlayed;
		if(didWin)
			++gamesWon;
		else
			++gamesLost;

		activeGames.remove(game);
		pastGames.add(game);
	}

	private String HashPassword(String password)
	{
		// has the password with some salt added on the end
		return Hash.SHA1(password + "*" + accountName + "#" + email.getEmail());
	}
}
