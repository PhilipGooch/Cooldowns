#include "Game.h"
#include <fstream>

Game::Game() :
	windowWidth(280.0f),
	windowHeight(280.0f),
	window(sf::VideoMode((unsigned int)windowWidth, (unsigned int)windowHeight), "", sf::Style::Close),
	fps(0),
	ID(-1)
{
	// Reading the IP address of the server from a text file, set by the user.
	std::ifstream inFile;
	inFile.open("IP.txt");
	if (!inFile)
	{
		printf("Error loading IP file.");
		assert(false);
	}
	inFile >> serverIP;
	inFile.close();

	// Binding th UDP socket to the specified port and setting it to non blocking mode.
	udpSocket.bind(sf::Socket::AnyPort);
	udpSocket.setBlocking(false);
	
	// Setting to blocking mode for the initial conversation. Could cause other players to lag if there is a problem with the connection phase of a client. not idea...
	tcpSocket.setBlocking(true);

	connect();
	receiveID();
	sendUDPPort();
	syncTime();

	// Receiving all connected clients' info and adding them to map.
	const int BUFFERSIZE = sizeof(char) * 2;
	char cBuffer[BUFFERSIZE];
	memset(cBuffer, '-', BUFFERSIZE);
	while (true)
	{
		status = tcpSocket.receive(cBuffer, BUFFERSIZE, bytes);
		if (status == sf::Socket::Done)
		{
			if (cBuffer[0] == 'a')
			{
				players.emplace((int)cBuffer[1], new Player());
			}
			else if (cBuffer[0] == 'z')
			{
				break;
			}
		}
	}

	tcpSocket.setBlocking(false);

	// Entering game loop.
	run();
}

Game::~Game()
{
	// Disconnecting the socket on closure of the application.
	tcpSocket.disconnect();
}

void Game::run()
{
	// Constant FPS
	sf::Clock gameLoopClock;
	sf::Time updateTimer = sf::Time::Zero;
	sf::Time fpsTimer = sf::Time::Zero;
	sf::Time updatePacketTimer = sf::Time::Zero;
	int frames = 0;

	while (window.isOpen())
	{
		// Incrementing timers by delta time.
		sf::Time deltaTime = gameLoopClock.restart();
		time += deltaTime;
		fpsTimer += deltaTime;
		updatePacketTimer += deltaTime;

		//printf("%i\n", time.asMicroseconds());
		
		processEvents();
		listen();
		handleInput();
		update(deltaTime.asSeconds());
		render();

		// Sending updated local position packets to the server every send interval.
		if (updatePacketTimer > sf::seconds(sendInterval))
		{
			//sendUpdate(clock.getElapsedTime().asMilliseconds() + startTime);
			updatePacketTimer -= sf::seconds(sendInterval);
		}
		// Counting frames for FPS output.
		frames++;
		// Keeping track of FPS.
		if (fpsTimer > sf::seconds(1))

		{
			fps = frames;
			frames = 0;
			fpsTimer -= sf::seconds(1);
		}
	}
}

