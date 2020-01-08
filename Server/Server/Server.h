#pragma once
#include <SFML/Network.hpp>
#include <SFML/Graphics.hpp>
#include <iostream>
#include <cassert>
#include <list>
#include "Client.h"

class Server
{
public:
	Server();
	~Server();

	// Main loop of the server.
	void run();

	// Returns the lowest available client ID.
	int allocateID();

	// Sends an empty packet too and from the client, calculates the average time to travel one way,
	// sends the client the server's time + the latency to sync the client's time with the server's.
	void syncTime(Client* client);

	// It should not send repeated update packets... this is the cause of skipping on packet drop tests!!!
	//void broadcastTCP();

	// Receives a packet from the client containing it's UDP port address and stores it in the Client struct.
	void receiveUDPPort(Client* client);

private:
	// Setting up a buffer to store raw data of different types.
	/*static const int BUFFERSIZE = sizeof(char) * 2 + sizeof(float) * 4 + sizeof(int) * 5 + sizeof(bool);
	char cBuffer[BUFFERSIZE];
	float* fBuffer = (float*)&cBuffer[2];
	int* iBuffer = (int*)&fBuffer[4];
	bool* bBuffer = (bool*)&iBuffer[5];*/

	// Setting up IP and port address'
	sf::IpAddress serverIP = sf::IpAddress::getLocalAddress();
	unsigned short serverPortTCP = 4444;
	unsigned short serverPortUDP = 4445;

	// Declaring sockets.
	sf::TcpListener tcpListener;
	sf::UdpSocket udpSocket;
	sf::SocketSelector selector;

	std::map<int, Client*> clients;

	// Variables to hold temporary information generated from socket functions.
	sf::Socket::Status status;
	std::size_t bytes;
	sf::IpAddress tempClientIP;
	unsigned short tempClientUDPPort;

	// Clocks to keep track of the server time.
	sf::Clock clock;
	sf::Clock deltaTimeClock;
	sf::Time broadcastTimer = sf::Time::Zero;
	float broadCastInterval = 0.05f;
};

