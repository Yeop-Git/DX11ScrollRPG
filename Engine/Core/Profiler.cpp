#include "Profiler.h"

void Profiler::BeginFrame()
{
	// 현재 프레임 측정값을 비우고 전체 프레임 측정의 시작 시각을 저장한다.
	currentFrame_ = {};
	frameStart_ = Clock::now();
}

void Profiler::EndFrame()
{
	// BeginFrame부터 Present 완료까지의 시간을 전체 프레임 시간으로 기록한다.
	const auto elapsed = Clock::now() - frameStart_;
	currentFrame_.milliseconds[static_cast<std::size_t>(ProfileCategory::Frame)] =
		Milliseconds(elapsed).count();

	// 순환 배열 끝에 도달하면 가장 오래된 프레임부터 새 값으로 교체한다.
	history_[historyWriteIndex_] = currentFrame_;
	historyWriteIndex_ = (historyWriteIndex_ + 1) % kHistorySize;
	if (historyCount_ < kHistorySize)
	{
		++historyCount_;
	}
}

void Profiler::ResetHistory()
{
	// 월드 재구성 비용과 이전 구성의 측정값이 새 평균에 포함되지 않게 한다.
	historyWriteIndex_ = 0;
	historyCount_ = 0;
	gpuHistoryWriteIndex_ = 0;
	gpuHistoryCount_ = 0;
	currentFrame_ = {};
	frameStart_ = Clock::now();
}

void Profiler::AddTime(ProfileCategory category, double milliseconds)
{
	currentFrame_.milliseconds[static_cast<std::size_t>(category)] += milliseconds;
}

void Profiler::Increment(ProfileCounter counter, std::uint64_t amount)
{
	currentFrame_.counters[static_cast<std::size_t>(counter)] += amount;
}

void Profiler::AddGpuSample(double milliseconds, std::uint64_t pixelShaderInvocations)
{
	// GPU 결과 도착 시점과 제출 프레임이 다르므로 CPU 프레임 배열과 별도로 평균 낸다.
	gpuHistory_[gpuHistoryWriteIndex_] = { milliseconds, pixelShaderInvocations };
	gpuHistoryWriteIndex_ = (gpuHistoryWriteIndex_ + 1) % kHistorySize;
	if (gpuHistoryCount_ < kHistorySize)
	{
		++gpuHistoryCount_;
	}
}

ProfileSnapshot Profiler::GetAverage() const
{
	ProfileSnapshot result{};
	if (historyCount_ == 0)
	{
		return result;
	}

	// 저장 순서는 평균에 영향을 주지 않으므로 기록된 프레임을 모두 더한다.
	for (std::size_t i = 0; i < historyCount_; ++i)
	{
		for (std::size_t category = 0;
			category < result.averageMilliseconds.size();
			++category)
		{
			result.averageMilliseconds[category] += history_[i].milliseconds[category];
		}

		for (std::size_t counter = 0;
			counter < result.averageCounters.size();
			++counter)
		{
			result.averageCounters[counter] +=
				static_cast<double>(history_[i].counters[counter]);
		}
	}

	// 배열에는 유효한 기록만 평균 내므로 시작 직후에도 값이 작게 왜곡되지 않는다.
	const double sampleCount = static_cast<double>(historyCount_);
	for (double& value : result.averageMilliseconds)
	{
		value /= sampleCount;
	}
	for (double& value : result.averageCounters)
	{
		value /= sampleCount;
	}

	if (gpuHistoryCount_ > 0)
	{
		result.hasGpuSamples = true;
		for (std::size_t i = 0; i < gpuHistoryCount_; ++i)
		{
			result.averageGpuMilliseconds += gpuHistory_[i].milliseconds;
			result.averagePixelShaderInvocations +=
				static_cast<double>(gpuHistory_[i].pixelShaderInvocations);
		}

		const double gpuSampleCount = static_cast<double>(gpuHistoryCount_);
		result.averageGpuMilliseconds /= gpuSampleCount;
		result.averagePixelShaderInvocations /= gpuSampleCount;
	}

	return result;
}

ProfileScope::ProfileScope(Profiler& profiler, ProfileCategory category)
	: profiler_(profiler)
	, category_(category)
	, startTime_(Clock::now())
{
}

ProfileScope::~ProfileScope()
{
	// 조기 return을 포함해 블록이 끝날 때 실제 경과 시간을 기록한다.
	const auto elapsed = Clock::now() - startTime_;
	const double elapsedMilliseconds =
		std::chrono::duration<double, std::milli>(elapsed).count();
	profiler_.AddTime(category_, elapsedMilliseconds);
}
