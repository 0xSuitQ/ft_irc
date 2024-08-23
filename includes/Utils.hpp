#ifndef UTILS_HPP
# define UTILS_HPP

# include <string>
# include <sys/socket.h>
#include <ctime>
#include <sstream>
#include <iostream>

void		sendResponse(const std::string msg, int fd);
std::string	getCurrentTime();


#endif