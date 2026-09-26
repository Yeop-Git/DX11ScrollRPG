#pragma once

#include <condition_variable>
#include <cstddef>
#include <functional>
#include <mutex>
#include <queue>
#include <thread>
#include <vector>

// Worker와 대기 작업을 소유한다. 작업이 캡처한 포인터의 대상은 소유하지 않는다.
// 생성한 스레드에서 Start / Stop / 파괴를 수행하고, Submit은 여러 스레드에서 호출할 수 있다.
class JobSystem
{
public:
	JobSystem() = default;
	~JobSystem();

	JobSystem(const JobSystem&) = delete;
	JobSystem& operator=(const JobSystem&) = delete;

	// 0개 Worker, 중복 시작, Worker 생성 실패는 false. 종료 후 재시작할 수 있다.
	bool Start(std::size_t workerCount);
	// true는 접수 성공이며 완료를 뜻하지 않는다. 빈 작업과 종료 이후 제출은 거부한다.
	bool Submit(std::function<void()> job);
	// 접수를 닫고 이미 받은 작업을 모두 처리한 뒤 Worker를 join한다. 반복 호출 가능.
	void Stop();
	// 최근 Start 이후 예외를 던진 작업 수. 실패한 작업은 자동 재시도하지 않는다.
	std::size_t GetFailedJobCount() const;

private:
	void CheckOwnerThread() const;
	void WorkerLoop();

private:
	const std::thread::id ownerThreadId_ = std::this_thread::get_id();
	std::vector<std::thread> workers_;
	std::queue<std::function<void()>> jobs_;
	// Queue, 접수 상태, 실패 횟수는 모두 같은 mutex로 보호한다.
	mutable std::mutex mutex_;
	std::condition_variable cv_;
	bool acceptingJobs_ = false;
	std::size_t failedJobCount_ = 0;
};
