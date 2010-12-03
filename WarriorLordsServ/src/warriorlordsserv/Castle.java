package warriorlordsserv;

import javax.jdo.annotations.IdGeneratorStrategy;
import javax.jdo.annotations.IdentityType;
import javax.jdo.annotations.PersistenceCapable;
import javax.jdo.annotations.Persistent;
import javax.jdo.annotations.PrimaryKey;

import com.google.appengine.api.datastore.Key;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Castle
{
	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Key id;

	@Persistent
	public int player;
	@Persistent
	public int building;
	@Persistent
	public int buildTime;

	public Castle(int owner)
	{
		player = owner;
		building = 0;
		buildTime = 0;
	}

	public Key getId()
	{
		return id;
	}
}
