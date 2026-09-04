#pragma once

struct Vector2
{
	float x = 0.0f;
	float y = 0.0f;

	constexpr Vector2& operator+=(const Vector2& other)
	{
		x += other.x;
		y += other.y;
		return *this;
	}
};

constexpr Vector2 operator+(Vector2 lv, const Vector2& rv)
{
	lv += rv;
	return lv;
}

constexpr Vector2 operator-(const Vector2& lv, const Vector2& rv)
{
	return { lv.x - rv.x, lv.y - rv.y };
}

constexpr Vector2 operator*(const Vector2& vector, float scalar)
{
	return { vector.x * scalar, vector.y * scalar };
}
