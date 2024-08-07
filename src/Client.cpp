#include "Client.hpp"

Client::Client() {}

Client::Client(int new_fd) : _client_fd(new_fd) {}

Client::~Client() {}

const int	&Client::getFd() {
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