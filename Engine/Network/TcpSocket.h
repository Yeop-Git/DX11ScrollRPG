#pragma once

#include <WinSock2.h>

class TcpSocket
{
public :
	TcpSocket() = default;
	~TcpSocket();

	TcpSocket(const TcpSocket&) = delete;
	TcpSocket& operator=(const TcpSocket&) = delete;

	bool Create();
	void Close();

	// 로컬 PC에서 사용할 서버 포트를 지정
	bool Bind(unsigned short port);
	// 연결 수락을 기다리는 요청 Queue를 준비한다
	bool Listen(int backlog = SOMAXCONN);

	bool Accept(TcpSocket& client);
	bool Connect(unsigned short port);
	bool SendAll(const char* data, int length);
	int Receive(char* buffer, int capacity);

private:
	SOCKET socket_ = INVALID_SOCKET;
};