#pragma once
#include "../Math/Vector2.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>
#include <vector>

namespace UIRadialFill
{
	using Triangle = std::array<Vector2, 3>;
	// 화면 좌표 기준: 위쪽 -Y, 12시에서 시계방향. 정사각형 모서리를 정확히 포함한다.
	inline Vector2 Boundary(double angle)
	{
		const double x = std::sin(angle), y = -std::cos(angle);
		const double scale = (std::max)(std::abs(x), std::abs(y));
		return {static_cast<float>(x / scale), static_cast<float>(y / scale)};
	}
	inline std::vector<Triangle> Remaining(float fraction)
	{
		std::vector<Triangle> triangles;
		if (!std::isfinite(fraction) || fraction <= 0.0f)
			return triangles;
		constexpr double pi = std::numbers::pi;
		const double start = (1.0 - (std::min)(fraction, 1.0f)) * 2.0 * pi;
		auto previous = Boundary(start);
		// 남은 영역은 회전 경계부터 12시까지. 감소할수록 밝은 부분이 시계방향으로 커진다.
		for (const double angle : {pi / 4, 3 * pi / 4, 5 * pi / 4, 7 * pi / 4, 2 * pi})
		{
			if (angle <= start)
				continue;
			const auto next = Boundary(angle);
			triangles.push_back({Vector2{}, previous, next});
			previous = next;
		}
		return triangles;
	}
} // namespace UIRadialFill
