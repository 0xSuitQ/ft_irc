#include "Server.hpp"
#include "Channel.hpp"

// TODO: Cleaning client after disconecting
// TODO: Add HELP command to list all commands
// TODO: Orthodox canonical form of the classes

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

void	Server::_handleNewConnection() {
	char			buf[] = ":ircserv 001 :Welcome to the IRC Network,!\n";
	struct pollfd	new_client;

	int new_fd = accept(_server_socket, NULL, NULL);
	if (new_fd == -1)
		throw AcceptException();
	else {
		// Adding new client
		fcntl(new_fd, F_SETFL, O_NONBLOCK);
	
		Client *client = new Client(new_fd);

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

void Server::_parse_cmd(std::string& message, int sender_fd) {
	std::map<std::string, CommandFunc> preset_cmds;
    preset_cmds["PASS"] = &Server::_pass;
    preset_cmds["USER"] = &Server::_user;
	preset_cmds["NICK"] = &Server::_nick;
	preset_cmds["JOIN"] = &Server::_join;
	preset_cmds["INVITE"] = &Server::_invite;
	preset_cmds["KICK"] = &Server::_kick;
	// preset_cmds["LEAVE"] = &Server::_leave;
	preset_cmds["TOPIC"] = &Server::_topic;
	preset_cmds["MODE"] = &Server::_mode;
	preset_cmds["DM"] = &Server::_directMessage;
	preset_cmds["CAP"] = &Server::_capLs;
	std::vector<std::string> cmds = _split(message);
	if (cmds.empty())
		return;
	std::map<std::string, CommandFunc>::iterator cmd_func = preset_cmds.find(cmds[0]);
    if (cmd_func != preset_cmds.end()) {
        (this->*(cmd_func->second))(message, sender_fd);
    } else {
        // Handle unknown command
		Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
		std::cout << "CLient in channel" << client->getInChannel() << "\n";
		if (client->getInChannel()) {
			_client_channel[client]->debugPrint();
			_client_channel[client]->broadcastMessage(*client, message);
		}
		else
			sendResponse("Unknown command or not connected to a channel\n", sender_fd);
		return;
    }
}

void Server::_handleData(int sender_fd) {
	char buf[1024];
	int nbytes;

	nbytes = recv(sender_fd, buf, sizeof(buf) - 1, 0);
	if (nbytes > 0) {
		buf[nbytes] = '\0';

		Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
		client->appendToBuffer(buf, nbytes); // Filling up the buffer till it has \n in it
		std::string message;
		while (client->getCompleteMessage(message)) { // Is executed if only \n was found in the received data
			std::cout << "Received complete message: " << message << std::endl;

			// Send to all clients except the sender
			// for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			// 	if (it->getFd() != sender_fd && it->getFd() != _server_socket)
			// 		if (send(it->getFd(), message.c_str(), message.length(), 0) == -1)
			// 			perror("Error: send()");
			// }

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

void	Server::_invite(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	Client* client_to_invite = *std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[1]));
	if (splitted_cmd.size() != 2) {
		sendResponse("Wrong number of parameters\n", sender_fd);
		return;
	}
	std::vector<Client*>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[1]));

	//Check that user is allowed to invite
	//Check that user to invite is present and is authorized
	if (!_checkAuth(*client, sender_fd, 0))
		return;
	if (!validateUserCreds(*client, sender_fd))
		return;
	if (_client_channel.find(client) == _client_channel.end()) {
		sendResponse("You are not connected to any channel\n", sender_fd);
		return;
	}
	if (_client_channel[client]->getInviteOnly() && !_client_channel[client]->isOperator(*client)) {
		sendResponse("You are not allowed to do that\n", sender_fd);
		return;
	}

	if (client == client_to_invite) {
		sendResponse("Wrong client nickname\n", sender_fd);
		return;
	}
	if (it == _clients.end()) {
		sendResponse("Client not found\n", sender_fd);
		return;
	}
	if (!_checkAuth(*client_to_invite, sender_fd, 1))
		return;
	if (!validateUserCreds(*client_to_invite, sender_fd))
		return;
	if (it != _clients.end() && client_to_invite->getInChannel()) {
		_client_channel[client_to_invite]->removeClientFromChannel(*client_to_invite);
		sendResponse("You left the old channel\n", client_to_invite->getFd());
	}
	if (!_client_channel[client]->addClientToChannel(*client_to_invite, client_to_invite->getFd(), 1)) {
		sendResponse("Unable to connect client to the channel.\n", sender_fd);
		return;
	}
	_client_channel[client_to_invite] = _client_channel[client]; // TODO: client doesn't see if person to invite is connected or no.
	sendResponse("You have been invited to the channel\n", client_to_invite->getFd());
	sendResponse("Client has been invited to the channel\n", sender_fd);
}

void Server::sendPrivateMessage(Client& sender, Client& receiver, const std::string& message) {
	if (sender != receiver) {
		receiver.receiveMessage(sender.getNickname());
		receiver.receiveMessage(message);
	}
}

void Server::_removeClient(int fd) {
	// 
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
	// message = message.substr(4);
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	// size_t pos = message.find_first_not_of(" \t\v\f");
    // if (pos != std::string::npos) {
    //     message = message.substr(pos);
    // } else {
    //     message = "";
    // }

	if (splitted_cmd.size() != 2) {
		sendResponse("Invalid amount of arguments. Use \"PASS <password>\".\n", sender_fd);
	}

	if (!client->getAuth()) {
		if (splitted_cmd[1] == _server_pass) {
			client->setAuth(true);
			sendResponse("Succesfully authenticated\n", client->getFd());
		}
		else
			sendResponse("Incorrect password.\n", client->getFd());
	} else
			sendResponse("Already authenticated.\n", client->getFd());
}

bool	Server::_validateName(std::string& name, int fd, std::string target, int flag) const {
	// flag 0 == username/nickname, flag 1 == channel name, flag 2 == channel pass

	// std::string forbidden = "!@#$%^&*()?/\\{}[]-<>,'\"|+;:";
	std::string forbidden = "!@$%^&*()?/\\{}[]-<>,'\"|+;:";
	std::string msg = "";
	if (std::find_if(name.begin(), name.end(), ::isspace) != name.end()) {
		msg = target + " cannot contain whitespaces\n";
		sendResponse(msg, fd);
		return false;
	}
	if (name.size() < 4) {
		msg = target + " is too short\n";
		sendResponse(msg, fd);
		return false;
	}
	if (flag == 0 && name.size() > 12) {
		msg = target + " is too long\n";
		sendResponse(msg, fd);
		return false;
	} else if (flag == 1 && name.size() > 20){
		msg = target + " is too long\n";
		sendResponse(msg, fd);
		return false;
	}
	if (flag != 2) {
		size_t pos = name.find_first_of(forbidden);
		if (pos != std::string::npos) {
			msg = target + " contains forbidden symbols\n";
			sendResponse(msg, fd);
			return false;
		}
	}

	return true;
}

bool Server::_validateChannelPass(std::string &msg, Channel *channel, int fd) {
	if (msg != channel->getKey()) {
		sendResponse("Wrong channel password\n", fd);
		return false;
	}
	return true;
}

void	Server::_user(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	// size_t pos = message.find_first_not_of(" \t\v\f");
	// if (pos != std::string::npos) {
	// 	message = message.substr(pos);
	// } else {
	// 	message = "";
	// 	sendResponse("Username cannot be empty\n", client->getFd());
	// 	return;
	// }

	// if (splitted_cmd.size() != 2) {
	// 	sendResponse("Invalid amount of arguments. Use \"USER <username>\".\n", client->getFd());
	// 	return;
	// }
	

	std::vector<Client*>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientUser(splitted_cmd[1]));
	if (it != _clients.end() && (*it)->getFd() != sender_fd) {
		sendResponse("Username is already taken\n", client->getFd());
	} else {
		// Username is not occupied or is occupied by the same client
		if (!client->getUsername().empty()){
			sendResponse("Username cannot be changed\n", client->getFd());
		}
		else {
			if (_validateName(splitted_cmd[1], client->getFd(), "Username", 0)) {
				client->setUsername(splitted_cmd[1]);
				sendResponse("Username is succesfully set up\n", client->getFd());
			}
		}
	}
}

