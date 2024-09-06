#include "Server.hpp"
#include "Channel.hpp"

// TODO: Cleaning client after disconecting
// TODO: Add HELP command to list all commands
// TODO: Orthodox canonical form of the classes

Server::Server(char **av) {
	this->_server_socket = -1;
	this->_port = -1;
	try {
        setPort(std::atoi(av[1])); //parser checks
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
	sockaddr_in addr = {};
	socklen_t   size = sizeof(addr);
	struct pollfd	new_client;

	int new_fd = accept(_server_socket, (sockaddr *) &addr, &size);
	if (new_fd == -1)
		throw std::runtime_error("Error while accepting a new client!");
	else {
		// Adding new client
		fcntl(new_fd, F_SETFL, O_NONBLOCK);

		char hostname[NI_MAXHOST];
		int res = getnameinfo((struct sockaddr *) &addr, sizeof(addr), hostname, NI_MAXHOST, NULL, 0, NI_NUMERICSERV);
		if (res != 0)
			throw std::runtime_error("Error while getting a hostname on a new client!");
	
		Client *client = new Client(new_fd, hostname);

		_clients.push_back(client);
		new_client.fd = new_fd;
		new_client.events = POLLIN;
		_pfds.push_back(new_client);
		_fd_count++;
		std::cout << "New client is connected" << std::endl;
		
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
	preset_cmds["PART"] = &Server::_leave;
	preset_cmds["TOPIC"] = &Server::_topic;
	preset_cmds["MODE"] = &Server::_mode;
	preset_cmds["DM"] = &Server::_directMessage;
	preset_cmds["CAP"] = &Server::_capLs;
	preset_cmds["PING"] = &Server::_ping;
	preset_cmds["QUIT"] = &Server::_quit;
	preset_cmds["PRIVMSG"] = &Server::_privmsg;

	std::vector<std::string> cmds = _split(message);
	if (cmds.empty())
		return;
	std::map<std::string, CommandFunc>::iterator cmd_func = preset_cmds.find(cmds[0]);
    if (cmd_func != preset_cmds.end()) {
        (this->*(cmd_func->second))(message, sender_fd);
    } else {
        // Handle unknown command
		// Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
		// std::cout << "Client in channel: " << client->getInChannel() << "\n";
		// if (client->getInChannel()) {
		// 	// _client_channel[client]->broadcastMessage(*client, message);
		// 	// TODO | now it broadcasts a message to all channels where the client is present
		// 	std::map<Client*, std::vector<Channel*> >::iterator it = _client_channel.find(client);
		// 	if (it != _client_channel.end()) {
		// 		std::vector<Channel*>::iterator channel_it;
		// 		for (channel_it = it->second.begin(); channel_it != it->second.end(); ++channel_it) {
		// 			(*channel_it)->broadcastMessage(*client, message);
		// 		}
		// 	}
		// }
		// else
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


// The nickname to invite and the channel to invite him or her to; if no
// 14:55     channel is given, the active channel will be used.
// 14:55 
// 14:55 Description:
// 14:55 
// 14:55     Invites the specified nick to a channel.
// 14:55 
// 14:55 Examples:
// 14:55 
// 14:55     /INVITE mike
// 14:55     /INVITE bob #irssi



void	Server::_invite(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<Channel*> channels = _client_channel[client];
	std::string channel_name = "";
	std::string client_to_invite_name = "";

	if (splitted_cmd.size() < 3) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "INVITE"), sender_fd);
		return;
	}

	client_to_invite_name = splitted_cmd[1];
	channel_name = splitted_cmd[2];
	
	if (channel_name.at(0) == '#')
		channel_name = channel_name.substr(1);

	std::vector<Client*>::iterator client_to_invite_it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(client_to_invite_name));

	if (client_to_invite_it == _clients.end() || client->getClientId() == (*client_to_invite_it)->getClientId()) {
		client->reply(ERR_NOSUCHNICK(client->getNickname(), client_to_invite_name), sender_fd);
        return;
	}

	Client *client_to_invite = *client_to_invite_it;

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
	
	std::vector<Channel*>::iterator channel_it = find_if(channels.begin(), channels.end(), CompareChannelName(channel_name));

	if (channel_it == channels.end()) {
		client->reply(ERR_NOTONCHANNEL(client->getNickname(), channel_name), sender_fd);
		return;
	}

	Channel* channel = *channel_it;

	if (!_checkAuth(*client_to_invite, sender_fd, 1))
		return;
	if (!validateUserCreds(*client_to_invite, sender_fd))
		return;
	
	std::vector<Client*> channel_clients = channel->getClients();
	std::vector<Client*>::iterator client_on_channel_it = find(channel_clients.begin(), channel_clients.end(), client_to_invite);

	if (client_on_channel_it != channel_clients.end()) {
		client->reply(ERR_USERONCHANNEL(client->getNickname(), client_to_invite_name, channel_name), sender_fd);
		return;
	}

	client_to_invite->addInvitedChannel(channel_name);
	std::string invite_message = ":" + client->getPrefix() + " INVITE " + client_to_invite->getNickname() + " :" + channel_name + "\n";
    client_to_invite->reply(invite_message, client_to_invite->getFd());
    client->reply(invite_message, sender_fd);
	
	// if (!channel->addClientToChannel(client_to_invite, client_to_invite->getFd(), 1)) {
	// 	sendResponse("Unable to connect client to the channel.\n", sender_fd);
	// 	return;
	// }
	// _client_channel[client_to_invite] = _client_channel[client]; // TODO: client doesn't see if person to invite is connected or no.
	// sendResponse("You have been invited to the channel\n", client_to_invite->getFd());
	// sendResponse("Client has been invited to the channel\n", sender_fd);
}

