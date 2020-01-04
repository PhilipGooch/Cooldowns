#include "Server.h"

Server::Server()
{
	printf("Slither Server\n\n");

	// Printing the IP of the server. This is the number that will need to be entered into the client's server IP text file in order to connect.
	std::cout << serverIP << "\n";

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

void Server::writeToBuffer(char type, Snake* snake)
{
	// 'i' = Set ID.
	// 'a' = Add snake.
	// 'r' = Remove snake.
	// 'u' = Sync game state.

	// Error handling for invalid packet type.
	if (type != 'i' && type != 'a' && type != 'r' && type != 'u' && type != 't')
	{
		printf("Invalid packet type.");
		assert(false);
	}

	// Writing data about the given snake into the buffer.
	memset(cBuffer, '-', BUFFERSIZE);
	cBuffer[0] = type;
	cBuffer[1] = (char)snake->ID;
	fBuffer[0] = snake->positionX;
	fBuffer[1] = snake->positionY;
	fBuffer[2] = snake->directionX;
	fBuffer[3] = snake->directionY;
	iBuffer[0] = snake->length;
	iBuffer[1] = snake->r;
	iBuffer[2] = snake->g;
	iBuffer[3] = snake->b;
	iBuffer[4] = snake->timeStamp;
	bBuffer[0] = snake->boosting;
}

void Server::sendPacket(sf::TcpSocket* client, char type, Snake* snake)
{
	writeToBuffer(type, snake);

	// Sending the packet.
	if (client->send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
	{
		printf("Error: send()");
		assert(false);
	}
}


void Server::run()
{
	while (true)
	{
		// wait() must be called before call to isReady() to know which sockets are ready to receive data.  
		if (selector.wait(sf::microseconds(1)))
		{
			// If there is a new incoming connection.
			// isReady() returns true if the socket passed as an argument will not block.
			if (selector.isReady(tcpListener))
			{
				// Accept connection
				Client* client = new Client;
				tcpListener.accept(client->TCPSocket);
				selector.add(client->TCPSocket);

				// Setting to blocking mode for the initial conversation. Could cause other players to lag if there is a problem with the connection phase of a client. not idea...
				client->TCPSocket.setBlocking(true);

				// Generates a unique ID for the client and it's snake.
				int ID = allocateID();

				// Adding client to map.
				clients.emplace(ID, client);

				// Add a new snake to server side snakes map.
				Snake* snake = generateSnake(ID);
				snakes.emplace(ID, snake);

				// Tell the client it's ID.
				memset(cBuffer, '-', BUFFERSIZE);
				cBuffer[1] = (char)snake->ID;
				if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
				{
					printf("Error: send()");
					assert(false);
				}

				receiveUDPPort(client);

				// Syncing the client's time with the server's.
				syncTime(client);



				// Tell the client to add a snake on the client side for each exisiting server side snake.
				std::map<int, Snake*>::iterator it;
				for (it = snakes.begin(); it != snakes.end(); it++)
				{
					sendPacket(&client->TCPSocket, 'a', it->second);
				}

				// Send message to indicate finished sending all snakes info.
				memset(cBuffer, '-', BUFFERSIZE);
				cBuffer[0] = 'z';
				if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Done)
				{
					printf("Error: send()");
					assert(false);
				}

				// Tell every other client to add this new snake.
				std::map<int, Client*>::iterator itt;
				for (itt = clients.begin(); itt != clients.end(); itt++)
				{
					sendPacket(&itt->second->TCPSocket, 'a', snake);
				}

				client->TCPSocket.setBlocking(false);
			}
			// If there is no new incoming connection.
			else
			{
				// TCP
				// HANDLING DISCONNECTIONS ///////////////////////////////////////////////

				bool disconnection = false;
				int disconnectedClientID = -1;

				// Check each of the client TCP sockets.
				std::map<int, Client*>::iterator it;
				for (it = clients.begin(); it != clients.end(); it++)
				{
					// If the TCP socket will not block.
					if (selector.isReady(it->second->TCPSocket))
					{
						// If the client disconnects.
						if (it->second->TCPSocket.receive(packet) == sf::Socket::Disconnected)
						{
							disconnection = true;
							disconnectedClientID = it->first;

							// Remove client from selector and array.
							selector.remove(it->second->TCPSocket);
							clients.erase(it);
							break;
						}
					}
				}
				if (disconnection)
				{
					// Tell each remaining client to remove that client's snake.
					for (it = clients.begin(); it != clients.end(); it++)
					{
						sendPacket(&it->second->TCPSocket, 'r', snakes.at(disconnectedClientID));

					}
					// Remove the snake from server side snake array.
					snakes.erase(disconnectedClientID);
				}

				////////////////////////////////////////////////////////////////////////////////


			}
		}



		// UDP
		// Check the server's UDP socket.

		// Allways receiving data while there is some... is this a problem if i am flooding the server with data? should i use multiple threads? the answer is YES! pretty sure...
		do
		{
			status = udpSocket.receive(cBuffer, BUFFERSIZE, bytes, tempClientIP, tempClientUDPPort);
			if (status == sf::Socket::Done)
			{
				if (cBuffer[0] == 'u')
				{
					// If the snake the packet is refering to has disconnected, ignore the packet.
					if (snakes.find(cBuffer[1]) == snakes.end())
					{
						continue;
					}
					Snake* snake = snakes.at(cBuffer[1]);
					snake->ID = cBuffer[1];
					snake->positionX = fBuffer[0];
					snake->positionY = fBuffer[1];
					snake->directionX = fBuffer[2];
					snake->directionY = fBuffer[3];
					snake->timeStamp = iBuffer[4];
					snake->boosting = bBuffer[0];
				}
			}
			else if (status == sf::Socket::Partial)
			{
				printf("receive() - Partial\n");
				assert(false);
			}
			else if (status == sf::Socket::Disconnected)
			{
				printf("receive() - Disconnected\n");
				//assert(false);
			}
			else if (status == sf::Socket::Error)
			{
				printf("receive() - Error\n");
				assert(false);
			}
		} while (status != sf::Socket::NotReady);

		// BROADCAST

		// Every broadcast interval, send a broadcast of all snakes data to each client.
		sf::Time deltaTime = deltaTimeClock.restart();
		broadcastTimer += deltaTime;
		if (broadcastTimer > sf::seconds(broadCastInterval))
		{
			broadcast();
			broadcastTimer -= sf::seconds(broadCastInterval);
		}
	}
}

Snake* Server::generateSnake(int ID)
{
	return new Snake(ID, 60, 30 * ID, -30 * ID, 20, 1, colours[ID % 6].r, colours[ID % 6].g, colours[ID % 6].b);
}

int Server::allocateID()
{
	int ID = 0;
	while (snakes.find(ID) != snakes.end())
	{
		ID++;
	}
	return ID;
}

void Server::syncTime(Client* client)
{
	// Sending ping test packets.
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
	cBuffer[0] = 't';
	iBuffer[3] = clock.getElapsedTime().asMilliseconds() + oneWayPingTime;
	if (client->TCPSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
	{
		printf("Error: send()");
		assert(false);
	}
}

void Server::broadcast()
{
	// Sending the last confirmed state of each snake to each client.
	std::map<int, Client*>::iterator it;
	for (it = clients.begin(); it != clients.end(); it++)
	{
		std::map<int, Snake*>::iterator itt;
		for (itt = snakes.begin(); itt != snakes.end(); itt++)
		{
			writeToBuffer('u', itt->second);
			udpSocket.send(cBuffer, BUFFERSIZE, it->second->TCPSocket.getRemoteAddress(), it->second->UDPPort);
		}
	}
}

void Server::receiveUDPPort(Client* client)
{
	// receiving client UDP port address.
	if (client->TCPSocket.receive(cBuffer, BUFFERSIZE, bytes) == sf::Socket::Status::Done)
	{
		unsigned short* usBuffer = (unsigned short*)&cBuffer[1];
		client->UDPPort = usBuffer[0];
	}
}

//bool Game::collision(sf::Vector2f position1, sf::Vector2f position2, float radius1, float radius2)
//{
//	return pow(position2.x - position1.x, 2) + pow(position2.y - position1.y, 2) < pow(radius1 + radius2, 2);
//}

// COLLISIONS

	/*for (int i = 1; i < (int)snakes.size(); i++)
	{
		if (collision(snakes[0].getPosition(), snakes[i].getPosition(), snakes[0].getRadius(), snakes[i].getCollisionZoneRadius()))
		{
			for (int j = 0; j < (int)snakes[i].getSnake().size(); j++)
			{
				if (collision(snakes[0].getPosition(), snakes[i].getSnake()[j], snakes[0].getRadius(), snakes[i].getRadius()))
				{
					snakes[0].die();
				}
			}
		}
	}*/


	/*for (int j = 0; j < snakes[0].getSnake().size(); j++)
	{
		if (collision(snakes[1].getPosition(), snakes[0].getSnake()[j], snakes[1].getRadius(), snakes[0].getRadius()))
		{
			snakes[1].die();
		}
	}*/