void Server::_directMessage(std::string& message, int sender_fd) {
	std::vector<Client*>::iterator sender_it = std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<std::string> splitted_cmd = _split(message);

	if (!_checkAuth(**sender_it, sender_fd, 0))
		return;
	if (!validateUserCreds(**sender_it, sender_fd))
		return;

	//Check if receiver is present
	std::vector<Client*>::iterator receiver_it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[1]));
	if (sender_it == receiver_it) {
		sendResponse("Unable to send it back to you\n", sender_fd);
		return;
	} else if (receiver_it == _clients.end()) {
		sendResponse("No client found\n", sender_fd);
		return;
	}

	//Check if command is correct
	if (splitted_cmd.size() < 3) {
		if (splitted_cmd.size() == 1) {
			sendResponse("No nickname provided\n", sender_fd);
		} else {
			sendResponse("Message cannot be empty\n", sender_fd);
		}
		return;
	}

	std::string time = getCurrentTime();
	std::string direct_message = time + " [Private] " + (*sender_it)->getNickname() + ": ";
	for (std::vector<std::string>::iterator it = splitted_cmd.begin() + 2; it != splitted_cmd.end(); ++it) {
		direct_message += *it + " ";
	}
	direct_message += "\n";
	(*receiver_it)->receiveMessage(direct_message);
}

