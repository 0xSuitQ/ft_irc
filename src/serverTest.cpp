#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <iostream>
#include <unistd.h>

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

int main() {
	int serverSocket;

	serverSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (serverSocket == INVALID_SOCKET) {
		std::cout << "Error at socket()" << std::endl;
	} else {
		std::cout << "Socket is ok" << std::endl;
	}

	sockaddr_in service;
	service.sin_family = AF_INET;
	service.sin_addr.s_addr = INADDR_ANY;
	service.sin_port = htons(5673);
	if (bind(serverSocket, (sockaddr *)&service, sizeof(service)) == -1) {
		perror("bind() failed");
		close(serverSocket);
	} else {
		std::cout << "bind is good" << std::endl;
	}

	if (listen(serverSocket, 1) == -1) {
		std::cout << "listen error" << std::endl;
	} else {
		std::cout << "listening for connections" << std::endl;
	}

	int acceptSocket = accept(serverSocket, NULL, NULL);
	if (acceptSocket == -1) {
		std::cout << "accept failed" << std::endl;
	} else {
		std::cout << "Accepted connection" << std::endl;
	}

	char buffer[200];

	int byteCount = recv(acceptSocket, buffer, 200, 0);
	if (byteCount > 0) {
		std::cout << "Message received: " << buffer << std::endl;
	} else {
		std::cout << "Failed to receive the message" << std::endl;
	}

	char confirmation[200] = "Message received";
	byteCount = send(acceptSocket, confirmation, 200, 0);
	if (byteCount > 0) {
		std::cout << "Auto message sent: " << confirmation << std::endl;
	} else {
		std::cout << "Failed to send auto message" << std::endl;
	}
}