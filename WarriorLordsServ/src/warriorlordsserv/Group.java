package warriorlordsserv;

import javax.jdo.annotations.IdGeneratorStrategy;
import javax.jdo.annotations.IdentityType;
import javax.jdo.annotations.PersistenceCapable;
import javax.jdo.annotations.Persistent;
import javax.jdo.annotations.PrimaryKey;

import com.google.appengine.api.datastore.Key;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Group
{
	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Key id;

	@Persistent
	public int player;
	@Persistent
	public int x;
	@Persistent
	public int y;

	@Persistent
	public int[] units;

	public Group(int _player, int _x, int _y, int[] _units)
	{
		player = _player;
		x = _x;
		y = _y;

		units = _units;
	}

	public Key getId()
	{
		return id;
	}
}
