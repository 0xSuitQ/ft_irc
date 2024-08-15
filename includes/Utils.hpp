#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sys/socket.h> 

void	sendResponse(const std::string msg, int fd);

#endif