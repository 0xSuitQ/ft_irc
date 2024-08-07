#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <unistd.h>

class Client {
public:
	Client();
	Client(int new_fd);
	~Client();

	int	getFd();

private:
	int	_client_fd;
};

#endif