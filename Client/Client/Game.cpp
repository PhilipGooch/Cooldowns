#include "Game.h"
#include <fstream>

Game::Game() :
	windowWidth(280.0f),
	windowHeight(280.0f),
	window(sf::VideoMode((unsigned int)windowWidth, (unsigned int)windowHeight), "", sf::Style::Close),
	mousePosition(sf::Vector2i()),
	mouseLeftDown(false),
	lastMouseLeftDown(false),
	frameCount(0),
	fps(0),
	tickSpeed(10),
	center(sf::Vector2f(windowWidth / 2, windowHeight / 2)),
	timeConstant(sf::seconds(1 / fpsConstant)),
	ID(-1)
{
	//font.loadFromFile("arial.ttf");
	//text.setString(std::to_string(fps));

	//text.setCharacterSize(30);
	//text.setPosition(windowWidth - 80, windowHeight - 50);
	//text.setOutlineColor(sf::Color::White);
	//text.setFont(font);

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

	// Receiving all connected snake's / clients' info and adding them to map.
	memset(cBuffer, '-', BUFFERSIZE);
	while (true)
	{
		status = tcpSocket.receive(cBuffer, BUFFERSIZE, bytes);
		if (status == sf::Socket::Done)
		{
			if (cBuffer[0] == 'a')
			{
				snakes.emplace((int)cBuffer[1], new Snake(sf::Vector2f(fBuffer[0], fBuffer[1]), iBuffer[0], iBuffer[0], sf::Color(iBuffer[1], iBuffer[2], iBuffer[3]), (int)cBuffer[1]));
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
		updateTimer += deltaTime;
		time += deltaTime;
		fpsTimer += deltaTime;
		updatePacketTimer += deltaTime;
		// Do multiple updates before rendering if the time has not reached the fixed time step time. 
		// This ensures local the local client snake will allways be moving a fixed amount in any direction,
		// this will mean the tail will not distort and will follow the head nicely. (This all goes to sh*t when using prediction from network positions!)
		while (updateTimer > timeConstant)
		{
			updateTimer -= timeConstant;
			processEvents();
			listen();
			handleInput();
			update(timeConstant.asSeconds());
		}
		render();
		// Sending updated local position packets to the server every send interval.
		if (updatePacketTimer > sf::seconds(sendInterval))
		{
			sendUpdate(clock.getElapsedTime().asMilliseconds() + startTime);
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
	// Storing state of mouse down state to do comparisons when boosting the snake.
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
		case sf::Event::Closed:
			window.close();
			break;
		}
	}
}

void Game::listen()
{
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
			// If an "add snake" packet is received. Add a new snake to the map with the information in the packet.
			if (cBuffer[0] == 'a')
			{
				snakes.emplace((int)cBuffer[1], new Snake(sf::Vector2f(fBuffer[0], fBuffer[1]), iBuffer[0], iBuffer[0], sf::Color(iBuffer[1], iBuffer[2], iBuffer[3]), (int)cBuffer[1]));
			}
			// If a "remove snake" packet is received. Remove the snake from the map at the sent ID 
			else if (cBuffer[0] == 'r')
			{
				snakes.erase((int)cBuffer[1]);
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
					// If the snake / client has not disconnected.
					if (snakes.find(cBuffer[1]) != snakes.end())
					{
						// Creating a confiremed state struct and populating it with information from the packet.
						ConfirmedState confirmedState;
						confirmedState.position.x = fBuffer[0];
						confirmedState.position.y = fBuffer[1];
						confirmedState.direction.x = fBuffer[2];
						confirmedState.direction.y = fBuffer[3];
						confirmedState.timeStamp = iBuffer[4];
						confirmedState.boosting = bBuffer[0];

						// Setting the direction of the snake to the latest packet's direction variables. Just used for the direction of the eyes currently... 
						snakes.at(cBuffer[1])->setDirection(confirmedState.direction);

						// Setting the speed of the snake with the boositing variable from the update packet.
						snakes.at(cBuffer[1])->boost(confirmedState.boosting);
						
						// Adding this to the specified snake's deque of confirmed states, to be used for prediction.
						snakes.at(cBuffer[1])->addConfirmedPosition(confirmedState);
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
		snakes[ID]->boost(true);
	}
	else if (mouseLeftDown == false && lastMouseLeftDown == true)
	{
		snakes[ID]->boost(false);
	}
}

void Game::update(float deltaTime)
{
	// Calculating the normalized direction vector of the mouse from the centre of the screen.
	sf::Vector2f lastDirection = direction;
	direction = sf::Vector2f(mousePosition.x - windowWidth / 2, mousePosition.y - windowHeight / 2);
	if (direction == sf::Vector2f(0, 0))
		direction = lastDirection;
	direction = normalize(direction);

	// Updating the local snake with the direction vector directly.
	snakes.at(ID)->setDirection(direction);
	snakes.at(ID)->update(deltaTime);

	// For every other snake, do a prediction update using the data sent from the server.
	std::map<int, Snake*>::iterator it;
	for (it = snakes.begin(); it != snakes.end(); it++)
	{
		if (it->first != ID)
		{
			// This function requires the client's time.
			it->second->predictionUpdate(deltaTime, clock.getElapsedTime().asMilliseconds() + startTime);
		}
	}
}

void Game::sendUpdate(int time)
{
	Snake* snake = snakes.at(ID);

	// Writing data about this client's snake to the buffer.
	memset(cBuffer, '-', BUFFERSIZE);
	cBuffer[0] = 'u';
	cBuffer[1] = (char)ID;
	fBuffer[0] = snake->getPosition().x;
	fBuffer[1] = snake->getPosition().y;
	fBuffer[2] = snake->getDirection().x;
	fBuffer[3] = snake->getDirection().y;
	iBuffer[4] = time;
	bBuffer[0] = snake->getBoosting();

	// Sending a packet containing this buffer.
	udpSocket.send(cBuffer, BUFFERSIZE, serverIP, serverPortUDP);
}

void Game::render()
{
	// Clear the screen.
	window.clear();

	// Call every snake's render function.
	std::map<int, Snake*>::iterator it;
	for (it = snakes.begin(); it != snakes.end(); it++)
	{
		it->second->render(&window, snakes[ID]->getPosition(), center);
	}

	//window.draw(text);

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
	unsigned short* usBuffer = (unsigned short*)& cBuffer[1];
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
	startTime = iBuffer[3];
}