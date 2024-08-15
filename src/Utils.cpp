#include "Utils.hpp"

void	sendResponse(std::string msg, int fd) {
	if (send(fd, msg.c_str(), msg.length(), 0) == -1)
		perror("Error: send()");
}