void Server::_removeClient(int fd) {
	
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(fd));
	std::vector<Channel*> channels = _client_channel[client];

	for (std::vector<Channel*>::iterator it = channels.begin(); it != channels.end(); ++it) {
		(*it)->removeClientFromChannel(client, 1);
	}

	_client_channel.erase(client);
	// Remove from _clients vector
	_clients.erase(std::remove_if(_clients.begin(), _clients.end(), CompareClientFd(fd)), _clients.end());
	// Remove from _pfds vector
	_pfds.erase(std::remove_if(_pfds.begin(), _pfds.end(), ComparePollFd(fd)), _pfds.end());

	close(fd);
	_fd_count--;

	delete client;
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

	std::string forbidden = "!#@$%^&*()?/\\{}[]-<>,'\"|+;:";
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

bool Server::_validateChannelPass(std::string &msg, Channel *channel, int fd, Client* client) {
	if (msg != channel->getKey()) {
		client->reply(ERR_BADCHANNELKEY(client->getNickname(), channel->getName()), fd);
		return false;
	}
	return true;
}


// USER <username> <hostname> <servername> <realname>

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
	

	// std::vector<Client*>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientUser(splitted_cmd[1]));
	if (!client->getUsername().empty()){
		client->reply(ERR_ALREADYREGISTERED(client->getNickname()), sender_fd);
	} else if (splitted_cmd.size() < 5) {
        client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "USER"), sender_fd);
        return;
    }
	else {
		// if (_validateName(splitted_cmd[1], client->getFd(), "Username", 0)) {
			// if (_clients.size() == 1)
			// 	client->setUsername(splitted_cmd[1]);
			// else
					std::ostringstream oss;
					oss << "user_" << client->getClientId();

					std::string new_username = oss.str();
				client->setUsername(new_username);
			client->reply(RPL_WELCOME(client->getNickname()), sender_fd);
		// }
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

	if (splitted_cmd.size() < 2 || splitted_cmd[1].empty()) {
		client->reply(ERR_NONICKNAMEGIVEN(client->getNickname()), sender_fd);
		return;
	}

	std::string new_nick = splitted_cmd[1];

	std::vector<Client*>::iterator it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(new_nick));
	if (it != _clients.end() && (*it)->getFd() != sender_fd) {
		client->reply(ERR_NICKNAMEINUSE(new_nick), sender_fd);
	} else {
		// Nickname is not occupied or is occupied by the same client
		if (new_nick == client->getNickname()){
			client->reply(ERR_NICKNAMEINUSE(new_nick), sender_fd);
		}
		else {
			// if (_validateName(splitted_cmd[1], client->getFd(), "Nickname", 0)) {
			client->setNickname(new_nick);
			client->reply(RPL_WELCOME(client->getNickname()), sender_fd);
			// }
		}
	}
}

