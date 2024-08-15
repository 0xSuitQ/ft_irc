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

	// Client& operator=(const Client& other);
	bool operator==(const Client& other) const;

	const int	&getFd() const;
	void		appendToBuffer(const char* data, size_t len);
	bool		getCompleteMessage(std::string& message);
	bool		getAuth() const;
	void		setAuth(bool value);
	void		setNickname(std::string& str);
	std::string	getNickname() const;
	void		setUsername(std::string& str);
	std::string	getUsername() const;

private:
	int			_client_fd;
	bool		_authenticated;
	std::string _buffer;
	std::string	_nickname;
	std::string	_username;
};

#endif