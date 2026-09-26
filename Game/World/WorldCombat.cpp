#include "GameWorld.h"
#include "../Player.h"
#include "../Monster.h"
#include "Ground.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>

void GameWorld::CreateCombatPools()
{
	std::vector<SkillObject*> skills;
	std::vector<HitEffect*> effects;
	for (unsigned i = 0; i < data_->skillPoolSize; ++i)
	{
		auto object = std::make_unique<SkillObject>();
		skills.push_back(object.get());
		AddGameObject(std::move(object), RenderLayer::TransparentSkill);
	}
	for (unsigned i = 0; i < data_->effectPoolSize; ++i)
	{
		auto object = std::make_unique<HitEffect>();
		effects.push_back(object.get());
		AddGameObject(std::move(object), RenderLayer::TransparentSkill);
	}
	skillPool_.Initialize(skills);
	effectPool_.Initialize(effects);
}
void GameWorld::ClearCombatObjects()
{
	skillPool_.Reset();
	effectPool_.Reset();
}
void GameWorld::SpawnMonster(Monster& monster)
{
	monster.Spawn(player_->GetStat().GetLevel());
	// 디버거/개발 로그에서 Spawn 레벨을 확인. 성능 테스트 Monster에는 출력하지 않는다.
	const auto message = "Monster spawned: Lv." + std::to_string(monster.GetStat().GetLevel()) + "\n";
	OutputDebugStringA(message.c_str());
}
DamageResult GameWorld::ApplyDamageToMonster(Monster& monster, const DamageRequest& request)
{
	if (!combatDamageEnabled_ || !stressMonsters_.empty() || !monster.IsActive() || player_->IsDead())
		return DamageResult::Ignored;
	const auto result = monster.TakeDamage(request);
	if (result == DamageResult::Killed) player_->GainExperience(monster.TakeKillReward());
	return result;
}
bool GameWorld::CaptureAttackInput()
{
	DWORD processId = 0;
	GetWindowThreadProcessId(GetForegroundWindow(), &processId);
	std::array<bool, 5> keys{};
	bool resetDown = false;
	for (size_t i = 0; i < keys.size(); ++i)
	{
		keys[i] = (GetAsyncKeyState(data_->attacks[i].virtualKey) & 0x8000) != 0;
		if (data_->attacks[i].virtualKey == 'R') resetDown = keys[i];
	}
	const auto input = attackInput_.Sample(keys, resetDown, processId == GetCurrentProcessId(),
										   player_->IsDead() && stressMonsters_.empty());
	if (input.restart)
	{
		Reset();
		return true;
	}
	player_->SubmitAttack(input.slot);
	return false;
}

AttackAvailability GameWorld::SpawnSkill(AttackSlot slot, Player& player)
{
	const auto& definition = data_->attacks.at(static_cast<size_t>(slot));
	const float direction = player.IsFacingRight() ? 1.0f : -1.0f;
	Vector2 position =
		player.transform.position + Vector2{definition.spawnOffset.x * direction, definition.spawnOffset.y};
	if (slot == AttackSlot::E || slot == AttackSlot::R)
	{
		bool found = false;
		for (const auto* ground : grounds_)
		{
			const auto box = ground->GetBodyBox();
			if (ground->IsActive() && ground->collider.enabled && position.x >= box.min.x &&
				position.x <= box.max.x)
			{
				position.y = box.max.y + definition.hitBox.y;
				found = true;
				break;
			}
		}
		if (!found) return AttackAvailability::NoGround;
	}
	const auto handle = skillPool_.TryAcquire(
		[&](SkillObject& object) { object.Spawn(slot, definition, position, player.IsFacingRight()); });
	if (!handle) return AttackAvailability::PoolFull;
	player.StartSkillCast();
	return AttackAvailability::Ready;
}
void GameWorld::SpawnHitEffect(Vector2 position)
{
	// 효과 부족은 피해 결과에 영향을 주지 않는다.
	effectPool_.TryAcquire([&](HitEffect& effect) { effect.Spawn(data_->hitEffect, position); });
}

namespace
{
	// 두 움직이는 AABB의 상대 경로를 확장된 목표 상자에 대한 선분으로 검사한다.
	bool Sweep(Vector2 from, Vector2 to, Vector2 extent, Vector2 targetFrom, Vector2 targetTo,
			   Vector2 targetExtent, float& time)
	{
		const Vector2 origin = from - targetFrom;
		const Vector2 delta = (to - from) - (targetTo - targetFrom);
		const Vector2 radius = extent + targetExtent;
		float enter = 0.0f, leave = 1.0f;
		const float o[] = {origin.x, origin.y}, d[] = {delta.x, delta.y}, r[] = {radius.x, radius.y};
		for (int axis = 0; axis < 2; ++axis)
		{
			if (std::abs(d[axis]) < 0.0000001f)
			{
				if (std::abs(o[axis]) > r[axis]) return false;
				continue;
			}
			float a = (-r[axis] - o[axis]) / d[axis], b = (r[axis] - o[axis]) / d[axis];
			if (a > b) std::swap(a, b);
			enter = (std::max)(enter, a);
			leave = (std::min)(leave, b);
			if (enter > leave) return false;
		}
		time = enter;
		return true;
	}
} // namespace
Vector2 GameWorld::MonsterPositionAt(size_t index, double elapsed, float deltaTime) const
{
	const float ratio = deltaTime > 0 ? static_cast<float>(elapsed / deltaTime) : 0.0f;
	const auto* monster = monsters_[index];
	return previousMonsterPositions_[index] +
		   (monster->transform.position - previousMonsterPositions_[index]) * ratio +
		   monster->collider.offset;
}