// Usage: JOIN <channel name> <password (optional)>
void	Server::_join(std::string& msg, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(msg);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	
	std::string channel_name;
	std::string password = "";
	std::vector<std::string> passwords;

	// Separate it into function
	// *
	// if (!_checkAuth(*client, sender_fd, 0))
	// 	return;
	// if (!validateUserCreds(*client, sender_fd))
	// 	return;
	// *

	if (splitted_cmd.size() == 3 && !splitted_cmd[2].empty()) {
		std::istringstream ss_passwords(splitted_cmd[2]);
		std::string temp;
		while (std::getline(ss_passwords, temp, ','))
			passwords.push_back(temp);
	}

	if (splitted_cmd.size() < 2 || splitted_cmd[1].empty()) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "JOIN"), sender_fd);
	}

	std::istringstream ss(splitted_cmd[1]);

	size_t i = 0;
	while (std::getline(ss, channel_name, ',')) {
		password = (i < passwords.size()) ? passwords[i] : "";
		
		if (channel_name[0] == '#')
			channel_name = channel_name.substr(1);

		std::cout << "Channels: \n";
		for (size_t i = 0; i < _channels.size(); i++) {
			std::cout << _channels.at(i)->getName() << std::endl;
		}
	
		std::vector<Channel*>::iterator it = std::find_if(_channels.begin(), _channels.end(), CompareChannelName(channel_name));

		if (splitted_cmd.size() > 3 || splitted_cmd.size() < 2) {
			sendResponse("Wrong number of parameters\n", sender_fd);
			return;
		}

		if (it == _channels.end()) {
			// if (!_validateName(channelName, client->getFd(), "Channel name", 1))
			// 	return;
			// if (password.size() > 0 && !_validateName(password, client->getFd(), "Channel password", 2))
			// 	return;

			Channel *new_channel = new Channel(channel_name, client);
			
			if (password.size() > 0)
				new_channel->setKey(password);

			_channels.push_back(new_channel);
			_client_channel[client].push_back(new_channel);
		} else {
			std::vector<Channel*> channels = _client_channel[client];

			std::vector<Client*> clients_in_channel = (*it)->getClients();
			std::vector<Client*>::iterator client_in_channel_it = clients_in_channel.begin();

			for (; client_in_channel_it != clients_in_channel.end(); ++client_in_channel_it) {
				// Check if the user is already in the channel			
				if ((*client_in_channel_it)->getNickname() == client->getNickname()) {
					return;
				}
			}

			if (!(*it)->getInviteOnly() && (*it)->getHasKey()) {
				if (!_validateChannelPass(password, *it, sender_fd, client))
					return;
			}

			// Checking if client trying to join the channel through invitation
			std::vector<std::string> client_invites = client->getInvitedChannels();
			std::vector<std::string>::iterator client_invite_it = find(client_invites.begin(), client_invites.end(), channel_name);

			if (client_invite_it != client_invites.end()) {
				if (!(*it)->addClientToChannel(client, sender_fd, 1))
					return;
			} else {
				if (!(*it)->addClientToChannel(client, sender_fd, 0))
					return;
			}
			
			_client_channel[client].push_back(*it);

			if ((*it)->getHasTopic())
				(*it)->broadcast(RPL_TOPIC(client->getPrefix(), (*it)->getName(), (*it)->getTopic()));
			
			std::vector<Channel*> channels1 = _client_channel[client];
			std::vector<Channel*>::iterator it_beg = channels1.begin();
			for (; it_beg != channels1.end(); ++it_beg) {
				std::cout << (*it_beg)->getName() << "\n";
			}
		}
		i++;
	}
}