void	Server::_nick(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	// message = message.substr(4);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	// size_t pos = message.find_first_not_of(" \t\v\f");
	// if (pos != std::string::npos) {
	// 	message = message.substr(pos);
	// } else {
	// 	message = "";
	// 	sendResponse("Nickname cannot be empty\n", client->getFd());
	// 	return;
	// }

	if (splitted_cmd.size() != 2) {
		sendResponse("Invalid amoun of arguments. Use \"NICK <nickname>\".\n", client->getFd());
		return;
	}

	std::vector<Client*>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[1]));
	if (it != _clients.end() && (*it)->getFd() != sender_fd) {
		sendResponse("Nickname is already taken\n", client->getFd());
	} else {
		// Nickname is not occupied or is occupied by the same client
		if (splitted_cmd[1] == client->getNickname()){
			sendResponse("Nickname cannot be the same\n", client->getFd());
		}
		else {
			if (_validateName(splitted_cmd[1], client->getFd(), "Nickname", 0)) {
				client->setNickname(splitted_cmd[1]);
				sendResponse("Nickname is succesfully set up\n", client->getFd());
			}
		}
	}
}

// Usage: JOIN <channel name> <password (optional)>
void	Server::_join(std::string& msg, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(msg);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	// if (msg == "JOIN :") {
    // 	sendResponse("ERROR :No channel name given", sender_fd);
	// 	return;
	// }

	// Separate it into function
	// *
	// if (!_checkAuth(*client, sender_fd, 0))
	// 	return;
	if (!validateUserCreds(*client, sender_fd))
		return;
	// *

	std::vector<Channel*>::iterator it = std::find_if(_channels.begin(), _channels.end(), CompareChannelName(splitted_cmd[1]));

	std::string password = "";
    if (splitted_cmd.size() == 3) {
        password = splitted_cmd[2];
    } else if (splitted_cmd.size() > 3 || splitted_cmd.size() < 2) {
		sendResponse("Wrong number of parameters\n", sender_fd);
		return;
	}

	if (it == _channels.end()) {
		if (!_validateName(splitted_cmd[1], client->getFd(), "Channel name", 1))
			return;
		if (splitted_cmd.size() == 3 && !_validateName(splitted_cmd[2], client->getFd(), "Channel password", 2))
			return;
		// If client already in channel leave the channel
		if (client->getInChannel()) {
			_client_channel[client]->removeClientFromChannel(*client);
			sendResponse("You left the old channel\n", sender_fd);
		}
		Channel *new_channel = new Channel(splitted_cmd[1], *client);
		if (splitted_cmd.size() == 3)
			new_channel->setKey(splitted_cmd[2]);
		// std::cout << "Address of original channel: " << new_channel << "\n";
		sendResponse("No channels found, new channel has been created\n", sender_fd);
		_channels.push_back(new_channel);
		_client_channel[client] = new_channel;
		if (_client_channel[client]->isOperator(*client))
			sendResponse("You are the operator here\n", sender_fd);
	} else {
		if (client->getInChannel() && _client_channel[client]->getName() == (*it)->getName()) { // TODO: add == for Channel
			sendResponse("Enter different channel name\n", sender_fd);
		}
		if ((*it)->getHasKey()) {
			if (!_validateChannelPass(splitted_cmd[2], *it, sender_fd))
				return;
		}
		// Can't put this higher since user can be removed from channel and not authenticated to enter new one
		if (client->getInChannel()) {
			_client_channel[client]->removeClientFromChannel(*client);
			sendResponse("You left the old channel\n", sender_fd);
		}
		if (!(*it)->addClientToChannel(*client, sender_fd, 0))
			return;
		_client_channel[client] = *it;
		if (_client_channel[client]->isOperator(*client))
			sendResponse("You are the operator here\n", sender_fd);
	}
}