void Game::processEvents()
{
	lastMouseLeftDown = mouseLeftDown;

	// Handling window events.
	sf::Event event;
	while (window.pollEvent(event))
	{
		switch (event.type)
		{
		case sf::Event::MouseMoved:
			mousePosition.x = event.mouseMove.x;
			mousePosition.y = event.mouseMove.y;
			break;
		case sf::Event::MouseButtonPressed:
			if (event.mouseButton.button == sf::Mouse::Left)
				mouseLeftDown = true;
			break;
		case sf::Event::MouseButtonReleased:
			if (event.mouseButton.button == sf::Mouse::Left)
				mouseLeftDown = false;
			break;
		case sf::Event::KeyPressed:
			if (event.key.code == sf::Keyboard::R)
			{
				(ID == 0) ? target = -1 : target = 0;
			}
			else if (event.key.code == sf::Keyboard::G)
			{
				(ID == 1) ? target = -1 : target = 1;
			}
			else if (event.key.code == sf::Keyboard::B)
			{
				(ID == 2) ? target = -1 : target = 2;
			}
			else if (event.key.code == sf::Keyboard::Num1 ||
				event.key.code == sf::Keyboard::Num2 ||
				event.key.code == sf::Keyboard::Num3 ||
				event.key.code == sf::Keyboard::Num4 ||
				event.key.code == sf::Keyboard::Num5 ||
				event.key.code == sf::Keyboard::Num6 ||
				event.key.code == sf::Keyboard::Num7)
			{
				if (target != -1)
				{
					// packet type
					// this client ID
					// target ID
					// spell ID
					// timestamp
					const int BUFFERSIZE = sizeof(char) * 4 + sizeof(unsigned int);
					char cBuffer[BUFFERSIZE];
					int* iBuffer = (int*)&cBuffer[4];
					cBuffer[0] = 's';
					cBuffer[1] = (char)ID;
					cBuffer[2] = (char)target;
					iBuffer[0] = time.asMilliseconds();
					if (event.key.code == sf::Keyboard::Num1)
					{						
						cBuffer[3] = (char)1;
					}
					else if (event.key.code == sf::Keyboard::Num2)
					{
						cBuffer[3] = (char)2;
					}
					else if (event.key.code == sf::Keyboard::Num3)
					{
						cBuffer[3] = (char)3;
					}
					else if (event.key.code == sf::Keyboard::Num4)
					{
						cBuffer[3] = (char)4;
					}
					else if (event.key.code == sf::Keyboard::Num5)
					{
						cBuffer[3] = (char)5;
					}
					else if (event.key.code == sf::Keyboard::Num6)
					{
						cBuffer[3] = (char)6;
					}
					else if (event.key.code == sf::Keyboard::Num7)
					{
						cBuffer[3] = (char)7;
					}
					sf::Socket::Status status = tcpSocket.send(cBuffer, BUFFERSIZE);
					if (status != sf::Socket::Status::Done)
					{
						printf("not sent correctly.");
						assert(false);
					}
				}
			}
			break;
		case sf::Event::Closed:
			window.close();
			break;
		}
	}
}

void Game::listen()
{
	const int BUFFERSIZE = sizeof(char) * 4 + sizeof(int);
	char cBuffer[BUFFERSIZE];

	// Clearing the buffer for receives, just to be safe.
	memset(cBuffer, '-', BUFFERSIZE);

	// TCP
	sf::Socket::Status status;
	size_t bytes;
	do
	{
		status = tcpSocket.receive(cBuffer, BUFFERSIZE, bytes);
		if (status == sf::Socket::Done)
		{
			// If an "connect" packet is received. Add a new layer to the map with the information in the packet.
			if (cBuffer[0] == 'c')
			{
				printf("New player connected with ID: %i", (int)cBuffer[1]);
				players.emplace((int)cBuffer[1], new Player());
			}
			// If a "disconnect" packet is received. Remove the player from the map at the sent ID.
			else if (cBuffer[0] == 'd')
			{
				printf("Player disconnected with ID: %i", (int)cBuffer[1]);
				players.erase((int)cBuffer[1]);
			}
			else if (cBuffer[0] == 's')
			{
				printf("%i\n", (int)cBuffer[3]);
			}
		}
		// Error handling / debugging.
		else if (status == sf::Socket::Partial)
		{
			printf("receive() - Partial\n");
			assert(false);
		}
		else if (status == sf::Socket::Disconnected)
		{
			printf("receive() - Disconnected\n");
			assert(false);
		}
		else if (status == sf::Socket::Error)
		{
			printf("receive() - Error\n");
			assert(false);
		}
	} while (status != sf::Socket::NotReady);

	// UDP
	do
	{
		sf::IpAddress tempServerIP;
		unsigned short tempServerUDPPort;
		status = udpSocket.receive(cBuffer, BUFFERSIZE, bytes, tempServerIP, tempServerUDPPort);
		if (status == sf::Socket::Done)
		{
			// If receiving an "update" packet.
			if (cBuffer[0] == 'u')		
			{
				// If the data is not about this client's snake.
				if (cBuffer[1] != ID)
				{
					// If the player has not disconnected.
					if (players.find(cBuffer[1]) != players.end())
					{
						
					}
				}
			}
		}
		// Error handling / debugging.
		else if (status == sf::Socket::Partial)
		{
			printf("receive() - Partial\n");
			assert(false);
		}
		else if (status == sf::Socket::Disconnected)
		{
			printf("receive() - Disconnected\n");
			assert(false);
		}
		else if (status == sf::Socket::Error)
		{
			printf("receive() - Error\n");
			assert(false);
		}
	} while (status != sf::Socket::NotReady);
}

