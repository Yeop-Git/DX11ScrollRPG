#pragma once
#include <cstdint>
#include <limits>

template <class T> struct PoolHandle
{
	uint64_t storageId = 0;
	uint32_t slot = UINT32_MAX;
	uint64_t generation = 0;
	bool operator==(const PoolHandle&) const = default;
};
