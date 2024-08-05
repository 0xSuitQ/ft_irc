#include "Server.hpp"

Server::Server() {
	this->_serverSocket = -1;
	this->_port = 52525;
}

Server::~Server() {
	if (_serverSocket != -1)
		close(_serverSocket);
}

void	Server::createSocket() {
	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(_port);
	
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_serverSocket == -1)
		throw SocketCreationException();
	
	if (bind(_serverSocket, (sockaddr *)&service, sizeof(service)) == -1)
		throw BindException();
	
	if (listen(_serverSocket, SOMAXCONN) == -1)
		throw ListenException();
}

void	Server::acceptClient() {
	pollfd client;
	int acceptSocket = accept(_serverSocket, NULL, NULL);
	if (acceptSocket == -1)
		throw FailedConnectionException();
	_clientSockets.push_back(acceptSocket);
	client.fd = acceptSocket;
	client.events = POLLIN;
}