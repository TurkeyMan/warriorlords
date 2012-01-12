module main;

import std.string;
import std.stdio;
import std.conv;
import std.socket;
import std.concurrency;
import std.algorithm;
import std.array;
import core.time: dur, Duration;
import core.thread;

void remove(T)(ref T[] a, size_t index)
{
	swap(a[index], a.front);
	a.popFront;
	if(a.length == 0)
		return;
	swap(a.front, a[index-1]);
}

private class Group
{
	Tid tid;
	uint id;
};

private shared Group[uint] groups;

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
	Socket connection = cast()newConnection;

	try
	{
		// receive connection request
		char[1024] request;
		auto bytes = connection.receive(request[]);

		auto tokens = request[0..bytes].split("\n");

		if(tokens.length < 2 || tokens[0] != "WLClient1.0")
			return;

		uint id = parse!uint(tokens[1], 16);

		// acknowledge connection
		string acceptedString = "Connection accepted";
		connection.send(acceptedString);

		auto group = id in groups;
		if(group == null)
		{
			// if the connection represents a new group
			auto newGroup = new shared(Group);
			groups[id] = newGroup;

			newGroup.id = id;
			newGroup.tid = cast(shared)spawn(&groupThread, cast(shared)newGroup);

			group = &newGroup;
		}

		// send the new connection to the group thread
		send(cast()group.tid, newConnection);
	}
	catch(Exception)
	{
		// owner terminated
		int x = 0;
	}
}

private void groupThread(shared Group group)
{
	immutable Duration d;

	Socket[] sockets;
	SocketSet readSet = new SocketSet();

	writefln("Creted new group: %08X", group.id);

	void newClient(shared Socket client)
	{
		Socket newClient = cast()client;
		sockets ~= newClient;

		writefln("Client %d joined group: %08X - num clients: %d", cast(int)newClient.handle, group.id, sockets.length);
	}

	void removeClient(int i)
	{
		int handle = cast(int)sockets[i].handle;
		sockets.remove(i);

		writefln("Client %d left group: %08X - num clients: %d", handle, group.id, sockets.length);
	}

	for (;; readSet.reset())
	{
		try
		{
			// poll for new clients
			while(receiveTimeout(d, &newClient, (OwnerTerminated){ /* we don't care when the patent thread terminates */ }))
			{
				// allow to process all incoming messages...
			}

			if(sockets.length > 0)
			{
				// add sockets to list
				foreach(i, socket; sockets)
				{
					if(socket.isAlive)
						readSet.add(socket);
					else
					{
						removeClient(i);

						// if there are none left in the group...
						if(sockets.length == 0)
						{
							writefln("All clients have left group: %08X", group.id);

							// destroy the group and terminate the thread
							groups.remove(group.id);
							return;
						}
					}
				}

				// should use a timeout or something
				int signal = Socket.select(readSet, null, null, 100_000); // dur!("msecs")(100) // WHY DOESN'T THIS OVERLOAD EXIST?
				if(signal > 0)
				{
					// echo messages
					foreach(i, socket; sockets)
					{
						if(readSet.isSet(socket))
						{
							// recv the packet
							char[1024] buffer;
							auto bytes = socket.receive(buffer[]);
							if(bytes > 0)
							{
								char[] message = buffer[0..bytes];

								writefln("Client %d -> %s", cast(int)socket.handle, message);

								// and echo to other clients
								for(int j=0; j<sockets.count; ++j)
								{
									if(i == j)
										continue;

									sockets[j].send(message);
								}
							}
						}
					}
				}
			}
		}
		catch
		{
			int x = 0;
		}
	}
}
