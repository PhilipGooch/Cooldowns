#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cassert>
#include <list>

// Each client has a TCP socket and a UDP port address.
struct Client
{
	sf::TcpSocket TCPSocket;
	unsigned short UDPPort;
};

// Holds information about the snakes head that will be sent accross the network.
class Snake
{
public:
	Snake(int ID, int length, float positionX, float positionY, float directionX, float directionY, int r, int g, int b) :
		ID(ID),
		length(length),
		positionX(positionX),
		positionY(positionY),
		directionX(directionX),
		directionY(directionY),
		r(r),
		g(g),
		b(b),
		boosting(false),
		timeStamp(0)
	{
	}

	int ID;
	float positionX;
	float positionY;
	float directionX;
	float directionY;
	int length;
	int r, g, b;
	bool boosting;
	int timeStamp;
};

class Server
{
public:
	Server();
	~Server();

	// Writes data to the buffer that will be sent accross the network. It uses data from the given snake pointer.
	void writeToBuffer(char type, Snake* snake);

	// Calls writeToBuffer depending on the type of message being sent and sends the data in the buffer to the given client.
	void sendPacket(sf::TcpSocket* client, char type, Snake* snake);

	// Main loop of the server.
	void run();

	// Generates a snake that is sent to the client on connection.
	Snake* generateSnake(int ID);

	// Returns the lowest available client ID.
	int allocateID();

	// Sends an empty packet too and from the client, calculates the average time to travel one way,
	// sends the client the server's time + the latency to sync the client's time with the server's.
	void syncTime(Client* client);

	// Broadcasts the most recent data about each snake to each client every 0.05 seconds.
	// It should not send repeated update packets... this is the cause of skipping on packet drop tests!!!
	void broadcast();

	// Receives a packet from the client containing it's UDP port address and stores it in the Client struct.
	void receiveUDPPort(Client* client);

private:
	// Setting up a buffer to store raw data of different types.
	static const int BUFFERSIZE = sizeof(char) * 2 + sizeof(float) * 4 + sizeof(int) * 5 + sizeof(bool);
	char cBuffer[BUFFERSIZE];
	float* fBuffer = (float*)&cBuffer[2];
	int* iBuffer = (int*)&fBuffer[4];
	bool* bBuffer = (bool*)&iBuffer[5];

	// Setting up IP and port address'
	sf::IpAddress serverIP = sf::IpAddress::getLocalAddress();
	unsigned short serverPortTCP = 4444;
	unsigned short serverPortUDP = 4445;

	// Declaring sockets.
	sf::TcpListener tcpListener;
	sf::SocketSelector selector;
	sf::UdpSocket udpSocket;

	// Declaring maps that will store information about the client and its corresponding snake.
	// The ID's will correspond so arguably, client and snake could be the same object, or client could contain a snake object...
	std::map<int, Client*> clients;
	std::map<int, Snake*> snakes;

	// Variables to hold temporary information generated from socket functions.
	sf::Socket::Status status;
	std::size_t bytes;
	sf::IpAddress tempClientIP;
	unsigned short tempClientUDPPort;
	sf::Packet packet;

	// Clocks to keep track of the server time.
	sf::Clock clock;
	sf::Clock deltaTimeClock;
	sf::Time broadcastTimer = sf::Time::Zero;
	float broadCastInterval = 0.05f;

	// Colours for the snakes.
	sf::Color colours[6] = { sf::Color::Red,
							 sf::Color::Green,
							 sf::Color::Blue,
							 sf::Color::Yellow,
							 sf::Color::Magenta,
							 sf::Color::Cyan };

	//bool collision(sf::Vector2f position1, sf::Vector2f position2, float radius1, float radius2);

};

