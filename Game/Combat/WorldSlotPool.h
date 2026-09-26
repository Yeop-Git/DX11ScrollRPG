#pragma once
#include "PoolHandle.h"
#include <optional>
#include <stdexcept>
#include <vector>

namespace PoolIdentity
{
	// Pool 생성도 메인 스레드에서만 수행. 번호는 월드 재생성 후에도 재사용하지 않는다.
	inline uint64_t next = 1;
	inline uint64_t Allocate()
	{
		if (next == UINT64_MAX) throw std::overflow_error("Pool identity exhausted");
		return next++;
	}
} // namespace PoolIdentity
// World가 객체를 소유한다. 이 레지스트리는 메인 스레드에서 사용 회차만 관리한다.
template <class T> class WorldSlotPool
{
	struct Slot
	{
		T* object;
		uint64_t generation = 1;
		bool used = false;
	};

public:
	using Handle = PoolHandle<T>;
	WorldSlotPool() : storageId_(PoolIdentity::Allocate()) {}
	WorldSlotPool(const WorldSlotPool&) = delete;
	WorldSlotPool& operator=(const WorldSlotPool&) = delete;
	void Initialize(const std::vector<T*>& objects)
	{
		storageId_ = PoolIdentity::Allocate();
		slots_.clear();
		free_.clear();
		for (T* object : objects)
		{
			object->SetActive(false);
			free_.push_back(static_cast<uint32_t>(slots_.size()));
			slots_.push_back({object});
		}
	}
	template <class Initializer> std::optional<Handle> TryAcquire(Initializer initialize)
	{
		if (free_.empty()) return std::nullopt;
		const auto index = free_.back();
		auto& slot = slots_[index];
		// 초기화 실패 시 free 목록은 그대로 유지. 활성화는 모든 상태 설정 이후 수행한다.
		initialize(*slot.object);
		free_.pop_back();
		slot.used = true;
		slot.object->SetActive(true);
		return Handle{storageId_, index, slot.generation};
	}
	T* Lookup(Handle handle) const
	{
		if (handle.storageId != storageId_ || handle.slot >= slots_.size()) return nullptr;
		const auto& slot = slots_[handle.slot];
		return slot.used && slot.generation == handle.generation ? slot.object : nullptr;
	}
	bool Release(Handle handle)
	{
		T* object = Lookup(handle);
		if (!object) return false;
		object->SetActive(false);
		auto& slot = slots_[handle.slot];
		slot.used = false;
		if (slot.generation != UINT64_MAX)
		{
			++slot.generation;
			free_.push_back(handle.slot);
		}
		return true;
	}
	std::vector<Handle> ActiveHandles() const
	{
		std::vector<Handle> result;
		for (uint32_t i = 0; i < slots_.size(); ++i)
			if (slots_[i].used) result.push_back({storageId_, i, slots_[i].generation});
		return result;
	}
	void Reset()
	{
		for (auto handle : ActiveHandles())
			Release(handle);
	}

private:
	uint64_t storageId_;
	std::vector<Slot> slots_;
	std::vector<uint32_t> free_;
};