/*

Example:  KICK #new anna :hello chill down

PLAN:

1. Check if at argc >= 3
2. Parse channel name arg[1]
3. Parse user to kick nick
4. If argc > 3 trim first part till :
5. Check if user to kick exists
6. Kick user to kick and display a message

*/
void	Server::_kick(std::string& message, int sender_fd) { // TODO: client is not removed, IRC client doesn't response to changes
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<Channel*> channels = _client_channel[client];
	std::string response;

	if (splitted_cmd.size() < 4) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "KICK"), sender_fd);
		return;
	}
	if (!client->getInChannel()) {
			client->reply(ERR_NOTONCHANNEL(client->getNickname(), splitted_cmd[1]), sender_fd);
		return;
	}

	std::string channel_name = splitted_cmd[1];
	if (channel_name.at(0) == '#')
		channel_name = channel_name.substr(1);
	std::string client_to_kick = splitted_cmd[2];
	if (splitted_cmd[3].size() > 0) {
		std::size_t pos = message.find(':');
		if (pos != std::string::npos)
			message = message.substr(pos + 1);
	}
	
	Channel* channel = *std::find_if(channels.begin(), channels.end(), CompareChannelName(channel_name));
	if (!channel->isOperator(client)) {
        client->reply(ERR_CHANOPRIVSNEEDED(client->getNickname(), channel_name), sender_fd);
		// TODO: remove sendChanOpPrivsNeeded(client, channel);
		return;
	}

	std::vector<Client*> clients = channel->getClients();

	std::vector<Client*>::iterator it_to_kick = std::find_if(clients.begin(), clients.end(), CompareClientNick(client_to_kick));
	if (it_to_kick == clients.end()) {
        client->reply(ERR_USERNOTINCHANNEL(client->getNickname(), client_to_kick, channel_name), sender_fd);
		return;
	} else if ((*it_to_kick)->getClientId() == client->getClientId()) {
        client->reply(ERR_NOSUCHNICK(client->getNickname(), client_to_kick), sender_fd);
		return;
	} else {
		(*it_to_kick)->reply("PART #" + channel_name + " :" + (*it_to_kick)->getNickname() + "\n", (*it_to_kick)->getFd());

		// client->reply(RPL_PART(client->getNickname(), channel_name, message), sender_fd);
		channel->broadcast(RPL_KICK(client->getPrefix(), channel->getName(), client_to_kick, message));
		channel->removeClientFromChannel(*it_to_kick, 1);
		
		// Remove channel from client's channels
		std::map<Client*, std::vector<Channel*> >::iterator it_client_channel = _client_channel.find((*it_to_kick));
		if (it_client_channel != _client_channel.end()) {
			std::vector<Channel*>& channels_to_remove = it_client_channel->second;
			std::vector<Channel*>::iterator it_channel = std::find(channels_to_remove.begin(), channels_to_remove.end(), channel);
			
			if (it_channel != channels.end()) {
				// Found the channel, remove it
				channels_to_remove.erase(it_channel);
				// If the client is no longer in any channels, remove the client from the map
				if (channels_to_remove.empty()) {
					it_client_channel->first->setInChannel(false);
					_client_channel.erase(it_client_channel);
				}
			}
		}
		// response = ":" + client->getNickname() + "!" + client->getUsername() + " KICK " + channel_name + " " + (*it_to_kick).getNickname() + message; // TODO: mb need \n and also need further debuging
		// sendResponse(response, sender_fd);
		// sendResponse(response, (*it_to_kick).getFd()); // TODO: Debug
	}
}

