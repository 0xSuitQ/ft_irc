#include "Channel.hpp"

Channel::Channel() {
	modes.invite_only = false;
	modes.topic_restricted = false;
	modes.has_key = false;
	modes.operator_privilege = false;
	modes.has_clients_limit = false;

	setName("Main Channel");
}

Channel::Channel(const std::string name, Client& client) {
	modes.invite_only = false;
	modes.topic_restricted = false;
	modes.has_key = false;
	modes.operator_privilege = false;
	modes.has_clients_limit = false;

	setName(name);
	_clients.push_back(client);
	_operators.push_back(client);
	_clients_count++;
	client.setInChannel(true);
}

Channel::~Channel() {}

void Channel::broadcastMessage(Client& client, const std::string& message) {
	std::string time = getCurrentTime();
	std::string broadcast_message = time + " " + client.getNickname() + ": " + message + "\n";
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (*it != client)
			it->receiveMessage(broadcast_message);
	}
}

void Channel::addClientToChannel(Client& client, std::string pass, int fd, bool invited) {
	if (!validateUserCreds(client, fd)) {
		return;
	}
	if (modes.invite_only && !invited) {
		sendResponse("Could not connect to the channel. The channel is invite-only\n", fd);
	}
	else if (modes.has_key && pass != _key) {
		sendResponse("Incorrect password to the channel\n", fd);
	}
	if (modes.has_clients_limit && _clients_count >= _clients_limit) {
		sendResponse("Could not connect to the channel. Too many clients in the channel\n", fd);
	} else {
		_clients.push_back(client);
		_clients_count++;
		client.setInChannel(true);
		
		sendResponse("Succesfully connected to the channel\n", fd);
	}
}

void Channel::removeClientFromChannel(Client& client) {
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (*it == client) {
			_clients.erase(it);
			break;
		} 
	}
}

void Channel::setName(std::string name) { _name = name; }

void Channel::setOperator(Client& giver, Client& receiver, int fd) {
	if (isOperator(giver)) {
		for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			if (*it == receiver) {
				_operators.push_back(receiver);
				sendResponse("You successfully granted operator role\n", fd);
				return;
			} else {
				sendResponse("Client is not a channel member\n", fd);
			}
		}
	} else {
		sendResponse("You do not have enough rights to do that\n", fd);
	}
}

void Channel::removeOperator(Client& remover, Client& target, int fd) {
	if (isOperator(remover)) {
		for (std::vector<Client>::iterator it = _operators.begin(); it != _operators.end(); ++it) {
			if (*it == target) {
				_operators.erase(it);
				sendResponse("You successfully removed operator role\n", fd);
				break;
			}
		}
	} else {
		sendResponse("You do not have enough rights to do that\n", fd);
	}
}

bool Channel::isOperator(Client& client) {
	return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

std::string Channel::getName() const { return _name; }

void Channel::setInviteOnly(bool value) {
    modes.invite_only = value;
}

void Channel::setTopicRestricted(bool value) {
    modes.topic_restricted = value;
}

void Channel::setKey(const std::string& value) {
    _key = value;
    modes.has_key = !value.empty();
}

void Channel::setOperatorPrivilege(bool value) {
    modes.operator_privilege = value;
}

void Channel::setUserLimit(int value) {
    _clients_limit = value;
    modes.has_clients_limit = (value > 0);
}

bool validateUserCreds(Client& client, int fd) {
	if (client.getNickname().empty() || client.getUsername().empty()) {
		sendResponse("Invalid client data\n", fd);
		return false;
	}
	return true;
}