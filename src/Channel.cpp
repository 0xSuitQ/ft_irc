#include "Channel.hpp"

Channel::Channel() {
	modes.invite_only = false;
	modes.has_topic = false;
	modes.has_key = false;
	modes.operator_privilege = false;
	modes.has_clients_limit = false;
	modes.topic_privelege = false;

	setName("Main Channel");
}

Channel::Channel(const std::string name, Client& client) {
	modes.invite_only = false;
	modes.has_topic = false;
	modes.topic_privelege = false;
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
	 debugPrint();
	for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
		if (*it != client)
			it->receiveMessage(broadcast_message);
	}
}

void Channel::debugPrint() const {
    std::cout << "Channel at " << this << ":\n";
    std::cout << "Clients vector address: " << &_clients << "\n";
    std::cout << "Clients vector size: " << _clients.size() << "\n";
    std::cout << "Clients vector capacity: " << _clients.capacity() << "\n";
}

bool Channel::addClientToChannel(Client& client, int fd, bool invited) {
	if (!validateUserCreds(client, fd)) {
		return false;
	}
	if (modes.invite_only && !invited) {
		sendResponse("Could not connect to the channel. The channel is invite-only\n", fd);
		return false;
	}
	// else if (modes.has_key && pass != _key) { // Mb remove it since check it from outer scope
	// 	sendResponse("Wrong password to the channel\n", fd);
	// 	return false;
	// }
	// if (modes.has_clients_limit && _clients_count >= _clients_limit) {
	if (this->getHasClientsLimit() && _clients_count >= getClientsLimit()) {
		sendResponse("Could not connect to the channel. Too many clients in the channel\n", fd);
		return false;
	} else {
		_clients.push_back(client);
		_clients_count++;
		client.setInChannel(true);
		sendResponse("Succesfully connected to the channel\n", fd);
		return true;
	}
}

void Channel::removeClientFromChannel(Client& client) {
	// if (this->isOperator(client)) {
	// 	_operators.erase(std::remove(_operators.begin(), _operators.end(), client), _operators.end());
	// }
	_clients.erase(std::remove(_clients.begin(), _clients.end(), client), _clients.end());
	_clients_count--;
	client.setInChannel(false);
}

void Channel::setName(std::string name) { _name = name; }

void Channel::setOperator(Client& giver, Client& receiver, int fd) {
	if (isOperator(giver)) {
		for (std::vector<Client>::iterator it = _clients.begin(); it != _clients.end(); ++it) {
			if (*it == receiver) {
				_operators.push_back(receiver);
				sendResponse("You successfully granted operator role.\n", fd);
				sendResponse("You received operator role on this channel.\n", receiver.getFd());
				return;
			} else {
				sendResponse("Client is not a channel member\n", fd);
			}
		}
	} else {
		sendResponse("You are not allowed to do that\n", fd);
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
		sendResponse("You are not allowed to do that\n", fd);
	}
}

std::vector<Client>	&Channel::getClients() {
	return _clients;
}

bool Channel::isOperator(Client& client) {
	return std::find(_operators.begin(), _operators.end(), client) != _operators.end();
}

std::string Channel::getName() const { return _name; }

void Channel::setInviteOnly(bool value) {
    modes.invite_only = value;
}

void Channel::setKey(const std::string& value) {
    _key = value;
    modes.has_key = !value.empty();
}

void Channel::setHasKey(bool value) {
	modes.has_key = value;
}

std::string Channel::getKey() const{
	return _key;
}

bool Channel::getHasKey() const{
	return modes.has_key;
}

bool Channel::getHasTopic() const {
	return modes.has_topic;
}

bool Channel::getInviteOnly() const {
	return modes.invite_only;
}

std::string Channel::getTopic() const {
	return _topic;
}

void Channel::setTopic(std::string &value) {
	_topic = value;
	modes.has_topic = true;
}

void Channel::setTopicPrivelege(bool value) {
	modes.topic_privelege = value;
}

bool Channel::getTopicPrivelege() const {
	return modes.topic_privelege;
}

void Channel::setOperatorPrivilege(bool value) {
    modes.operator_privilege = value;
}

void Channel::setClientLimit(int value) {
	_clients_limit = value;
	modes.has_clients_limit = (value > 0);
}

int Channel::getClientsLimit() {
	return _clients_limit;
}

bool Channel::getHasClientsLimit() const{
	return modes.has_clients_limit;
}

bool validateUserCreds(Client& client, int fd) {
	if (client.getNickname().empty() || client.getUsername().empty()) {
		sendResponse("Invalid client data\n", fd);
		return false;
	}
	return true;
}