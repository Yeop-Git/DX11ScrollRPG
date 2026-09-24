#pragma once

#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>

// 시간 측정 결과를 구분한다. 하위 구간은 상위 Update 시간에 포함될 수 있다.
enum class ProfileCategory : std::size_t
{
	Frame,
	MessagePump,
	Update,
	EntityUpdate,
	Physics,
	GroundCollision,
	CombatCollision,
	ItemCollision,
	Render,
	Present,
	Count
};

// 프레임마다 발생 횟수를 합산하는 항목.
enum class ProfileCounter : std::size_t
{
	ActiveEntities,
	SpriteDraws,
	DrawCalls,
	CollisionChecks,
	Count
};

// 최근 프레임 기록에서 계산한 시간 및 카운터 평균값을 외부에 전달한다.
struct ProfileSnapshot
{
	// 시간은 밀리초, 카운터는 프레임 평균 횟수로 보관한다.
	std::array<double, static_cast<std::size_t>(ProfileCategory::Count)> milliseconds{};
	std::array<double, static_cast<std::size_t>(ProfileCounter::Count)> counters{};

	double GetMilliseconds(ProfileCategory category) const
	{
		return milliseconds[static_cast<std::size_t>(category)];
	}

	double GetCounter(ProfileCounter counter) const
	{
		return counters[static_cast<std::size_t>(counter)];
	}

	double GetAverageFPS() const
	{
		// 평균 프레임 시간의 역수로 전체 처리량 기준 FPS를 계산한다.
		const double frameMilliseconds = GetMilliseconds(ProfileCategory::Frame);
		return frameMilliseconds > 0.0 ? 1000.0 / frameMilliseconds : 0.0;
	}
};

// 프레임별 측정값을 모으고 최근 기록을 고정 크기 배열에 보관한다.
class Profiler
{
public:
	// 60~120 프레임 평균을 확인할 수 있도록 최대 120개 프레임을 유지한다.
	static constexpr std::size_t kHistorySize = 120;

	// 프레임 경계에서 현재 누적값을 초기화하고 기록한다.
	void BeginFrame();
	void EndFrame();
	// 측정 구간의 시간과 카운터를 현재 프레임 데이터에 더한다.
	void AddTime(ProfileCategory category, double milliseconds);
	void Increment(ProfileCounter counter, std::uint64_t amount = 1);
	// 기록된 프레임이 120개보다 적으면 실제 기록 개수만 사용한다.
	ProfileSnapshot GetAverage() const;

private:
	// steady_clock은 시스템 시각 보정에 영향을 받지 않아 경과 시간 측정에 적합하다.
	using Clock = std::chrono::steady_clock;
	// Profiler 내부 시간 단위는 출력에서 읽기 쉬운 밀리초로 통일한다.
	using Milliseconds = std::chrono::duration<double, std::milli>;

	struct FrameData
	{
		std::array<double, static_cast<std::size_t>(ProfileCategory::Count)> milliseconds{};
		std::array<std::uint64_t, static_cast<std::size_t>(ProfileCounter::Count)> counters{};
	};

	// 현재 프레임 임시 누적값과 최근 프레임 순환 기록.
	FrameData currentFrame_{};
	std::array<FrameData, kHistorySize> history_{};
	std::size_t historyWriteIndex_ = 0;
	std::size_t historyCount_ = 0;
	Clock::time_point frameStart_{};
};

// 스코프 수명 동안 시간을 재고 소멸 시 Profiler에 기록하는 RAII 타이머.
class ProfileScope
{
public:
	// Profiler는 소유하지 않고 참조만 빌린다. 이 객체보다 오래 살아야 한다.
	ProfileScope(Profiler& profiler, ProfileCategory category)
		: profiler_(profiler), category_(category), start_(std::chrono::steady_clock::now())
	{
	}

	~ProfileScope()
	{
		// 조기 return을 포함해 이 객체가 소멸하는 모든 범위 종료 경로에서 기록한다.
		const auto elapsed = std::chrono::steady_clock::now() - start_;
		profiler_.AddTime(
			category_,
			std::chrono::duration<double, std::milli>(elapsed).count());
	}

	// 복사하면 같은 구간 시간이 두 번 기록될 수 있어 스코프 타이머 복제를 금지한다.
	ProfileScope(const ProfileScope&) = delete;
	ProfileScope& operator=(const ProfileScope&) = delete;

private:
	Profiler& profiler_;
	ProfileCategory category_;
	std::chrono::steady_clock::time_point start_;
};
