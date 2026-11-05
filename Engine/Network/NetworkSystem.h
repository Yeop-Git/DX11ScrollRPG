#pragma once

class NetworkSystem
{
public:
	NetworkSystem() = default;
	~NetworkSystem();

	NetworkSystem(const NetworkSystem&) = delete;
	NetworkSystem& operator=(const NetworkSystem&) = delete;

	bool Initialize();
	void Shutdown();

private:
	bool initialized_ = false;
};