void	Server::_capLs(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

    if (splitted_cmd.size() < 2) {
        client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "CAP"), sender_fd);
        return;
    }

	client->reply("CAP * LS :\n", sender_fd);

    // std::string subcommand = splitted_cmd[1];
    // if (subcommand == "LS") {
    //     // List supported capabilities (none in this case)
    //     client->reply("CAP * LS :\n", sender_fd);
    // } else if (subcommand == "REQ") {
    //     // Handle capability requests (none supported)
    //     client->reply("CAP * NAK :" + message.substr(message.find(':') + 1) + "\n", sender_fd);
    // } else if (subcommand == "END") {
    //     // End capability negotiation
    //     client->reply("CAP * END\n", sender_fd);
    // } else {
    //     // Unhandled subcommand
    //     std::cerr << "Unhandled CAP subcommand: " << subcommand << std::endl;
    // }
}

/*

Example:  /TOPIC
15:51     /TOPIC #<channel_name> :The robots are taking over!
15:51     /TOPIC #<channel_name> :

received input: TOPIC #new :hello world
(if empty then remove it)

*/

void	Server::_topic(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	size_t pos = message.find_first_not_of(" \t\v\f");
	message = message.substr(pos);
	message = message.substr(5);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<Channel*> channels = _client_channel[client];
	std::string response;
	std::string topic = "";

	if (!client->getInChannel()) {
		sendResponse("You are not connected to any channel\n", sender_fd);
		return;
	}
	if (splitted_cmd.size() < 3) {
		sendResponse("Wrong number of parameters\n", sender_fd);
		return;
	}

	std::string channel_name = splitted_cmd[1];
	if (channel_name.at(0) == '#') 
		channel_name = channel_name.substr(1);
	Channel* channel = *std::find_if(channels.begin(), channels.end(), CompareChannelName(channel_name));

	if (channel->getTopicPrivelege() && !channel->isOperator(client)) {
		sendChanOpPrivsNeeded(client, channel);
	} else {
		pos = message.find(':');
		if (pos != std::string::npos)
			topic = message.substr(pos + 1);

		if (topic.size() > 0) {
			channel->setTopic(topic);
			// response = ":server 332 " + client->getNickname() + " " + channel->getName() + " " + topic + "\n";
    		// sendResponse(response, sender_fd);
			channel->broadcast(RPL_TOPIC(client->getPrefix(), channel->getName(), topic));
		} else {
			channel->setTopic("");
			channel->setHasTopic(false);
			// response = ":server 331 " + client->getNickname() + " " + channel->getName() + " :Topic removed\n";
			// sendResponse(response, sender_fd);
			channel->broadcast(RPL_NOTOPIC(client->getPrefix(), channel->getName()));
		}
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


	MODE <channel> <flags> [<args>]
*/

void	Server::_mode(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<Channel*> channels = _client_channel[client];
	std::string response;
	std::string password = "";

	if (splitted_cmd.size() > 2 && splitted_cmd.size() < 5) {
		std::string mode = splitted_cmd[2];
        char operation = mode[0];
        char mode_key = mode[1];
		std::string channel_name = splitted_cmd[1];

		if (channel_name.at(0) == '#') {
			channel_name = channel_name.substr(1);
		}

		std::vector<Channel*>::iterator channel_it = find_if(channels.begin(), channels.end(), CompareChannelName(channel_name));
		if (channel_it == channels.end()) {
            client->reply(ERR_NOSUCHCHANNEL(client->getNickname(), channel_name), sender_fd);
            return;
        }

        Channel* channel = *channel_it;

		if (!channel->isOperator(client)) {
			// TODO: remove sendChanOpPrivsNeeded(client, channel);
			// TODO: remove client from a channel  # !IMPORTANT
			client->reply(ERR_CHANOPRIVSNEEDED(client->getNickname(), channel_name), sender_fd);

			channel->broadcast(RPL_KICK(client->getPrefix(), channel->getName(), "System", message)); // TODO: redo RPL, since it is not a kick
			channel->removeClientFromChannel(client, 0);
			
			// Remove channel from client's channels
			std::map<Client*, std::vector<Channel*> >::iterator it_client_channel = _client_channel.find(client);
			if (it_client_channel != _client_channel.end()) {
				std::vector<Channel*>& channels_to_remove = it_client_channel->second;
				std::vector<Channel*>::iterator it_channel = std::find(channels_to_remove.begin(), channels_to_remove.end(), channel);
				
				if (it_channel != channels.end()) {
					// Found the channel, remove it
					channels_to_remove.erase(it_channel);
					// If the client is no longer in any channels, remove the client from the map
					if (channels_to_remove.empty()) {
						it_client_channel->first->setInChannel(false);
						_client_channel.erase(it_client_channel);
					}
				}
			}

			return;
		}

		if ((operation != '+' && operation != '-') || mode.size() != 2) {
			client->reply(ERR_UNKNOWNMODE(client->getNickname(), channel_name, operation), sender_fd);
			return;
		}
        if (mode_key == 'i') {
            // Set/remove Invite-only channel
			if (splitted_cmd.size() != 3) {
				client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd);
    			sendResponse(response, sender_fd);
			} else {
				channel->setInviteOnly(operation == '+' ? true : false);
				channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), (channel->getInviteOnly() ? "+i" : "-i"), ""));
			}
        } else if (mode_key == 't') {
            // Set/remove the restrictions of the TOPIC command to channel operators
			if (splitted_cmd.size() < 3)
				client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd);
			else {
				channel->setTopicPrivelege(operation == '+' ? true : false);
				channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), (channel->getTopicPrivelege() ? "+t" : "-t"), ""));
			}
        } else if (mode_key == 'k') {
            // Set/remove the channel key (password)
			if (splitted_cmd.size() < 3)
				client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd);
			// else if (operation == '-' && splitted_cmd.size() != 2)
			// 	sendResponse("Invalid arguments for this mode. Use \"MODE +k <password>\" or \"MODE -k\".\n", sender_fd);
			else if (operation == '+') {
				password = splitted_cmd[3];

				channel->setKey(password);
				channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), "+k", password));
			} else if (operation == '-') {
					channel->setHasKey(false);
					channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), "-k", ""));
			}
        } else if (mode_key == 'o') {
			if (splitted_cmd.size() < 4)
				client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd);
			else {
				std::vector<Client*>::iterator receiver_it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[3]));
				if (receiver_it == _clients.end()) {
					client->reply(ERR_NOSUCHNICK(client->getNickname(), splitted_cmd[3]), sender_fd);
					return;
				}

				Client* receiver = *std::find_if(_clients.begin(), _clients.end(), CompareClientNick(splitted_cmd[3]));
				std::vector<Client*> clients_in_channel = channel->getClients();
				std::vector<Client*>::iterator clients_in_channel_it = clients_in_channel.begin();
				
				for (; clients_in_channel_it != clients_in_channel.end(); ++clients_in_channel_it) {
					if ((*clients_in_channel_it)->getNickname() == receiver->getNickname())
						break;
				}
				
				if (clients_in_channel_it == clients_in_channel.end()) {
					client->reply(ERR_USERNOTINCHANNEL(client->getNickname(), receiver->getNickname(), channel_name), sender_fd);
				} else if (receiver->getFd() == client->getFd()) {
					client->reply(ERR_NOSUCHNICK(client->getNickname(), receiver->getNickname()), sender_fd);
					return ;
				}

				else if (operation == '-') {
					channel->removeOperator(client, receiver);
					channel->broadcast(RPL_MODE(client->getPrefix(), channel_name, "-o", receiver->getNickname()));
				} else if (operation == '+') {
					channel->setOperator(client, receiver, sender_fd);
					channel->broadcast(RPL_MODE(client->getPrefix(), channel_name, "+o", receiver->getNickname()));
				}
			}
			
        } else if (mode_key == 'l') {
            // Set / remove the client limit to channel
			if (splitted_cmd.size() < 3)
				client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd);
			else if (operation == '-') {
				channel->setClientLimit(0);
				channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), "-l", ""));
			} else if (operation == '+') {
				if (splitted_cmd.size() != 4) {
					client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "MODE"), sender_fd); // TODO: specify response 
					return;
				}

				int clientsLimit;
				if (!_validateLimit(splitted_cmd[3], clientsLimit)) {
					client->reply(ERR_INVALIDMODEPARAM(client->getNickname(), channel->getName(), "l", splitted_cmd[3]), sender_fd);
					return;
				}
	
				clientsLimit = atoi(splitted_cmd[3].c_str());
				channel->setClientLimit(clientsLimit);
				channel->broadcast(RPL_MODE(client->getPrefix(), channel->getName(), "+l", splitted_cmd[3]));
			}
        } else {
			// sendResponse("Invalid mode key. Use 'i', 't', 'k', 'o', or 'l'.\n", sender_fd);
			std::cout << "Unknown mode\n";
			client->reply(ERR_UNKNOWNMODE(client->getNickname(), channel_name, operation), sender_fd);
        }
    // } else if (splitted_cmd.size() > 2) {
	// 	sendResponse("No channel specified.\n", sender_fd);
	// } else {
    //     // Show the current mode
	// 	response = 
	// 	"Invalid input.\nUsage:\n"
	// 	"MODE +i: Set the channel to invite-only.\n"
	// 	"\tMODE -i: Remove the invite-only restriction from the channel.\n"
	// 	"\tMODE +t: Restrict the TOPIC command to channel operators.\n"
	// 	"\tMODE -t: Remove the restriction of the TOPIC command to channel operators.\n"
	// 	"\tMODE +k <password>: Set the channel key (password).\n"
	// 	"\tMODE -k: Remove the channel key (password).\n"
	// 	"\tMODE +o <nickname>: Give channel operator privilege to a user.\n"
	// 	"\tMODE -o <nickname>: Take channel operator privilege from a user.\n"
	// 	"\tMODE +l <limit>: Set the user limit to the channel.\n"
	// 	"\tMODE -l: Remove the user limit from the channel.\n";
	// 	sendResponse(response, sender_fd);
    // }
	}
}

