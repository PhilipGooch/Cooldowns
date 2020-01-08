#pragma once
#include <SFML/Network.hpp>

class Client
{
public:
	Client()
	{
		
	}

	// Each client has a TCP socket and a UDP port address.
	sf::TcpSocket TCPSocket;
	unsigned short UDPPort = 0;

	int ID = -1;
	int timeStamp = 0;
};

