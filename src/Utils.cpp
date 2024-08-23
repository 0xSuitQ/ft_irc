#include "Utils.hpp"

void	sendResponse(std::string msg, int fd) {
	if (send(fd, msg.c_str(), msg.length(), 0) == -1)
		std::cerr << "Error: send()\n";
}

std::string getCurrentTime() {
	std::time_t now = std::time(0);
	std::tm* localTime = std::localtime(&now);

	char buffer[80];
	std::strftime(buffer, sizeof(buffer), "%Y-%m-%d %X", localTime);
	return std::string(buffer);
}