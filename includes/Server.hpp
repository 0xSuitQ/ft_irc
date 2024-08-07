#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <unistd.h>
# include <poll.h>
# include <fcntl.h>
# include "Client.hpp"

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
			return "Error: cannot connect a client";
		}
	};
	class PollCountException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: poll()";
		}
	};
	class AcceptException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: failed to accept a client";
		}
	};
	class RecvException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: failed to handle a recv function";
		}
	};
	class SendException : public std::exception {
		public:
		virtual const char *what() const throw() {
			return "Error: failed to send data to a client";
		}
	};

private:
	int					_server_socket;
	int					_port;
	int					_fd_count;

	std::vector<struct pollfd>		_pfds;
	std::vector<Client>				_clients;

	void	_mainLoop();
	void	_handleNewConnection();
	void	_handleData(int sender_fd);
	void	_removeClient(int fd);

};

struct CompareClientFd {
	int fd;
	CompareClientFd(int fd) : fd(fd) {}
	bool operator()(Client& client) { return client.getFd() == fd; }
};

struct ComparePollFd {
	int fd;
	ComparePollFd(int fd) : fd(fd) {}
	bool operator()(const pollfd& pfd) { return pfd.fd == fd; }
};

#endif