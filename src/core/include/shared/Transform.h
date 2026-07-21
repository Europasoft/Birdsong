#pragma once
#include "core/types/CommonTypes.h"

#include <cstdint>
#include <utility>

namespace WorldSystem
{
	using SectorInt = intmax_t;

	class SectorCoord
	{
	public:

		SectorCoord() : x{ 0 }, y{ 0 }, z{ 0 } {};
		SectorCoord(SectorInt x, SectorInt y, SectorInt z) : x{ x }, y{ y }, z{ z } {};
		SectorInt x, y, z;
		bool operator==(const SectorCoord& s) const { return x == s.x && y == s.y && z == s.z; } // ==
		bool operator!=(const SectorCoord& s) const { return !(s == *this); } // !=
		SectorCoord operator+(const SectorCoord& s) const { return SectorCoord{ x + s.x, y + s.y, z + s.z }; } // +
		SectorCoord operator-(const SectorCoord& s) const { return SectorCoord{ x - s.x, y - s.y, z - s.z }; } // -
		SectorCoord operator+=(const SectorCoord& s) { *this = s + *this; return *this; } // +=
		SectorCoord operator-=(const SectorCoord& s) { *this = s - *this; return *this; } // -=
	};
}

struct Transform
{
	using SC = WorldSystem::SectorCoord;

	Vec translation;
	Vec rotation;
	float rotation_w = 1.f; // TODO: should store rotations properly as quat
	Vec scale{ 1.f, 1.f, 1.f };
	SC sector;

	Transform() = default;
	Transform(const Vec& t, const Vec& r = Vec::zero(), const Vec& s = Vec::one(), const SC& sec = SC())
		: translation{ t }, rotation{ r }, scale{ s }, sector{ sec } {};

	static constexpr uint64_t getPackedSize()
	{
		return (sizeof(decltype(std::declval<Transform>().translation.x)) * 9) + (sizeof(decltype(std::declval<Transform>().sector.x)) * 3);
	}

};