#include "Server.hpp"
#include "Channel.hpp"

Server::Server(char **av) {
	this->_server_socket = -1;
	this->_port = -1;
	try {
        setPort(std::stoi(av[1])); //parser checks
    } catch (const std::invalid_argument& ia) {
        std::cerr << "Invalid argument: " << ia.what() << '\n';
        return;
    } catch (const std::out_of_range& oor) {
        std::cerr << "Out of Range error: " << oor.what() << '\n';
        return;
    }
	setPass(av[2]);
	createSocket();
}

Server::~Server() {
	if (_server_socket != -1)
		close(_server_socket);
}

void	Server::setPass(std::string pass) {
	if (this->_server_pass.empty()) {
		_server_pass = pass;
	} else {
		perror("Server password is already set up");
	}
}

void	Server::setPort(int port) {
	if (this->_port == -1) {
		_port = port;
	} else {
		perror("Server password is already set up");
	}
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

void Server::_parse_cmd(std::string& message, int sender_fd) {
	std::map<std::string, CommandFunc> preset_cmds;
    preset_cmds["PASS"] = &Server::_pass;
    preset_cmds["USER"] = &Server::_user;
	preset_cmds["NICK"] = &Server::_nick;
	preset_cmds["JOIN"] = &Server::_join;
	std::vector<std::string> cmds = _split(message);
	if (cmds.empty())
		return;
	std::map<std::string, CommandFunc>::iterator cmd_func = preset_cmds.find(cmds[0]);
    if (cmd_func != preset_cmds.end()) {
        (this->*(cmd_func->second))(message, sender_fd);
    } else {
        // Handle unknown command
		return;
    }
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
		while (client.getCompleteMessage(message)) { // Is executed if only \n was found in the received data
			std::cout << "Received complete message: " << message << std::endl;

			// Send to all clients except the sender
			for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
				if (it->getFd() != sender_fd && it->getFd() != _server_socket)
					if (send(it->getFd(), message.c_str(), message.length(), 0) == -1)
						perror("Error: send()");
			}

			// Parsing of the data
			_parse_cmd(message, sender_fd);

		}
	} else if (nbytes == 0) {
		// Client disconnected
		std::cout << "Client on socket " << sender_fd << " disconnected." << std::endl;
		_removeClient(sender_fd);
	} else {
		if (errno != EAGAIN && errno != EWOULDBLOCK) {
			_removeClient(sender_fd);
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

std::vector<std::string> Server::_split(std::string& str) {
	std::istringstream			iss(str);
	std::vector<std::string>	commands;
	std::string					command;

	while (iss >> command)
		commands.push_back(command);
	return commands;
}

void	Server::_pass(std::string& message, int sender_fd) {
	message = message.substr(4);
	Client& client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	size_t pos = message.find_first_not_of(" \t\v\f");
    if (pos != std::string::npos) {
        message = message.substr(pos);
    } else {
        message = "";
    }

	if (!client.getAuth()) {
		if (message == _server_pass) {
			client.setAuth(true);
			sendResponse("Succesfully authenticated\n", client.getFd());
		}
		else
			sendResponse("Incorrect password\n", client.getFd());
	} else
			sendResponse("Already authenticated\n", client.getFd());
}

bool	Server::_validateUser(std::string& name, int flag, int fd) const { // flag 0 == username, flag 1 == nickname
	std::string forbidden = "!@#$%^&*()?/\\{}[]-<>,'\"|+;:";
	if (std::find_if(name.begin(), name.end(), ::isspace) != name.end()) {
		(flag == 0) ? sendResponse("Nickname cannot contain whitespaces\n", fd) : sendResponse("Username cannot contain whitespaces\n", fd);
		return false;
	}
	if (name.size() < 5) {
		(flag == 0) ? sendResponse("Nickname is too short\n", fd) : sendResponse("Username is too short\n", fd);
		return false;
	}
	if (name.size() > 12) {
		(flag == 0) ? sendResponse("Nickname is too long\n", fd) : sendResponse("Username is too long\n", fd);
		return false;
	}
	size_t pos = name.find_first_of(forbidden);
	if (pos != std::string::npos) {
		(flag == 0) ? sendResponse("Nickname contains forbidden symbols\n", fd) : sendResponse("Username contains forbidden symbols\n", fd);
		return false;
	}

	return true;
}

void	Server::_user(std::string& message, int sender_fd) {
	message = message.substr(4);
	Client& client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	size_t pos = message.find_first_not_of(" \t\v\f");
	if (pos != std::string::npos) {
		message = message.substr(pos);
	} else {
		message = "";
		sendResponse("Username cannot be empty\n", client.getFd());
		return;
	}

	std::vector<Client>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientUser(message));
	if (it != _clients.end() && it->getFd() != sender_fd) {
		sendResponse("Username is already taken\n", client.getFd());
	} else {
		// Username is not occupied or is occupied by the same client
		if (!client.getUsername().empty()){
			sendResponse("Username cannot be changed\n", client.getFd());
		}
		else {
			if (_validateUser(message, 0, client.getFd())) {
				client.setUsername(message);
				sendResponse("Username is succesfully set up\n", client.getFd());
			}
		}
	}
}

void	Server::_nick(std::string& message, int sender_fd) {
	message = message.substr(4);
	Client& client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	size_t pos = message.find_first_not_of(" \t\v\f");
	if (pos != std::string::npos) {
		message = message.substr(pos);
	} else {
		message = "";
		sendResponse("Nickname cannot be empty\n", client.getFd());
		return;
	}

	std::vector<Client>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(message));
	if (it != _clients.end() && it->getFd() != sender_fd) {
		sendResponse("Nickname is already taken\n", client.getFd());
	} else {
		// Nickname is not occupied or is occupied by the same client
		if (message == client.getNickname()){
			sendResponse("Nickname cannot be the same\n", client.getFd());
		}
		else {
			if (_validateUser(message, 1, client.getFd())) {
				client.setNickname(message);
				sendResponse("Nickname is succesfully set up\n", client.getFd());
			}
		}
	}
}

// Usage: JOIN <channel name> <password (optional)>
void	Server::_join(std::string& msg, int sender_fd) { // TODO: parsing error handling
	std::vector<std::string> splitted_cmd = _split(msg);
	Client& client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	if (!_checkAuth(client))
		return;

	std::vector<Channel>::iterator it = std::find_if(_channels.begin(), _channels.end(), CompareChannelName(splitted_cmd[0]));
	if (!it->validateUserCreds(client, sender_fd))
		return;

	std::string password = "";
    if (splitted_cmd.size() > 1) {
        password = splitted_cmd[1];
    }

	if (it == _channels.end()) {
		Channel new_channel(splitted_cmd[0], client);
		sendResponse("No channels found, new channel has been created\n", sender_fd);
		_channels.push_back(new_channel);
	} else {
		it->addClientToChannel(client, splitted_cmd[1], sender_fd, 0);
	}
}

bool	Server::_checkAuth(Client& client) {
	if (!client.getAuth()) {
		sendResponse("You are not authorized to do that. Enter the password by typing <PASS> <password>\n", client.getFd());
		return false;
	}
	return true;
}