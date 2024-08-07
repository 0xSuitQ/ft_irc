#include "Client.hpp"

Client::Client() {}

Client::Client(int new_fd) : _client_fd(new_fd) {}

int	Client::getFd() {
	return _client_fd;
}