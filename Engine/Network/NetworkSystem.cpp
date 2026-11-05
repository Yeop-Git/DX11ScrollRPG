#include <WinSock2.h>
#include "NetworkSystem.h"

#pragma comment(lib, "ws2_32.lib")

bool NetworkSystem::Initialize()
{
	if (initialized_) return true;

	WSADATA wsaData{};
	const int result = WSAStartup(MAKEWORD(2, 2), &wsaData);

	if (result != 0) return false;

	initialized_ = true;
	return true;
}

void NetworkSystem::Shutdown()
{
	if (!initialized_) return;
	WSACleanup();
	initialized_ = false;
}

NetworkSystem::~NetworkSystem()
{
	Shutdown();
}