#pragma once
#include <array>

struct AttackInputResult
{
	int slot = -1;
	bool restart = false;
};
class AttackInput
{
public:
	AttackInputResult Sample(const std::array<bool, 5>& down, bool resetDown, bool focused, bool canRestart)
	{
		AttackInputResult result;
		for (size_t i = 0; i < down.size(); ++i)
		{
			// Ctrl은 유지 입력, 스킬은 상승 에지. 실패해도 낮은 우선순위로 대체하지 않는다.
			if (focused && down[i] && (i == 0 || !previous_[i]))
				if (result.slot < 0 || result.slot == 0) result.slot = static_cast<int>(i);
		}
		result.restart = focused && canRestart && resetDown && !previousReset_;
		previous_ = down;
		previousReset_ = resetDown;
		if (result.restart) result.slot = -1;
		return result;
	}

private:
	std::array<bool, 5> previous_{};
	bool previousReset_ = false;
};
