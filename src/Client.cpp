#include "Client.hpp"

Client::Client() {}

Client::Client(int new_fd, const std::string &hostname) : _client_fd(new_fd), _client_id(_next_id++), _authenticated(false), _in_channel(false), _hostname(hostname) {
	std::ostringstream oss;
	oss << "user_" << _client_id;

	_nickname = oss.str();
}

Client::~Client() {}

// Client& operator=(const Client& other) {
// 	if (this != &other) {

// 	}
// 	return *this;
// }

bool Client::operator==(const Client& other) const {
	return this->getNickname() == other.getNickname() && this->getUsername() == other.getUsername();
}
bool Client::operator!=(const Client& other) const {
	return !(*this == other);
}

bool Client::operator<(const Client& other) const {
	return this->getNickname() < other.getNickname() && this->getUsername() < other.getUsername();
}

int Client::_next_id = 0;

const int	&Client::getFd() const {
	return _client_fd;
}

void Client::appendToBuffer(const char* data, size_t len) {
	_buffer.append(data, len);
}

bool Client::getCompleteMessage(std::string& message) {
	size_t pos = _buffer.find('\n');
	if (pos != std::string::npos) {
		message = _buffer.substr(0, pos);
		_buffer.erase(0, pos + 1);
		return true;
	}
	return false;
}

void Client::receiveMessage(const std::string& message) {
    if (send(getFd(), message.c_str(), message.size(), 0) == -1)
		std::cerr << "Error: send()\n";
		
}

std::string Client::getPrefix() const {
    std::string username = _username.empty() ? "" : "!" + _username;
    std::string hostname = _hostname.empty() ? "" : "@" + _hostname;

	std::cout << "Prefix: " << _nickname + username + hostname << std::endl;

    return _nickname + username + hostname;
}

void Client::write(const std::string& message, int fd) const {
    std::string buffer = message + "\n";
    if (send(fd, buffer.c_str(), buffer.length(), 0) < 0) {
        throw std::runtime_error("Error while sending a message to a client!"); }
	
	std::cout << "Send to client: " << buffer;
}

void Client::reply(const std::string& reply, int fd) {
    this->write(":" + getPrefix() + " " + reply, fd);
}

std::vector<std::string>&	Client::getInvitedChannels() {
	return _invited_channels;
}

void Client::addInvitedChannel(std::string& channel_name) {
	_invited_channels.push_back(channel_name);
}

bool Client::getAuth() const { return _authenticated; }

void Client::setAuth(bool value) { _authenticated = value; }

void Client::setNickname(std::string& str) { _nickname = str; }

std::string Client::getNickname() const { return _nickname; }

int Client::getClientId() const { return _client_id; }

void Client::setUsername(std::string str) { _username = str; }

std::string	Client::getUsername() const { return _username; };

void Client::setInChannel(bool value) { _in_channel = value; }

bool Client::getInChannel() const { return _in_channel; }
