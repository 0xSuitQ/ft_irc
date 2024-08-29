#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sys/socket.h>
# include <ctime>
# include <sstream>
# include <iostream>

# define ERR_CHANNELISFULL(source,channel) "471 " + source + " " + channel + " :Cannot join channel (+l)"
# define ERR_BADCHANNELKEY(source,channel) "475 " + source + " " + channel + " :Cannot join channel (+k)"
# define RPL_NAMREPLY(source,channel,users) "353 " + source + " = " + channel + " :" + users
# define RPL_ENDOFNAMES(source,channel) "366 " + source + " " + channel + " :End of /NAMES list."
# define RPL_JOIN(source,channel) ":" + source + " JOIN :" + channel
# define ERR_ALREADYREGISTERED(source) "462 " + source + " :You may not register"
# define ERR_NEEDMOREPARAMS(source,command) "461 " + source + " " + command + " :Not enough parameters"
# define ERR_NONICKNAMEGIVEN(source) "431 " + source + " :Nickname not given"
# define ERR_NICKNAMEINUSE(source) "433 " + source + " " + source + " :Nickname is already in use"
# define ERR_NOTONCHANNEL(source,channel) "442 " + source + " " + channel + " :You're not on that channel"
# define ERR_CHANOPRIVSNEEDED(source,channel) "482 " + source + " " + channel + " :You're not channel operator"
# define ERR_USERNOTINCHANNEL(source,nickname,channel) "441 " + source + " " + nickname + " " + channel + " :They aren't on that channel"
# define ERR_NOSUCHNICK(source,nickname) "401 " + source + " " + nickname + " :No such nick"
# define RPL_KICK(source,channel,target,reason) ":" + source + " KICK " + channel + " " + target + " :" + reason
# define ERR_CHANOPRIVSNEEDED(source,channel) "482 " + source + " " + channel + " :You're not channel operator"
# define ERR_UNKNOWNMODE(source,channel, operation) "501 " + source + " " + channel + " " + operation + " :Is unknown mode char.\n"
# define RPL_MODE(source,channel,modes,args) ":" + source + " MODE " + channel + " " + modes + " " + args
# define RPL_PING(source,token) ":" + source + " PONG :" + token
# define RPL_QUIT(source,message) ":" + source + " QUIT :Quit: " + message
# define ERR_NOSUCHCHANNEL(source,channel) "403 " + source + " " + channel + " :No such channel"
# define RPL_PRIVMSG(source,target,message) ":" + source + " PRIVMSG " + target + " :" + message
# define ERR_CANNOTSENDTOCHAN(source,channel) "404 " + source + " " + channel + " :Cannot send to channel"

void		sendResponse(const std::string msg, int fd);
std::string	getCurrentTime();



#endif