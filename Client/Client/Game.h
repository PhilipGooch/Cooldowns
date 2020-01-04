#pragma once
#include "Resources.h"
#include <SFML/Network.hpp>
#include <random>
#include <iostream>
#include "Snake.h"

class Game
{

public:
	Game();
	~Game();

	void run();

private:

	// Handling window events.
	void processEvents();
	
	// Listening for incoming network packets.
	void listen();

	// Handling user input.
	void handleInput();

	// Updating the snakes positions.
	void update(float deltaTime);

	// Sending this client's local position to the server.
	void sendUpdate(int time);

	// Drawing everything.
	void render();

	// Attempting to connect to the server.
	void connect();

	// Receives and sets its ID. 
	// Sets the position of the window based on this information for debugging with multiple clients on one machine.
	void receiveID();

	// Sends the UDP address of this client.
	void sendUDPPort();

	// Sends an empty packet too and from the client, calculates the average time to travel one way,
	// sends the client the server's time + the latency to sync the client's time with the server's.
	void syncTime();

private:
	float windowWidth;
	float windowHeight;
	sf::RenderWindow window;
	sf::Vector2i mousePosition;
	bool mouseLeftDown;
	bool lastMouseLeftDown;
	float frameCount;
	int fps = 0;
	float tickSpeed;
	sf::Vector2f center;
	float fpsConstant = 60.0f;
	sf::Time timeConstant;
	int ID;
	//sf::Font font;		
	//sf::Text text;

	// Variables to hold temporary information generated from socket functions.
	sf::Socket::Status status;
	std::size_t bytes;

	// Setting up IP and port address'
	sf::IpAddress  serverIP;
	unsigned short serverPortTCP = 4444;
	unsigned short serverPortUDP = 4445;

	// Declaring sockets.
	sf::UdpSocket udpSocket;
	sf::TcpSocket tcpSocket;

	// Setting up a buffer to store raw data of different types.
	static const int BUFFERSIZE = sizeof(char) * 2 + sizeof(float) * 4 + sizeof(int) * 5 + sizeof(bool);
	char cBuffer[BUFFERSIZE];
	float* fBuffer = (float*)& cBuffer[2];
	int* iBuffer = (int*)& fBuffer[4];
	bool* bBuffer = (bool*)& iBuffer[5];

	// Time variables.
	int startTime;
	sf::Clock clock;
	sf::Time time;
	float sendInterval = 0.05f;

	// Map of all the snakes in the game.
	std::map<int, Snake*> snakes;

	// Direction vector of the client's mouse.
	sf::Vector2f direction;
	bool boosting;
};

