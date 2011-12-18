module main;

import std.stdio;
import std.socket;
import std.concurrency;
import core.time: dur, Duration;
import std.string;

private class Group
{
	Tid tid;
	uint id;
};

int main(string[] argv)
{
	Socket listener = new Socket(AddressFamily.INET, SocketType.STREAM, ProtocolType.TCP);
	listener.bind(new InternetAddress(43210));

	while(1)
	{
		try
		{
			// listen for incoming connections
			listener.listen(10);
			Socket connection = listener.accept();

			// send the new socket to the connection negotiation thread
			spawn(&negotiationThread, cast(shared)connection);
		}
		catch(SocketException)
		{
			// failed to accept...
			int x = 0;
		}
	}

	return 0;
}

private void negotiationThread(shared Socket newConnection)
{
	class ClientGroups
	{
		Group[uint] groups;
	};

	shared static ClientGroups activeGroups;

	Socket connection = cast()newConnection;

	try
	{
		// receive connection request
		char[1024] request;
		auto bytes = connection.receive(request[]);

//		string[] tokens = request.split("\n");

		// acknowledge connection
		string acceptedString = "Connection accepted";
		connection.send(acceptedString);

		uint id = 0;
		auto group = activeGroups.groups[id];

		// if the connection represents a new group
		if(group is null)
		{
			group = new shared(Group);
			activeGroups.groups[id] = cast(shared)group;

			group.id = id;
			group.tid = cast(shared)spawn(&groupThread, cast(shared)group);
		}

		// send the new connection to the group thread
		send(cast()group.tid, newConnection);
	}
	catch(OwnerTerminated)
	{
		// owner terminated
		int x = 0;
	}
}

private void groupThread(shared Group group)
{
	Socket[] sockets;

	Duration d;

	SocketSet socketSet = new SocketSet();
	for (;; socketSet.reset())
	{
		try
		{
			// poll for new clients
			Socket newClient;
			while(receiveTimeout(d, (shared Socket client){ newClient = cast()client; }))
				sockets ~= newClient;

			// add sockets to list
			foreach(socket; sockets)
				socketSet.add(socket);

			// should use a timeout or something
			Socket.select(socketSet, null, null);

			// echo the message to others in group
			//...
		}
		catch
		{
			int x = 0;
		}
	}
}
