#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sys/socket.h>
# include <ctime>
# include <sstream>
# include <iostream>

# define ERR_CHANNELISFULL(source,channel) "471 " + source + " " + channel + " :Cannot join channel (+l)"
# define ERR_BADCHANNELKEY(source,channel) "475 " + source + " " + channel + " :Cannot join channel (+k)"
# define ERR_ALREADYREGISTERED(source) "462 " + source + " :You may not register"
# define ERR_NEEDMOREPARAMS(source,command) "461 " + source + " " + command + " :Not enough parameters"
# define ERR_NONICKNAMEGIVEN(source) "431 " + source + " :Nickname not given"
# define ERR_NICKNAMEINUSE(source) "433 " + source + " " + source + " :Nickname is already in use"
# define ERR_NOTONCHANNEL(source,channel) "442 " + source + " " + channel + " :You're not on that channel"
# define ERR_CHANOPRIVSNEEDED(source,channel) "482 " + source + " " + channel + " :You're not channel operator"
# define ERR_USERNOTINCHANNEL(source,nickname,channel) "441 " + source + " " + nickname + " " + channel + " :They aren't on that channel"
# define ERR_NOSUCHNICK(source,nickname) "401 " + source + " " + nickname + " :No such nick"
# define ERR_CHANOPRIVSNEEDED(source,channel) "482 " + source + " " + channel + " :You're not channel operator"
# define ERR_UNKNOWNMODE(source,channel, operation) "501 " + source + " " + channel + " " + operation + " :Is unknown mode char.\n"
# define ERR_NOSUCHCHANNEL(source,channel) "403 " + source + " " + channel + " :No such channel"
# define ERR_CANNOTSENDTOCHAN(source,channel) "404 " + source + " " + channel + " :Cannot send to channel"
# define ERR_INVALIDMODEPARAM(source, channel, mode, param) "696 " + source + " " + channel + " " + mode + " " + param + " :Invalid mode parameter."
# define RPL_TOPIC(source, channel, topic) "332 "  + source + " " + channel + " :" + topic + "\n"
# define RPL_NOTOPIC(source, channel) "331 " + source + " " + channel + " :Topic removed\n"
# define RPL_PRIVMSG(source,target,message) ":" + source + " PRIVMSG " + target + " :" + message
# define RPL_QUIT(source,message) ":" + source + " QUIT :Quit: " + message
# define RPL_MODE(source,channel,modes,args) ":" + source + " MODE #" + channel + " " + modes + " " + args
# define RPL_PING(source,token) ":" + source + " PONG :" + token
# define RPL_KICK(source,channel,target,reason) ":" + source + " KICK #" + channel + " " + target + " :" + reason
# define RPL_NAMREPLY(source,channel,users) "353 " + source + " = #" + channel + " :" + users
# define RPL_ENDOFNAMES(source,channel) "366 " + source + " " + channel + " :End of /NAMES list."
# define RPL_JOIN(source,channel) ":" + source + " JOIN :#" + channel
# define RPL_WELCOME(source) "001 " + source + " :Welcome " + source + " to the ft_irc network"

void		sendResponse(const std::string msg, int fd);
std::string	getCurrentTime();



#endif