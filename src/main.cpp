#include "Server.hpp"
#include "Client.hpp"

int main(int ac, char **av) {
	if (ac == 3)
		Server server(av);
	else {
		std::cerr << "Error\nUsage: ./ircserv <port> <password>\n";
	}
}