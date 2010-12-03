package warriorlordsserv;

import java.util.ArrayList;

import javax.jdo.annotations.IdGeneratorStrategy;
import javax.jdo.annotations.IdentityType;
import javax.jdo.annotations.PersistenceCapable;
import javax.jdo.annotations.Persistent;
import javax.jdo.annotations.PrimaryKey;

import com.google.appengine.api.datastore.Key;

@PersistenceCapable(identityType = IdentityType.APPLICATION)
public class Unit
{
	public enum TargetType
	{
		Any,
		Melee,
		Ranged
	}

	public enum TargetStrength
	{
		Strongest,
		Weakest
	}

	@PrimaryKey
	@Persistent(valueStrategy = IdGeneratorStrategy.IDENTITY)
	private Key id;

	@Persistent
	public int type;
	@Persistent
	public int player;

	@Persistent
	public int movement;
	@Persistent
	public int life;

	@Persistent
	public int kills;
	@Persistent
	public int victories;

	@Persistent
	public TargetType targetType;
	@Persistent
	public TargetStrength targetStrength;
	@Persistent
	public boolean bAttackAvailable;

	@Persistent
	public ArrayList<Integer> items;

	public Unit(int _player, int _type, int _movement, int _life)
	{
		type = _type;
		player = _player;
		movement = _movement;
		life = _life;

		kills = 0;
		victories = 0;

		targetType = TargetType.Ranged;
		targetStrength = TargetStrength.Weakest;
		bAttackAvailable = false;
		
		if(type < 8)
			items = new ArrayList<Integer>();
	}

	public Key getId()
	{
		return id;
	}
}
