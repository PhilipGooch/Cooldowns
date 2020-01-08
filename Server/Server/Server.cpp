#include "Server.h"

Server::Server()
{
	printf("Server\n\n");

	// Printing the IP of the server. This is the number that will need to be entered into the client's server IP text file in order to connect.
	std::cout << serverIP << "\n\n";

	// Telling the listener socket to listen for new connections.
	tcpListener.listen(serverPortTCP, serverIP);

	// Binding th UDP socket to the specified port and setting it to non blocking mode.
	udpSocket.bind(serverPortUDP, serverIP);
	udpSocket.setBlocking(false);

	// Adding the TCP and UDP sockets to the socket selector.
	selector.add(tcpListener);
	selector.add(udpSocket);
}

Server::~Server()
{
}

void Server::run()
{
	while (true)
	{
		//printf("%i\n", clock.getElapsedTime().asMicroseconds());
		// wait() must be called before call to isReady() to know which sockets are ready to receive data.  
		if (selector.wait(sf::microseconds(1)))
		{
			// If there is a new incoming connection.
			// isReady() returns true if the socket passed as an argument will not block.
			if (selector.isReady(tcpListener))
			{
				// Accept connection
				Client* client = new Client();
				tcpListener.accept(client->TCPSocket);
				selector.add(client->TCPSocket);

				// Setting to blocking mode for the initial conversation. 
				client->TCPSocket.setBlocking(true);

				// Generates a unique ID for the client and it's snake.
				int ID = allocateID();

				// Adding client to map.
				clients.emplace(ID, client);

				// Tell the client it's ID.
				const int BUFFERSIZE = sizeof(char) * 2;
				char cBuffer[BUFFERSIZE];
				memset(cBuffer, '-', BUFFERSIZE);
				cBuffer[1] = (char)ID;
				if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
				{
					printf("Error: send()");
					assert(false);
				}

				//receiveUDPPort(client);

				//// Syncing the client's time with the server's.
				//syncTime(client);



				//// Tell the client to add a player on the client side for each exisiting server side player.
				////const int BUFFERSIZE = sizeof(char) * 2;
				////char cBuffer[BUFFERSIZE];

				//std::map<int, Client*>::iterator it;
				//for (it = clients.begin(); it != clients.end(); it++)
				//{
				//	memset(cBuffer, '-', BUFFERSIZE);
				//	cBuffer[0] = 'c';
				//	cBuffer[1] = (char)it->second->ID;
				//	if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
				//	{
				//		printf("Error: send()");
				//		assert(false);
				//	}
				//}

				//// Send message to indicate finished sending all snakes info.
				//memset(cBuffer, '-', BUFFERSIZE);
				//cBuffer[0] = 'z';
				//if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
				//{
				//	printf("Error: send()");
				//	assert(false);
				//}

				//// Tell every other client to add this new player.
				//std::map<int, Client*>::iterator itt;
				//for (itt = clients.begin(); itt != clients.end(); itt++)
				//{
				//	memset(cBuffer, '-', BUFFERSIZE);
				//	cBuffer[0] = 'a';
				//	cBuffer[1] = (char)ID;
				//}

				client->TCPSocket.setBlocking(false);

				printf("Added new client with ID: %i\n", ID);
			}
			// If there is no new incoming connection.
			else
			{
				// TCP

				



				bool disconnection = false;
				int disconnectedClientID = -1;

				// Check each of the client TCP sockets.
				std::map<int, Client*>::iterator it;
				for (it = clients.begin(); it != clients.end(); it++)
				{
					// If the TCP socket will not block.
					if (selector.isReady(it->second->TCPSocket))
					{
						const int BUFFERSIZE = sizeof(char) * 4 + sizeof(unsigned int);
						char cBuffer[BUFFERSIZE];
						int* iBuffer = (int*)&cBuffer[4];

						status = it->second->TCPSocket.receive(cBuffer, BUFFERSIZE, bytes);

						// If the client disconnects.
						if (status == sf::Socket::Disconnected)
						{
							disconnection = true;
							disconnectedClientID = it->first;

							// Remove client from selector and array.
							selector.remove(it->second->TCPSocket);
							clients.erase(it);
							break;
						}
						else if (status == sf::Socket::Done)
						{
							// If a player has cast a spell.
							if (cBuffer[0] == 's')
							{
								std::map<int, Client*>::iterator it;
								for (it = clients.begin(); it != clients.end(); it++)
								{
									if (it->first != (int)cBuffer[1])
									{
										status = it->second->TCPSocket.send(cBuffer, BUFFERSIZE, bytes);
										if (status != sf::Socket::Status::Done)
										{
											printf("Send failed.");
											assert(false);
										}
									}
								}
							}
						}
					}
				}
				if (disconnection)
				{
					const int BUFFERSIZE = sizeof(char) * 2;
					char cBuffer[BUFFERSIZE];
					// Tell each remaining client to remove that player.
					for (it = clients.begin(); it != clients.end(); it++)
					{
						memset(cBuffer, '-', BUFFERSIZE);
						cBuffer[0] = 'd';
						cBuffer[1] = (char)disconnectedClientID;
						if (it->second->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
						{
							printf("Error: send()");
							assert(false);
						}
					}
					// Remove the player from server side player array.
					clients.erase(disconnectedClientID);
					printf("Client disconnected with ID: %i\n", disconnectedClientID);
				}

				////////////////////////////////////////////////////////////////////////////////


			}
		}



		// UDP
		// Check the server's UDP socket.

		// Allways receiving data while there is some... is this a problem if i am flooding the server with data? should i use multiple threads? the answer is YES! pretty sure...
		//do
		//{
		//	status = udpSocket.receive(cBuffer, BUFFERSIZE, bytes, tempClientIP, tempClientUDPPort);
		//	if (status == sf::Socket::Done)
		//	{
		//		if (cBuffer[0] == 'u')
		//		{
		//			// If the snake the packet is refering to has disconnected, ignore the packet.
		//			if (snakes.find(cBuffer[1]) == snakes.end())
		//			{
		//				continue;
		//			}
		//			Snake* snake = snakes.at(cBuffer[1]);
		//			snake->ID = cBuffer[1];
		//			snake->positionX = fBuffer[0];
		//			snake->positionY = fBuffer[1];
		//			snake->directionX = fBuffer[2];
		//			snake->directionY = fBuffer[3];
		//			snake->timeStamp = iBuffer[4];
		//			snake->boosting = bBuffer[0];
		//		}
		//	}
		//	else if (status == sf::Socket::Partial)
		//	{
		//		printf("receive() - Partial\n");
		//		assert(false);
		//	}
		//	else if (status == sf::Socket::Disconnected)
		//	{
		//		printf("receive() - Disconnected\n");
		//		//assert(false);
		//	}
		//	else if (status == sf::Socket::Error)
		//	{
		//		printf("receive() - Error\n");
		//		assert(false);
		//	}
		//} while (status != sf::Socket::NotReady);

		// BROADCAST

		// Every broadcast interval, send a broadcast of all snakes data to each client.
		/*sf::Time deltaTime = deltaTimeClock.restart();
		broadcastTimer += deltaTime;
		if (broadcastTimer > sf::seconds(broadCastInterval))
		{
			broadcast();
			broadcastTimer -= sf::seconds(broadCastInterval);
		}*/
	}
}

int Server::allocateID()
{
	int ID = 0;
	while (clients.find(ID) != clients.end())
	{
		ID++;
	}
	return ID;
}

void Server::syncTime(Client* client)
{
	// Sending ping test packets.
	const int BUFFERSIZE = sizeof(int);
	char cBuffer[BUFFERSIZE];
	int* iBuffer = (int*)&cBuffer[0];
	memset(cBuffer, '-', BUFFERSIZE);

	sf::Time pingTimer = sf::Time::Zero;
	sf::Time pingTimes[5];
	sf::Clock pingClock;
	for (int i = 0; i < 5; i++)
	{
		if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
		{
			printf("Error: send()");
			assert(false);
		}
		if (client->TCPSocket.receive(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
		{
			printf("Error: receive()");
			assert(false);
		}
		pingTimes[i] = pingClock.restart();
	}
	// Calculating estimated latency.
	float averagePingTime = 0;
	for (int i = 0; i < 5; i++)
	{
		averagePingTime += pingTimes[i].asMicroseconds();
	}
	averagePingTime /= 5;
	int oneWayPingTime = averagePingTime / 2;

	// Sending server's time plus the one way ping.
	memset(cBuffer, '-', BUFFERSIZE);
	iBuffer[0] = clock.getElapsedTime().asMilliseconds() + oneWayPingTime;
	if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
	{
		printf("Error: send()");
		assert(false);
	}
}

//void Server::broadcastTCP()
//{
//	std::map<int, Client*>::iterator it;
//	for (it = clients.begin(); it != clients.end(); it++)
//	{
//		std::map<int, Snake*>::iterator itt;
//		for (itt = snakes.begin(); itt != snakes.end(); itt++)
//		{
//			writeToBuffer('u', itt->second);
//			it->second->TCPSocket.send(cBuffer, BUFFERSIZE, it->second->TCPSocket.getRemoteAddress(), it->second->UDPPort);
//		}
//	}
//}

void Server::receiveUDPPort(Client* client)
{
	const int BUFFERSIZE = sizeof(char) * 2;
	char cBuffer[BUFFERSIZE];
	// receiving client UDP port address.
	if (client->TCPSocket.receive(cBuffer, BUFFERSIZE, bytes) == sf::Socket::Status::Done)
	{
		unsigned short* usBuffer = (unsigned short*)&cBuffer[1];
		client->UDPPort = usBuffer[0];
	}
}