void Game::handleInput()
{
	// When the left mouse down state changes, set the speed of the snake accordingly.
	if (mouseLeftDown == true && lastMouseLeftDown == false)
	{
		
	}
	else if (mouseLeftDown == false && lastMouseLeftDown == true)
	{
		
	}
}

void Game::update(float deltaTime)
{
	// For every other player.
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		if (it->first != ID)
		{

		}
	}
}

void Game::sendUpdate(int time)
{
	const int BUFFERSIZE = sizeof(char) * 2;
	char cBuffer[BUFFERSIZE];

	Player* player = players.at(ID);

	// Writing data about this client's snake to the buffer.
	memset(cBuffer, '-', BUFFERSIZE);
	cBuffer[0] = 'u';
	cBuffer[1] = (char)ID;

	// Sending a packet containing this buffer.
	udpSocket.send(cBuffer, BUFFERSIZE, serverIP, serverPortUDP);
}

void Game::render()
{
	// Clear the screen.
	if (ID == 0)
	{
		window.clear(sf::Color::Red);
	}
	else if (ID == 1)
	{
		window.clear(sf::Color::Green);
	}
	else if (ID == 2)
	{
		window.clear(sf::Color::Blue);
	}
	else
	{
		window.clear();
	}

	// Call every snake's render function.
	std::map<int, Player*>::iterator it;
	for (it = players.begin(); it != players.end(); it++)
	{
		//it->second->render(&window);
	}

	window.display();
}


void Game::connect()
{
	// Attempting to connect to the server.
	sf::Socket::Status status = tcpSocket.connect(serverIP, serverPortTCP);
	// Error handling / debugging.
	if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Partial");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Disconnected");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Error");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - NotReady");
		assert(false);
	}
}

void Game::receiveID()
{
	// Receiving a packet from the server containing this client's ID.
	const int BUFFERSIZE = sizeof(char) * 2;
	char cBuffer[BUFFERSIZE];

	status = tcpSocket.receive(cBuffer, BUFFERSIZE, bytes);
	if (status == sf::Socket::Done)
	{
		// Settin the ID of this client.
		ID = (int)cBuffer[1];
		// Setting the position of the window based on this information for debugging with multiple clients on one machine.
		window.setPosition(sf::Vector2i(windowWidth * (ID % 4), windowHeight * (int)(ID / 4)));
	}
	// Error handling / debugging.
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Partial");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Disconnected");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Error");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - NotReady");
		assert(false);
	}
}

void Game::sendUDPPort()
{
	// Sending the UDP port address of this client.
	const int BUFFERSIZE = sizeof(char) * 2 + sizeof(unsigned short);
	char cBuffer[BUFFERSIZE];
	unsigned short* usBuffer = (unsigned short*)& cBuffer[2];
	memset(cBuffer, '-', BUFFERSIZE);

	usBuffer[0] = udpSocket.getLocalPort();
	sf::Socket::Status status = tcpSocket.send(cBuffer, BUFFERSIZE);
	// Error handling / debugging.
	if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Partial");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Disconnected");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - Error");
		assert(false);
	}
	else if (status == sf::Socket::Partial)
	{
		printf("ERROR: connect() - NotReady");
		assert(false);
	}
}

void Game::syncTime()
{
	// OPTIMIZE...!
	const int BUFFERSIZE = sizeof(int);
	char cBuffer[BUFFERSIZE];
	int* iBuffer = (int*)&cBuffer[0];
	memset(cBuffer, '-', BUFFERSIZE);

	// Relaying empty packets between the client and server.
	// The server uses the time between these sends to calculate latency.
	for (int i = 0; i < 5; i++)
	{
		if (tcpSocket.receive(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
		{
			printf("Error: send()");
			assert(false);
		}
		if (tcpSocket.send(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
		{
			printf("Error: receive()");
			assert(false);
		}
	}

	// Receiving server time.
	if (tcpSocket.receive(cBuffer, BUFFERSIZE, bytes) != sf::Socket::Status::Done)
	{
		printf("Error: send()");
		assert(false);
	}
	// Syncing the clients time to the server.
	startTime = iBuffer[0];
	time = sf::milliseconds(startTime);
}