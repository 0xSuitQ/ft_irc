#ifndef CHANNEL_HPP
# define CHANNEL_HPP

# include <vector>
# include <algorithm>
# include "Client.hpp"
# include "Utils.hpp"

class Channel {
public:
	Channel();
	Channel(const std::string name, Client* client);
	~Channel();

	void	setName(std::string value);
	void	setInviteOnly(bool value);
	void	setTopicRestricted(bool value);
	void	setKey(const std::string& value);
	void	setHasKey(bool value);
	void	setTopic(std::string value);
	void	setTopicPrivelege(bool value);
	void	setClientLimit(int value);
	bool	addClientToChannel(Client* client, int fd, bool invited);
	void	removeOperator(Client* remover, Client* target);
	void	setOperator(Client* giver, Client* receiver, int fd);
	void	setHasTopic(bool value);
	bool	isOperator(Client* client);
	void	removeClientFromChannel(Client* client);
	void	broadcastMessage(Client* client, const std::string& message);
	void	broadcast(const std::string& message);
	void	broadcast(const std::string& message, Client* exclude);
	
	std::string				getName() const;
	std::string				getKey() const;
	std::string				getTopic() const;
	bool					getHasKey() const;
	bool					getHasTopic() const;
	bool					getInviteOnly() const;
	bool					getTopicPrivelege() const;
	bool					getHasClientsLimit() const;
	int						getClientsLimit();
	std::vector<Client*>	&getClients();

	void debugPrint() const;

private:
	std::string				_name;
	std::string				_topic;
	std::vector<Client*>	_clients;
	std::vector<Client*>	_operators;
	std::string 			_key;
	int 					_clients_limit;
	int						_clients_count;

	struct {
		bool	invite_only;
		bool	has_key;
		bool	has_topic;
		bool	topic_privelege;
		bool	has_clients_limit;
	} modes;

};

bool	validateUserCreds(Client& client, int fd);
void	sendCannotSendToChannel(Client* client, Channel* channel);
void	sendChanOpPrivsNeeded(Client* client, Channel* channel);

#endif





/*
Received complete message: KICK #new nandroso_ :
Prefix: nandroso_!user_1@localhost
Prefix: nandroso_!user_1@localhost
Send to client: :nandroso_!user_1@localhost :nandroso_!user_1@localhost PART #new :nandroso_

Prefix: nandroso!user_0@localhost
Send to client: :nandroso!user_0@localhost KICK #new nandroso_ :
Send to client: :nandroso!user_0@localhost KICK #new nandroso_ :



Wrong:

Received complete message: KICK #new nandroso :
Prefix: nandroso!user_0@localhost
Prefix: nandroso!user_0@localhost
Send to client: :nandroso!user_0@localhost :nandroso!user_0@localhost PART #new :nandroso

Prefix: nandroso_!user_1@localhost
Send to client: :nandroso_!user_1@localhost KICK #new nandroso :
Send to client: :nandroso_!user_1@localhost KICK #new nandroso :
*/