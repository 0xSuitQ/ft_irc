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

	const int	&getFd();
	void		appendToBuffer(const char* data, size_t len);
	bool		getCompleteMessage(std::string& message);

private:
	int			_client_fd;
	std::string _buffer;
};

#endif