package warriorlordsserv;

import com.google.appengine.api.datastore.Key;

import javax.jdo.annotations.IdGeneratorStrategy;
import javax.jdo.annotations.IdentityType;
import javax.jdo.annotations.PersistenceCapable;
import javax.jdo.annotations.Persistent;
import javax.jdo.annotations.PrimaryKey;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Action
{
	public enum ActionType
	{
		UNKNOWN_ACTION,

		ADDCASTLES,
		ADDRUINS,
		CREATEUNIT,
		CREATEGROUP,
		DESTROYGROUP,
		MOVEGROUP,
		REARRANGEGROUP,
		CLAIMCASTLE,
		SETBUILDING,
		SETBATTLEPLAN,
		SEARCH,
		BATTLE,
		ENDTURN,
		VICTORY,
		CAPTUREUNITS,
	}

	Action(ActionType action, int[] arguments)
	{
		type = action;
		args = arguments;
	}

	public Key getId()
	{
		return id;
	}

	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Key id;

	@Persistent
	public ActionType type;
	@Persistent
	public int[] args;
}
