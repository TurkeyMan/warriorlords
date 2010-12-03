package warriorlordsserv;

public class ServerException extends Exception
{
	private static final long serialVersionUID = 2982315871054594995L;

	ServerException(Error error)
	{
		serverError = error;
	}

	public Error serverError;
}
