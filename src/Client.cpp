#include "Client.hpp"

Client::Client() {}

Client::Client(int new_fd) : _client_fd(new_fd), _authenticated(false) {}

Client::~Client() {}

// Client& operator=(const Client& other) {
// 	if (this != &other) {

// 	}
// 	return *this;
// }

bool Client::operator==(const Client& other) const {
	return this->getNickname() == other.getNickname() && this->getUsername() == other.getUsername();
}

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

bool Client::getAuth() const { return _authenticated; }

void Client::setAuth(bool value) { _authenticated = value; }

void Client::setNickname(std::string& str) { _nickname = str; }

std::string Client::getNickname() const { return _nickname; }

void		Client::setUsername(std::string& str) { _username = str; }

std::string	Client::getUsername() const { return _username; };
