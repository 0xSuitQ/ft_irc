#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <vector>
# include "Client.hpp"
# include "Utils.hpp"

class Channel {
public:
	Channel();
	Channel(const std::string name, Client& client);
	~Channel();

	void	setName(std::string value);
	void	setInviteOnly(bool value);
	void	setTopicRestricted(bool value);
	void	setKey(const std::string& value);
	void	setOperatorPrivilege(bool value);
	void	setUserLimit(int value);
	void	addClientToChannel(Client& client, std::string pass, int fd, bool invited);
	void	removeOperator(Client& remover, Client& target, int fd);
	void	setOperator(Client& giver, Client& receiver, int fd);
	bool	isOperator(Client& client);
	void	removeClientFromChannel(Client& client);
	void	broadcastMessage(Client& client, const std::string& message);
	
	std::string	getName() const;

private:
	std::string			_name;
	std::string			_topic;
	std::vector<Client>	_clients;
	std::vector<Client>	_operators;
	std::string 		_key;
	int 				_clients_limit;
	int					_clients_count;

	struct {
		bool	invite_only;
		bool	topic_restricted;
		bool	has_key;
		bool	operator_privilege;
		bool	has_clients_limit;
	} modes;

};

bool	validateUserCreds(Client& client, int fd);

#endif