void	Server::_kick(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	if (!_checkAuth(*client, sender_fd, 0))
		return;
	if (!validateUserCreds(*client, sender_fd))
		return;
	if (splitted_cmd.size()!= 2) {
		sendResponse("Wrong number of parameters\n", sender_fd);
		return;
	}
	if (!client->getInChannel()) {
	//if (_client_channel.find(client) == _client_channel.end()) {
		sendResponse("You are not connected to any channel\n", sender_fd);
		return;
	}
	if (!_client_channel[client]->isOperator(*client)) {
		sendResponse("You are not allowed to do that\n", sender_fd);
		return;
	}

	std::vector<Client>& channel_clients = _client_channel[client]->getClients();
	std::vector<Client>::iterator it = std::find_if(channel_clients.begin(), channel_clients.end(), CompareClientNickRef(splitted_cmd[1]));
	std::vector<Client>::iterator client_it = std::find(channel_clients.begin(), channel_clients.end(), *client);
	if (it == channel_clients.end()) {
		sendResponse("No client found\n", sender_fd);
		return;
	} else if (it == client_it) {
		sendResponse("Wrong client to kick\n", sender_fd);
		return;
	} else {
		_client_channel[client]->removeClientFromChannel(*it);
		sendResponse("You have been kicked from the channel\n", it->getFd());
		std::map<Client*, Channel*>::iterator it_client_channel = std::find_if(_client_channel.begin(), _client_channel.end(), CompareClientNickMap(splitted_cmd[1]));
		if (it_client_channel != _client_channel.end()) {
			it_client_channel->first->setInChannel(false);
			_client_channel.erase(it_client_channel);
		}
		sendResponse("Successfuly kicked the client\n", sender_fd);
	}
}

void	Server::_capLs(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	// Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	sendResponse("CAP LS:", sender_fd);
}

void	Server::_topic(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	size_t pos = message.find_first_not_of(" \t\v\f");
	message = message.substr(pos);
	message = message.substr(5);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::string topic = "";

	if (!client->getInChannel()) {
		sendResponse("You are not connected to any channel\n", sender_fd);
		return;
	}
	// if (splitted_cmd.size() > 2) {
	// 	sendResponse("Wrong number of parameters\n", sender_fd);
	// 	return;
	// }
	if (splitted_cmd.size() == 1) {
		if (_client_channel[client]->getHasTopic()) {
			topic = "Channel Topic: " + _client_channel[client]->getTopic() + "\n";
			sendResponse(topic, sender_fd);
		}
		else
			sendResponse("No topic in the channel\n", sender_fd);
	} else if (_client_channel[client]->getTopicPrivelege() && !_client_channel[client]->isOperator(*client)) { //TODO: make variable so it looks into map only once (optional optimisation)
		sendResponse("You are not allowed to do that\n", sender_fd);
	} else if (message.size() > 24) {
		sendResponse("Topic is too long\n", sender_fd);
	} else {
		// size_t pos = message.find_first_not_of(" \t\v\f"); // Example:     TOPIC topic_itself
		// message = message.substr(pos);
		// message = message.substr(5);
		pos = message.find_first_not_of(" \t\v\f");
		message = message.substr(pos);
		std::cout << message << std::endl;
		_client_channel[client]->setTopic(message);
		sendResponse("Topic is successfully set up\n", sender_fd);
	}
}