void Server::_ping(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	if (splitted_cmd.size() < 2) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "PING"), sender_fd);
	} else {
		client->write(RPL_PING(client->getPrefix(), splitted_cmd[1]), sender_fd);
	}
}

void Server::_pong(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	if (splitted_cmd.size() < 2) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "PONG"), sender_fd);
	} else {
		client->write(RPL_PING(client->getPrefix(), splitted_cmd[1]), sender_fd);
	}
}

// QUIT <reason (optional)>
void Server::_quit(std::string& message, int sender_fd) {
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::vector<std::string> splitted_cmd = _split(message);
	message = message.substr(4);
	size_t pos = message.find_first_not_of(" \t\v\f");
	message = message.substr(pos);
    std::string reason = splitted_cmd.size() < 2 ? "Leaving..." : message;

    if (reason.at(0) == ':')
        reason = reason.substr(1);

    client->write(RPL_QUIT(client->getPrefix(), reason), sender_fd);
	_removeClient(sender_fd);
}

// PRIVMSG <msgtarget> :<message>
void Server::_privmsg(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));

	std::vector<Channel*> channels = _client_channel[client];

	if (splitted_cmd.size() < 3 || splitted_cmd[1].empty() || splitted_cmd[2].empty()) {
        client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "PRIVMSG"), sender_fd);
        return;
    }

	std::string target_name = splitted_cmd[1];

	size_t		pos = message.find(':');
	message = message.substr(pos + 1);
	
	if (target_name.at(0) == '#') {
		std::vector<Channel*>::iterator channel_it = find_if(channels.begin(), channels.end(), CompareChannelName(target_name.substr(1)));

		// debug
		std::vector<Channel*>::iterator it = channels.begin();
		for (; it != channels.end(); ++it) {
			std::cout << "Channel in channels for client: " << (*it)->getName() << "\n";
		}

		if (channel_it == channels.end()) {
			client->reply(ERR_NOSUCHCHANNEL(client->getNickname(), target_name), sender_fd);
			return;
		}
		else {
			Channel* channel = *channel_it;

			channel->broadcast(RPL_PRIVMSG(client->getPrefix(), target_name, message), client);
       		return;
		}
	} else {
		std::vector<Client*>::iterator target_client_it = std::find_if(_clients.begin(), _clients.end(), CompareClientNick(target_name));

		if (target_client_it == _clients.end()) {
        	client->reply(ERR_NOSUCHNICK(client->getNickname(), target_name), sender_fd);
			return;
		}

		Client* target_client = *target_client_it;
	
		// sendPrivateMessage(client, target_client, message);	
		target_client->reply(RPL_PRIVMSG(client->getPrefix(), target_name, message), target_client->getFd());
	}
}

