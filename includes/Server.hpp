#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <unistd.h>
# include <poll.h>

class Server {
public:
	Server();
	~Server();

	void	createSocket();
	void	acceptClient();

	class SocketCreationException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: cannot create a socket";
		}
	};
	class BindException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: cannot bind a socket";
		}
	};
	class ListenException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: cannot start listening";
		}
	};
	class FailedConnectionException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: cannot conncet a client";
		}
	};

private:
	int					_serverSocket;
	int					_port;
	std::vector<int>	_clientSockets;

};

#endif