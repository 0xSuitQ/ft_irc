#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

int main() {
	int clientSocket = -1;
	clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (clientSocket == -1) {
		std::cout << "Eror at socket" << std::endl;
	}
	else {
		std::cout << "socket is ok" << std::endl;
	}

	sockaddr_in clientService;
	clientService.sin_family = AF_INET;
	clientService.sin_port = htons(5673);
	clientService.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (connect(clientSocket, (sockaddr*)&clientService, sizeof(clientService)) == -1) {
		std::cout << "Client connection error" << std::endl;
	} else {
		std::cout << "Client connection OK!" << std::endl;
	}

	char buffer[200];

	std::cout << "Enter a message: ";
	std::cin.getline(buffer, 200);
	int byteCount = send(clientSocket, buffer, 200, 0);
	if (byteCount > 0) {
		std::cout << "Message sent: " << buffer << std::endl;
	} else {
		std::cout << "Failed to send the message" << std::endl;
	}

	byteCount = recv(clientSocket, buffer, 200, 0);
	if (byteCount > 0) {
		std::cout << "Message received: " << buffer << std::endl;
	} else {
		std::cout << "Failed to receive the message" << std::endl;
	}
}