void Server::sendPrivateMessage(Client* sender, Client* receiver, const std::string& message) {
	if (sender != receiver) {
		receiver->receiveMessage(sender->getNickname());
		receiver->receiveMessage(message);
	}
}

// syntax: PART <channel_name>
//		   PART <channel_name> :<mesage>

void	Server::_leave(std::string& message, int sender_fd) {
	std::vector<std::string> splitted_cmd = _split(message);
	Client* client = *std::find_if(_clients.begin(), _clients.end(), CompareClientFd(sender_fd));
	std::string reply_message = "";

	std::vector<Channel*> channels = _client_channel[client];

	if (splitted_cmd.size() < 2) {
		client->reply(ERR_NEEDMOREPARAMS(client->getNickname(), "PART"), sender_fd);
		return;
	} else if (splitted_cmd.size() >= 3) {
		size_t pos = message.find(':');

		if (pos != std::string::npos) {
			reply_message = message.substr(pos + 1);
		}
	}

	std::string channel_name = splitted_cmd[1];
	if (channel_name.at(0) == '#')
		channel_name = channel_name.substr(1);

	std::vector<Channel*>::iterator channel_it = find_if(channels.begin(), channels.end(), CompareChannelName(channel_name));

	if (channel_it == channels.end()) {
		client->reply(ERR_NOSUCHCHANNEL(client->getNickname(), channel_name), sender_fd);
		return;
	}

	Channel* channel = *channel_it;

	client->reply("PART #" + channel_name + " :" + client->getNickname() + "\n", client->getFd());
	channel->broadcast(RPL_LEAVE(client->getPrefix(), channel->getName(), client->getNickname()), client); // TODO: add different RPL_KICK to be RPL_LEAVE
	channel->removeClientFromChannel(client, 0);
	
	// Remove channel from client's channels
	std::map<Client*, std::vector<Channel*> >::iterator it_client_channel = _client_channel.find(client);
	if (it_client_channel != _client_channel.end()) {
		std::vector<Channel*>& channels_to_remove = it_client_channel->second;
		std::vector<Channel*>::iterator it_channel = std::find(channels_to_remove.begin(), channels_to_remove.end(), channel);
		
		if (it_channel != channels.end()) {
			// Found the channel, remove it
			channels_to_remove.erase(it_channel);
			// If the client is no longer in any channels, remove the client from the map
			if (channels_to_remove.empty()) {
				it_client_channel->first->setInChannel(false);
				_client_channel.erase(it_client_channel);
			}
		}
	}
}

bool Server::_validateLimit(std::string message, int& clientsLimit) {
	try {
		clientsLimit = std::atoi(message.c_str());
	} catch (std::invalid_argument& e) {
		return false;
	} catch (std::out_of_range& e) {
		return false;
	}

	if (clientsLimit < 1 || clientsLimit > 500) {
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
