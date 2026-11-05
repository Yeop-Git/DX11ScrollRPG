#include "TcpSocket.h"

TcpSocket::~TcpSocket()
{
	Close();
}

bool TcpSocket::Create()
{
	if (socket_ != INVALID_SOCKET) return false;

	socket_ = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	return socket_ != INVALID_SOCKET;
}

void TcpSocket::Close()
{
	if (socket_ == INVALID_SOCKET) return;
	::closesocket(socket_);
	socket_ = INVALID_SOCKET;
}

bool TcpSocket::Bind(unsigned short port)
{
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);

	//현재는 로컬서버만
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	return ::bind(socket_, reinterpret_cast<sockaddr*>(&address), sizeof(address)) == 0;
}

bool TcpSocket::Listen(int backlog)
{
	return ::listen(socket_, backlog) != SOCKET_ERROR;
}

bool TcpSocket::Accept(TcpSocket& client)
{
	// 기존 소켓을 덮어써서 소유권을 잃는 것을 방지
	if (client.socket_ != INVALID_SOCKET)
	{
		WSASetLastError(WSAEINVAL);
		return false;
	}

	// 현재는 상대 주소 정보가 필요하지 않아 nullptr로 전달
	const SOCKET acceptedSocket = ::accept(socket_, nullptr, nullptr);

	if (acceptedSocket == INVALID_SOCKET) return false;

	// 이후 소켓 정리는 client의 Close 또는 소멸자가 담당
	client.socket_ = acceptedSocket;
	return true;
}

bool TcpSocket::Connect(unsigned short port)
{
	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = htons(port);
	address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	return ::connect(
		socket_,
		reinterpret_cast<const sockaddr*>(&address),
		sizeof(address)) != SOCKET_ERROR;
}

bool TcpSocket::SendAll(const char* data, int length)
{
	int totalSent = 0;

	// 남은 바이트 계속 전송
	while (totalSent < length)
	{
		const int sent = ::send(
			socket_, data + totalSent, length - totalSent, 0);

		if (sent == SOCKET_ERROR) return false;

		if (sent == 0)
		{
			// 진행 없이 반복되는 상황 방지
			WSASetLastError(WSAECONNABORTED);
			return false;
		}

		totalSent += sent;
	}

	return true;
}

int TcpSocket::Receive(char* buffer, int capacity)
{
	// 받은 바이트 수, 정상 종료(0), 오류(SOCKET_ERROR) 전달
	return ::recv(socket_, buffer, capacity, 0);
}