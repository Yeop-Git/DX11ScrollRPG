#include "Profiler.h"

void Profiler::BeginFrame()
{
	// 이전 프레임의 누적값을 초기화하고 전체 프레임의 시작 시각을 저장한다.
	currentFrame_ = {};
	frameStart_ = Clock::now();
}

void Profiler::EndFrame()
{
	// 메시지 처리부터 Present 종료까지를 이 프레임의 전체 시간으로 기록한다.
	const auto elapsed = Clock::now() - frameStart_;
	currentFrame_.milliseconds[static_cast<std::size_t>(ProfileCategory::Frame)] =
		Milliseconds(elapsed).count();

	// 배열 끝에 도달하면 처음으로 돌아가 가장 오래된 프레임을 덮어쓴다.
	history_[historyWriteIndex_] = currentFrame_;
	historyWriteIndex_ = (historyWriteIndex_ + 1) % kHistorySize;
	if (historyCount_ < kHistorySize)
	{
		++historyCount_;
	}
}

void Profiler::AddTime(ProfileCategory category, double milliseconds)
{
	currentFrame_.milliseconds[static_cast<std::size_t>(category)] += milliseconds;
}

void Profiler::Increment(ProfileCounter counter, std::uint64_t amount)
{
	currentFrame_.counters[static_cast<std::size_t>(counter)] += amount;
}

ProfileSnapshot Profiler::GetAverage() const
{
	ProfileSnapshot result{};
	if (historyCount_ == 0)
	{
		return result;
	}

	// 슬롯의 시간 순서는 평균에 영향을 주지 않으므로 채워진 항목을 모두 합산한다.
	for (std::size_t i = 0; i < historyCount_; ++i)
	{
		for (std::size_t category = 0; category < result.milliseconds.size(); ++category)
		{
			result.milliseconds[category] += history_[i].milliseconds[category];
		}

		for (std::size_t counter = 0; counter < result.counters.size(); ++counter)
		{
			result.counters[counter] += static_cast<double>(history_[i].counters[counter]);
		}
	}

	const double sampleCount = static_cast<double>(historyCount_);
	for (double& value : result.milliseconds)
	{
		value /= sampleCount;
	}
	for (double& value : result.counters)
	{
		value /= sampleCount;
	}

	return result;
}
