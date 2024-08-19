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
	void	setTopic(std::string &value);
	void	setOperatorPrivilege(bool value);
	void	setUserLimit(int value);
	bool	addClientToChannel(Client& client, int fd, bool invited);
	void	removeOperator(Client& remover, Client& target, int fd);
	void	setOperator(Client& giver, Client& receiver, int fd);
	bool	isOperator(Client& client);
	void	removeClientFromChannel(Client& client);
	void	broadcastMessage(Client& client, const std::string& message);
	
	std::string			getName() const;
	std::string			getKey() const;
	std::string			getTopic() const;
	bool				getHasKey() const;
	bool				getHasTopic() const;
	bool				getInviteOnly() const;
	std::vector<Client>	&getClients();

	void debugPrint() const ;

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
		bool	has_key;
		bool	has_topic;
		bool	operator_privilege;
		bool	has_clients_limit;
	} modes;

};

bool	validateUserCreds(Client& client, int fd);

#endif