/*
	MODE +i: Set the channel to invite-only.
	MODE -i: Remove the invite-only restriction from the channel.
	MODE +t: Restrict the TOPIC command to channel operators.
	MODE -t: Remove the restriction of the TOPIC command to channel operators.
	MODE +k <password>: Set the channel key (password).
	MODE -k: Remove the channel key (password).
	MODE +o <nickname>: Give channel operator privilege to a user.
	MODE -o <nickname>: Take channel operator privilege from a user.
	MODE +l <limit>: Set the user limit to the channel.
	MODE -l: Remove the user limit from the channel.
*/

void	Server::_mode(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::string response = "";
	std::string password = "";

	if (!client->getInChannel()) {
        sendResponse("You are not connected to any channel\n", sender_fd);
        return;
    } else if (!_client_channel[client]->isOperator(*client)) {
		sendResponse("You are not allowed to do that\n", sender_fd);
        return;
	}

	if (splitted_cmd.size() > 1) {
		std::string mode = splitted_cmd[1];
        char operation = mode[0];
        char mode_key = mode[1];

		if ((operation != '+' && operation != '-') || mode.size() != 2) {
			sendResponse("Invalid mode operation. Use '+' to set and '-' to remove.\n", sender_fd);
			return;
		}
        if (mode_key == 'i') {
            // Set/remove Invite-only channel
			if (splitted_cmd.size() != 2)
				sendResponse("Invalid amount of arguments for this mode. Use \"MODE +i\" or \"MODE -i\".\n", sender_fd);
			else {
				_client_channel[client]->setInviteOnly(operation == '+' ? true : false);
				response = (operation == '+') ? "Successfully set channel to invite-only.\n" : "Successfully removed invite-only restriction from channel.\n";
				sendResponse(response, sender_fd);
			}
        } else if (mode_key == 't') {
            // Set/remove the restrictions of the TOPIC command to channel operators
			if (splitted_cmd.size() != 2)
				sendResponse("Invalid amount of arguments for this mode. Use \"MODE +t\" or \"MODE -t\".\n", sender_fd);
			else {
				_client_channel[client]->setTopicPrivelege(operation == '+' ? true : false);
				response = (operation == '+') ? "Successfully set restrictions of the TOPIC command to channel operators.\n" : "Successfully removed restrictions of the TOPIC command to channel operators.\n";
				sendResponse(response, sender_fd);
			}
        } else if (mode_key == 'k') {
            // Set/remove the channel key (password)
			if (splitted_cmd.size() < 2 || splitted_cmd.size() > 3)
				sendResponse("Invalid amount of arguments for this mode. Use \"MODE +k <password>\" or \"MODE -k\".\n", sender_fd);
			else if (operation == '-' && splitted_cmd.size() != 2)
				sendResponse("Invalid arguments for this mode. Use \"MODE +k <password>\" or \"MODE -k\".\n", sender_fd);
			else if (operation == '+') {
				password = splitted_cmd[2];
				if (!_validateName(password, sender_fd, "Channel password", 2))
					return;
				_client_channel[client]->setKey(password);
				sendResponse("Successfully set the channel key.\n", sender_fd);
			} else if (operation == '-') {
				if (!_client_channel[client]->getHasKey())
					sendResponse("No password to be removed.\n", sender_fd);
				else {
					_client_channel[client]->setHasKey(false);
					sendResponse("Successfully removed the channel key.\n", sender_fd);
				}
			}
        } else if (mode_key == 'o') {
            // Give/take channel operator privilege
			//		 1. Find out if receiver is on the server
			//		 2. If receiver is not client themselves
			//		 3. If receiver does not already have it or vice versa if removing
			if (splitted_cmd.size() != 3)
				sendResponse("Invalid amount of arguments for this mode. Use \"MODE +o <nickname>\" or \"MODE -o <nickname>\".\n", sender_fd);
			else {
				Client* receiver = *std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[2]));
				if (!receiver->getInChannel() || _client_channel[receiver] != _client_channel[client])
					sendResponse("User is not present in the channel.\n", sender_fd);
				else if (receiver == client)
					sendResponse("Incorrect client.\n", sender_fd);
				else if (operation == '-') {
					if (!_client_channel[receiver]->isOperator(*receiver))
						sendResponse("Unable to remove operator privelege. Client has no operator privelege.\n", sender_fd);
					else {
						_client_channel[receiver]->removeOperator(*client, *receiver, sender_fd);
					}
				} else if (operation == '+') {
					if (_client_channel[receiver]->isOperator(*receiver))
						sendResponse("Unable to grant operator privelege. Client already has operator privelege.\n", sender_fd);
					else {
						_client_channel[receiver]->setOperator(*client, *receiver, sender_fd);
					}
				}
			}
			
        } else if (mode_key == 'l') {
            // Set/remove the client limit to channel
			if (splitted_cmd.size() < 2 || splitted_cmd.size() > 3)
				sendResponse("Invalid amount of arguments for this mode. Use \"MODE +l <limit>\" or \"MODE -l\".\n", sender_fd);
			else if (operation == '-' && splitted_cmd.size() != 2)
				sendResponse("Invalid arguments for this mode. Use \"MODE +l <limit>\" or \"MODE -l\".\n", sender_fd);
			else if (operation == '-') {
				if (!_client_channel[client]->getHasClientsLimit())
					sendResponse("Unable to remove client limit. Client limit is already removed.\n", sender_fd);
				else {
					_client_channel[client]->setClientLimit(0);
					sendResponse("Successfully removed client limit.\n", sender_fd);
				}
			} else if (operation == '+') {
				if (splitted_cmd.size() != 3) {
					sendResponse("Invalid amount of arguments for this mode. Use \"MODE +l <limit>\" or \"MODE -l\".\n", sender_fd);
					return;
				}
				int clientsLimit;
				if (!_validateLimit(splitted_cmd[2], clientsLimit, sender_fd))
					return;
				if (_client_channel[client]->getHasClientsLimit() && _client_channel[client]->getClientsLimit() == clientsLimit)
					sendResponse("Unable to set clients limit. Use different value from what channel clients limit already has.\n", sender_fd);
				else {
					_client_channel[client]->setClientLimit(clientsLimit);
					sendResponse("Clients limit is successfully set up.\n", sender_fd);
				}
			}
        } else {
			sendResponse("Invalid mode key. Use 'i', 't', 'k', 'o', or 'l'.\n", sender_fd);
        }
    } else {
        // Show the current mode
		response = 
		"Invalid input.\nUsage:\n"
		"MODE +i: Set the channel to invite-only.\n"
		"\tMODE -i: Remove the invite-only restriction from the channel.\n"
		"\tMODE +t: Restrict the TOPIC command to channel operators.\n"
		"\tMODE -t: Remove the restriction of the TOPIC command to channel operators.\n"
		"\tMODE +k <password>: Set the channel key (password).\n"
		"\tMODE -k: Remove the channel key (password).\n"
		"\tMODE +o <nickname>: Give channel operator privilege to a user.\n"
		"\tMODE -o <nickname>: Take channel operator privilege from a user.\n"
		"\tMODE +l <limit>: Set the user limit to the channel.\n"
		"\tMODE -l: Remove the user limit from the channel.\n";
		sendResponse(response, sender_fd);
    }
}

bool Server::_validateLimit(std::string message, int& clientsLimit, int fd) {
	try {
		clientsLimit = std::stoi(message);
	} catch (std::invalid_argument& e) {
		sendResponse("Invalid limit. Please enter a valid number.\n", fd);
		return false;
	} catch (std::out_of_range& e) {
		sendResponse("Limit is out of range. Please enter a smaller number.\n", fd);
		return false;
	}

	if (clientsLimit < 1 || clientsLimit > 500) {
		sendResponse("Limit is out of range. Please enter a smaller number.\n", fd);
		return false;
	} else
		return true;
}

bool	Server::_checkAuth(Client& client, int fd, int flag) { // flag 0 = client, flag 1 = different client 
	if (!client.getAuth()) {
		if (flag == 0)
			sendResponse("You are not authorized to do that. Enter the password by typing <PASS> <password>\n", fd);
		else
			sendResponse("Client is not authorized\n", fd);
		return false;
	}
	return true;
}
