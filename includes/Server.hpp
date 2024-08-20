#ifndef SERVER_HPP
# define SERVER_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <unistd.h>
# include <poll.h>
# include <fcntl.h>
# include <sstream>
# include <map>
# include "Client.hpp"
# include "Channel.hpp"
# include "Utils.hpp"

class Server {
public:
	Server(char **av);
	~Server();

	void	createSocket();
	void	acceptClient();
	void	setPass(std::string pass);
	void	setPort(int port);
	void	sendPrivateMessage(Client& sender, Client& receiver, const std::string& message);

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
	std::string			_server_pass;

	std::vector<struct pollfd>		_pfds;
	std::vector<Client*>			_clients;
	std::vector<Channel*>			_channels;
	std::map<Client*, Channel*>		_client_channel;

	void	_mainLoop();
	void	_handleNewConnection();
	void	_handleData(int sender_fd);
	void	_removeClient(int fd);
	void	_parse_cmd(std::string& message, int sender_fd);
	bool	_validateName(std::string& namem, int fd, std::string target, int flag) const;
	bool	_checkAuth(Client& client, int fd, int flag);
	bool	_validateChannelPass(std::string &msg, Channel *channel, int fd);
	bool	_validateLimit(std::string message, int& clientsLimit, int fd) ;

	void	_pass(std::string& message, int sender_fd);
	void	_user(std::string& message, int sender_fd);
	void	_nick(std::string& message, int sender_fd);
	void	_join(std::string& message, int sender_fd);
	void	_invite(std::string& message, int sender_fd);
	void	_kick(std::string& message, int sender_fd);
	void	_topic(std::string& message, int sender_fd);
	void	_mode(std::string& message, int sender_fd);
	void	_directMessage(std::string& message, int sender_fd);

	std::vector<std::string>	_split(std::string& str);

};

typedef void (Server::*CommandFunc)(std::string&, int);

struct CompareClientFd {
	int fd;
	CompareClientFd(int fd) : fd(fd) {}
	bool operator()(Client* client) { return client->getFd() == fd; }
};

struct ComparePollFd {
	int fd;
	ComparePollFd(int fd) : fd(fd) {}
	bool operator()(const pollfd& pfd) { return pfd.fd == fd; }
};

struct CompareClientNick {
	std::string nickname;
	CompareClientNick(const std::string& nickname) : nickname(nickname) {}
	bool operator()(const Client* client) const {
		return client->getNickname() == nickname;
	}
};

struct CompareClientNickRef {
	std::string nickname;
	CompareClientNickRef(const std::string& nickname) : nickname(nickname) {}
	bool operator()(const Client& client) const {
		return client.getNickname() == nickname;
	}
};

struct CompareClientUser {
    std::string username;
    CompareClientUser(const std::string& username) : username(username) {}
    bool operator()(const Client* client) const {
        return client->getUsername() == username;
    }
};

struct CompareChannelName {
	std::string channel_name;
	CompareChannelName(const std::string& channel_name) : channel_name(channel_name) {}
	bool operator()(const Channel* channel) const {
		return channel->getName() == channel_name;
	}
};

struct CompareClientNickMap {
    std::string nick;
    CompareClientNickMap(const std::string& nick) : nick(nick) {}
    bool operator()(const std::pair<const Client*, Channel*>& pair) const {
        return pair.first->getNickname() == nick;
    }
};

#endif