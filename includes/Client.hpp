#ifndef CLIENT_HPP
# define CLIENT_HPP

# include <sys/types.h>
# include <sys/socket.h>
# include <netinet/in.h>
# include <iostream>
# include <cstdio>
# include <unistd.h>
# include <sstream>
# include <vector>

class Client {
public:
	Client();
	Client(int new_fd, const std::string &hostname);
	~Client();

	// Client& operator=(const Client& other);
	bool operator==(const Client& other) const;
	bool operator!=(const Client& other) const;
	bool operator<(const Client& other) const;

	const int	&getFd() const;
	void		appendToBuffer(const char* data, size_t len);
	bool		getCompleteMessage(std::string& message);
	bool		getAuth() const;
	void		setAuth(bool value);
	void		setNickname(std::string& str);
	std::string	getNickname() const;
	void		setUsername(std::string str);
	std::string	getUsername() const;
	void		setInChannel(bool value);
	bool		getInChannel() const;
	int			getClientId() const;
	void		receiveMessage(const std::string& message);
	std::string	getPrefix() const;
	void		write(const std::string& message, int fd) const;
	void		reply(const std::string& reply, int fd);
	
	std::vector<std::string>&	getInvitedChannels();
	void						addInvitedChannel(std::string& channel_name);

private:
	static int	_next_id;
	int			_client_fd;
	int			_client_id;
	bool		_authenticated;
	bool		_in_channel;
	std::string _buffer;
	std::string	_nickname;
	std::string	_username;
	std::string _hostname;

	std::vector<std::string>	_invited_channels;
	
};

#endif