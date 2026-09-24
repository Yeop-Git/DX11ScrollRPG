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
	// 게임 HUD와 Profiler UI 렌더 시간을 Scene Render와 분리한다.
	UIRender,
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
	// 스트레스 Monster와 Player 몸체가 실제로 겹친 프레임당 평균 횟수.
	StressCollisionOverlaps,
	Count
};

// 최근 프레임 기록에서 계산한 시간 및 카운터 평균값을 외부에 전달한다.
struct ProfileSnapshot
{
	// 최근 기록의 프레임당 평균 시간(ms)과 평균 발생 횟수를 보관한다.
	std::array<double, static_cast<std::size_t>(ProfileCategory::Count)> averageMilliseconds{};
	std::array<double, static_cast<std::size_t>(ProfileCounter::Count)> averageCounters{};

	// 항목별 시간 및 평균 카운터 값을 읽는다.
	double GetMilliseconds(ProfileCategory category) const
	{
		return averageMilliseconds[static_cast<std::size_t>(category)];
	}

	double GetCounter(ProfileCounter counter) const
	{
		return averageCounters[static_cast<std::size_t>(counter)];
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
	// 짧은 변동과 최근 성능 변화를 함께 볼 수 있도록 최근 최대 120프레임을 유지한다.
	static constexpr std::size_t kHistorySize = 120;

	// 프레임 측정을 시작하고 완료된 측정값을 기록한다.
	void BeginFrame();
	void EndFrame();
	// 테스트 구성이 바뀐 경우 이전 프레임 기록을 비운다.
	void ResetHistory();
	// 측정 구간의 시간과 발생 횟수를 현재 프레임에 누적한다.
	void AddTime(ProfileCategory category, double milliseconds);
	void Increment(ProfileCounter counter, std::uint64_t amount = 1);
	// 저장된 최근 프레임의 평균을 UI에 전달할 복사본으로 만든다.
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

	// currentFrame_은 진행 중인 프레임, history_는 최근 완료 프레임을 보관한다.
	FrameData currentFrame_{};
	std::array<FrameData, kHistorySize> history_{};
	std::size_t historyWriteIndex_ = 0;
	std::size_t historyCount_ = 0;
	Clock::time_point frameStart_{};
};

// 선언된 코드 블록의 실행 시간을 재고 블록 종료 시 Profiler에 기록한다.
class ProfileScope
{
public:
	// Profiler는 Application 소유 객체를 참조만 하며, 이 Scope보다 오래 살아야 한다.
	ProfileScope(Profiler& profiler, ProfileCategory category);
	~ProfileScope();

	// 복사된 Scope가 같은 구간을 중복 기록하지 않도록 복사와 대입을 막는다.
	ProfileScope(const ProfileScope&) = delete;
	ProfileScope& operator=(const ProfileScope&) = delete;

private:
	using Clock = std::chrono::steady_clock;

	Profiler& profiler_;
	ProfileCategory category_;
	Clock::time_point startTime_;
};
