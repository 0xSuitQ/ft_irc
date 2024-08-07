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
	int			new_fd;
	char		buf[256];
	sockaddr_in service;
	int			yes = 1;
	struct pollfd tmp;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(_port);
	
	_serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_serverSocket == -1)
		throw SocketCreationException();
	setsockopt(_serverSocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (bind(_serverSocket, (sockaddr *)&service, sizeof(service)) == -1)
		throw BindException();
	if (listen(_serverSocket, SOMAXCONN) == -1)
		throw ListenException();
	fcntl(_serverSocket, F_SETFL, O_NONBLOCK);
	tmp.fd = _serverSocket;
	tmp.events = POLLIN;
	_pfds.push_back(tmp);
	_mainLoop();
}

void	Server::_addClient(int new_fd) {
	fcntl(new_fd, F_SETFL, O_NONBLOCK);
	
	Client client = Client(new_fd);
	struct pollfd	newClient;

	_clients.push_back(client);
	newClient.fd = new_fd;
	newClient.events = POLLIN;
	_pfds.push_back(newClient);
}

void	Server::_handleNewConnection() {
	int new_fd = accept(_serverSocket, NULL, NULL);
	if (new_fd == -1)
		throw AcceptException();
	else
		_addClient(new_fd);
}

void	Server::_mainLoop() {	
	for (;;) {
		int	poll_count = poll(&_pfds[0], _pfds.size(), -1); //forgot to allocate memory for pfds
		if (poll_count = -1)
			throw PollCountException();
		
		for (int i = 0; i < poll_count; i++) {
			if (_pfds[i].revents & POLLIN) {
				if (_pfds[i].fd == _serverSocket)
					_handleNewConnection();
				else
					_hadleData(_pfds[i].fd);
			}
		}
	}
}

void	Server::_hadleData(int fd) {
	char	buf[1024];
	int nbytes = recv(fd, buf, sizeof(buf), 0);
	
	if (nbytes <= 0) {
		// TODO: remove all the data related to the client
	} else {

	}
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