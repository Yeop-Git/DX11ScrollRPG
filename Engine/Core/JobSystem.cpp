#include "JobSystem.h"

#include <stdexcept>
#include <utility>

JobSystem::~JobSystem()
{
	// 캡처한 참조의 대상은 이 대기가 끝날 때까지 호출자가 유지해야 한다.
	Stop();
}

bool JobSystem::Start(std::size_t workerCount)
{
	CheckOwnerThread();
	if (workerCount == 0 || !workers_.empty()) return false;

	try
	{
		// 모든 Worker 생성이 성공한 뒤에만 접수를 연다.
		// Worker는 이 잠금이 풀릴 때까지 Queue에 접근하지 못한다.
		std::lock_guard<std::mutex> lock(mutex_);
		failedJobCount_ = 0;
		workers_.reserve(workerCount);
		for (std::size_t i = 0; i < workerCount; ++i)
		{
			workers_.emplace_back(&JobSystem::WorkerLoop, this);
		}
		acceptingJobs_ = true;
	}
	catch (...)
	{
		// 부분 생성 실패도 잠금을 놓은 뒤 회수해 join 대기 중 교착을 막는다.
		Stop();
		return false;
	}
	return true;
}

bool JobSystem::Submit(std::function<void()> job)
{
	if (!job) return false;

	{
		std::lock_guard<std::mutex> lock(mutex_);
		if (!acceptingJobs_) return false;

		// 상태 확인과 등록을 한 잠금 안에서 수행해 Stop과의 접수 경계를 정한다.
		jobs_.push(std::move(job));
	}
	cv_.notify_one();
	return true;
}

void JobSystem::Stop()
{
	CheckOwnerThread();
	{
		std::lock_guard<std::mutex> lock(mutex_);
		acceptingJobs_ = false;
	}
	cv_.notify_all();

	// Worker가 남은 작업을 꺼내고 종료할 수 있도록 mutex 없이 기다린다.
	for (std::thread& worker : workers_)
	{
		if (worker.joinable()) worker.join();
	}
	workers_.clear();
}

std::size_t JobSystem::GetFailedJobCount() const
{
	std::lock_guard<std::mutex> lock(mutex_);
	return failedJobCount_;
}

void JobSystem::CheckOwnerThread() const
{
	// Worker의 자기 자신 join과 여러 스레드의 동시 Start / Stop을 금지한다.
	if (std::this_thread::get_id() != ownerThreadId_)
	{
		throw std::logic_error("JobSystem lifecycle must run on its owner thread.");
	}
}

void JobSystem::WorkerLoop()
{
	while (true)
	{
		std::function<void()> job;
		{
			std::unique_lock<std::mutex> lock(mutex_);
			// wait는 대기 중 잠금을 놓고, 깨어나면 다시 잠근 뒤 조건을 확인한다.
			cv_.wait(lock, [this]()
			{
				return !jobs_.empty() || !acceptingJobs_;
			});

			if (!acceptingJobs_ && jobs_.empty()) return;

			job = std::move(jobs_.front());
			jobs_.pop();
		}

		// 긴 작업과 캡처 데이터 해제는 Queue 잠금 밖에서 실행한다.
		try
		{
			job();
		}
		catch (...)
		{
			std::lock_guard<std::mutex> lock(mutex_);
			++failedJobCount_;
		}
	}
}
