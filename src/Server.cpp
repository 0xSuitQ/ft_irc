#include "Server.hpp"

Server::Server() {
	this->_server_socket = -1;
	this->_port = 52525;
	createSocket();
}

Server::~Server() {
	if (_server_socket != -1)
		close(_server_socket);
}

void	Server::createSocket() {
	sockaddr_in service;
	int			yes = 1;
	struct pollfd tmp;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(_port);
	
	_server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (_server_socket == -1)
		throw SocketCreationException();
	setsockopt(_server_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
	if (bind(_server_socket, (sockaddr *)&service, sizeof(service)) == -1)
		throw BindException();
	if (listen(_server_socket, SOMAXCONN) == -1)
		throw ListenException();
	fcntl(_server_socket, F_SETFL, O_NONBLOCK);
	tmp.fd = _server_socket;
	tmp.events = POLLIN;
	_pfds.push_back(tmp);
	_fd_count = 1;
	_mainLoop();
}

void	Server::_handleNewConnection() {
	char			buf[] = "Welcome\n";
	struct pollfd	new_client;

	int new_fd = accept(_server_socket, NULL, NULL);
	if (new_fd == -1)
		throw AcceptException();
	else {
		// Adding new client
		fcntl(new_fd, F_SETFL, O_NONBLOCK);
	
		Client client = Client(new_fd);

		_clients.push_back(client);
		new_client.fd = new_fd;
		new_client.events = POLLIN;
		_pfds.push_back(new_client);
		_fd_count++;
		std::cout << "New client is connected" << std::endl;
		if (send(new_fd, buf, sizeof(buf), 0) == -1) {
			perror("Error: send()");
		}
	}
}

void Server::_mainLoop() {
	for (;;) {
		int poll_count = poll(&_pfds[0], _pfds.size(), -1);
		if (poll_count == -1)
			throw PollCountException();

		for (size_t i = 0; i < _pfds.size(); i++) {
			if (_pfds[i].revents & POLLIN) {
				if (_pfds[i].fd == _server_socket)
					_handleNewConnection();
				else
					_handleData(_pfds[i].fd);
			}
		}
	}
}

void Server::_handleData(int sender_fd) {
	char buf[1024];
	int nbytes;

	nbytes = recv(sender_fd, buf, sizeof(buf) - 1, 0);
	if (nbytes > 0) {
		buf[nbytes] = '\0';

		Client& client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
		client.appendToBuffer(buf, nbytes); // Filling up the buffer till it has \n in it
		std::string message;
		while (client.getCompleteMessage(message)) { // Is executed if only \n was not found in the received data
			std::cout << "Received complete message: " << message << std::endl;

			// Send to all clients except the sender
			for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
				if (it->getFd() != sender_fd && it->getFd() != _server_socket)
					if (send(it->getFd(), message.c_str(), message.length(), 0) == -1)
						perror("Error: send()");
			}
		}
	} else if (nbytes == 0) {
		// Client disconnected
		std::cout << "Client on socket " << sender_fd << " disconnected." << std::endl;
		_removeClient(sender_fd);
	} else {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			_removeClient(sender_fd); // TODO: mb concider RecvException in here as well
		}
	}
}

void Server::_removeClient(int fd) {
	// Remove from _clients vector
	_clients.erase(std::remove_if(_clients.begin(), _clients.end(), CompareClientFd(fd)), _clients.end());

	// Remove from _pfds vector
	_pfds.erase(std::remove_if(_pfds.begin(), _pfds.end(), ComparePollFd(fd)), _pfds.end());

	close(fd);
	_fd_count--;
}