bool GameWorld::CanHitMonster(size_t index) const
{
	const auto* monster = monsters_[index];
	return monster->IsActive() && !monster->IsDead() && monster->collider.enabled;
}

void GameWorld::ApplySkillDamage(const SkillObject& skill, size_t index, Vector2 position)
{
	const auto reaction = skill.slot == AttackSlot::R ? DamageReaction::Periodic : DamageReaction::NormalHit;
	const DamageRequest request{skill.definition->damage, position.x, reaction};
	if (ApplyDamageToMonster(*monsters_[index], request) != DamageResult::Ignored) SpawnHitEffect(position);
}

void GameWorld::ResolvePeriodicSkill(SkillObject& skill, float deltaTime, Profiler& profiler)
{
	const auto& definition = *skill.definition;
	// 절대 틱 번호를 사용해 프레임 분할과 무관하게 t=0부터 만료 직전까지 한 번씩 처리한다.
	while (skill.nextTick * static_cast<double>(definition.hitInterval) < definition.lifetime &&
		   skill.nextTick * static_cast<double>(definition.hitInterval) <= skill.age)
	{
		const double tick = skill.nextTick++ * static_cast<double>(definition.hitInterval);
		const double elapsed = (std::max)(0.0, tick - skill.previousAge);
		const float distance =
			definition.speed * static_cast<float>(elapsed) * (skill.facingRight ? 1.0f : -1.0f);
		const Vector2 position = skill.previousPosition + Vector2{distance, 0};
		const AABB hitBox{position - definition.hitBox, position + definition.hitBox};
		for (size_t i = 0; i < monsters_.size(); ++i)
		{
			if (!CanHitMonster(i)) continue;
			const auto target = MonsterPositionAt(i, elapsed, deltaTime);
			const auto extent = monsters_[i]->collider.halfSize;
			profiler.Increment(ProfileCounter::CollisionChecks);
			if (Intersects(hitBox, {target - extent, target + extent})) ApplySkillDamage(skill, i, position);
		}
	}
}

bool GameWorld::ResolveContactSkill(SkillObject& skill, float deltaTime, Profiler& profiler)
{
	const auto& definition = *skill.definition;
	double begin = 0.0, end = skill.age - skill.previousAge;
	if (skill.slot == AttackSlot::E)
	{
		// 긴 프레임이 3~5번 프레임을 모두 통과해도 유효 시간의 교집합에서 검사한다.
		begin = (std::max)(skill.previousAge,
						   static_cast<double>(definition.firstActiveFrame) * definition.frameDuration) -
				skill.previousAge;
		end = (std::min)(skill.age,
						 static_cast<double>(definition.lastActiveFrame + 1) * definition.frameDuration) -
			  skill.previousAge;
		if (end <= begin) return false;
	}
	std::vector<std::pair<float, size_t>> hits;
	for (size_t i = 0; i < monsters_.size(); ++i)
	{
		if (!CanHitMonster(i)) continue;
		float time = 0.0f;
		profiler.Increment(ProfileCounter::CollisionChecks);
		if (Sweep(skill.previousPosition, skill.transform.position, definition.hitBox,
				  MonsterPositionAt(i, begin, deltaTime), MonsterPositionAt(i, end, deltaTime),
				  monsters_[i]->collider.halfSize, time))
			hits.push_back({time, i});
	}
	// 가장 가까운 접촉 우선. 동일 시각에는 기존 Monster 인덱스로 순서 고정.
	std::sort(hits.begin(), hits.end());
	for (const auto& [time, index] : hits)
	{
		if (!skill.RegisterContact(index, monsters_[index]->GetSpawnSerial())) continue;
		const auto position =
			skill.previousPosition + (skill.transform.position - skill.previousPosition) * time;
		ApplySkillDamage(skill, index, position);
		if (skill.slot == AttackSlot::Q) return true;
	}
	return false;
}

void GameWorld::UpdateSkills(float deltaTime, const std::vector<PoolHandle<SkillObject>>& handles,
							 Profiler& profiler)
{
	ProfileScope scope(profiler, ProfileCategory::CombatCollision);
	std::vector<PoolHandle<SkillObject>> returns;
	for (auto handle : handles)
	{
		auto* skill = skillPool_.Lookup(handle);
		if (!skill) continue;
		const auto& definition = *skill->definition;
		skill->previousPosition = skill->transform.position;
		skill->previousAge = skill->age;
		const double step =
			(std::min)(static_cast<double>(deltaTime), static_cast<double>(definition.lifetime) - skill->age);
		skill->age += step;
		skill->transform.position.x +=
			definition.speed * static_cast<float>(step) * (skill->facingRight ? 1.0f : -1.0f);
		bool returnOnContact = false;
		if (skill->slot == AttackSlot::R)
			ResolvePeriodicSkill(*skill, deltaTime, profiler);
		else
			returnOnContact = ResolveContactSkill(*skill, deltaTime, profiler);
		// 화면 이탈도 마지막 이동 구간의 충돌 확인 뒤 처리한다.
		if (returnOnContact || skill->age >= definition.lifetime ||
			std::abs(skill->transform.position.x) > 1.3f)
			returns.push_back(handle);
	}
	// 순회 중에는 free 슬롯으로 돌려놓지 않는다.
	for (auto handle : returns)
		skillPool_.Release(handle);
}
