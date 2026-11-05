#include "../Engine/Network/NetworkSystem.h"
#include "../Engine//Network/TcpSocket.h"

#include <iostream>

int main()
{
	NetworkSystem networkSystem;

	if (!networkSystem.Initialize())
	{
		std::cerr << "Winsock initialization failed." << std::endl;
		return 1;
	}

	TcpSocket listenSocket;

	if (!listenSocket.Create())
	{
		const int error = WSAGetLastError();
		std::cerr << "Socket creation failed: " << error << std::endl;
		return 1;
	}

	if (!listenSocket.Bind(50000))
	{
		const int error = WSAGetLastError();
		std::cerr << "Bind failed: " << error << '\n';
		return 1;
	}

	if (!listenSocket.Listen())
	{
		const int error = WSAGetLastError();
		std::cerr << "Listen failed: " << error << '\n';
		return 1;
	}

	std::cout << "Listening on 127.0.0.1:50000\n";

	std::cout << "Socket created successfully." << std::endl;

	std::cout << "Waiting for a client..." << std::endl;

	// accept()가 생성한 소켓을 이 객체가 받는다.
	TcpSocket clientSocket;

	if (!listenSocket.Accept(clientSocket))
	{
		const int error = WSAGetLastError();
		std::cerr << "Accept failed: " << error << '\n';
		return 1;
	}

	std::cout << "Client connected." << std::endl;
	
	char buffer[512];

	while (true)
	{
		const int received = clientSocket.Receive(buffer, sizeof(buffer));

		if (received == 0)
		{
			std::cout << "Client disconnected.\n";
			break;
		}

		if (received == SOCKET_ERROR)
		{
			const int error = WSAGetLastError();
			std::cerr << "Receive failed: " << error << '\n';
			return 1;
		}

		// 수신 데이터에는 문자열 끝의 '\0'이 없을 수 있음
		std::cout.write(buffer, received);
		std::cout.flush();

		// 받은 바이트 그대로 돌려줌
		if (!clientSocket.SendAll(buffer, received))
		{
			const int error = WSAGetLastError();
			std::cerr << "Send failed: " << error << '\n';
			return 1;
		}
	}

	return 0;
}