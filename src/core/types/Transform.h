#pragma once
#include "core/types/CommonTypes.h"

#include <cstdint>

namespace WorldSystem
{
	using SectorInt = intmax_t;

	class SectorCoord
	{
	public:
		SectorCoord();
		SectorCoord(SectorInt x, SectorInt y, SectorInt z);
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
	Vec translation;
	WorldSystem::SectorCoord sector;
	Vec rotation;
	Vec scale{ 1.f, 1.f, 1.f };

	Transform() = default;
	Transform(const Vec& t, const Vec& r = Vec::zero(), const Vec& s = Vec::zero(), const WorldSystem::SectorCoord& sec = WorldSystem::SectorCoord())
		: translation{ t }, rotation{ r }, scale{ s }, sector{ sec } {};

	std::vector<uint8_t> packData(const MyData& data)
	{
		// Pre-calculate exact size to avoid reallocations
		constexpr size_t bufferSize = sizeof(data.velocity) +
			sizeof(data.acceleration) +
			sizeof(data.timestamp) +
			sizeof(data.id);

		std::vector<uint8_t> buffer(bufferSize);
		uint8_t* ptr = buffer.data();

		// Copy elements sequentially (Tightly packed, no padding blocks)
		std::memcpy(ptr, &data.velocity, sizeof(data.velocity));
		ptr += sizeof(data.velocity);

		std::memcpy(ptr, &data.acceleration, sizeof(data.acceleration));
		ptr += sizeof(data.acceleration);

		std::memcpy(ptr, &data.timestamp, sizeof(data.timestamp));
		ptr += sizeof(data.timestamp);

		std::memcpy(ptr, &data.id, sizeof(data.id));

		return buffer;
	}

	glm::mat4 mat4() const { return makeMatrix(rotation, scale, translation); }

	static glm::mat4 makeMatrix(const Vec& rotationIn, const Vec& scaleIn,
		const Vec& translationIn = { 0.f, 0.f, 0.f })
	{
		return makeMatrixDef(rotationIn, scaleIn, translationIn);
	}


	static glm::mat4 makeMatrixZYX(const Vec& rotationIn, const Vec& scaleIn,
		const Vec& translationIn = { 0.f, 0.f, 0.f })
	{
		/* returns Translation * Rz * Ry * Rx * Scale
		* Tait-bryan angles Z(1)-Y(2)-X(3) rotation order
		* https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix */
		const float c3 = glm::cos(rotationIn.x);
		const float s3 = glm::sin(rotationIn.x);
		const float c2 = glm::cos(rotationIn.y);
		const float s2 = glm::sin(rotationIn.y);
		const float c1 = glm::cos(rotationIn.z);
		const float s1 = glm::sin(rotationIn.z);
		return glm::mat4
		{
			{
				scaleIn.x * (c1 * c2),
				scaleIn.x * (c2 * s1),
				scaleIn.x * (-s2),
				0.0f,
			},
			{
				scaleIn.y * (c1 * s2 * s3 - c3 * s1),
				scaleIn.y * (c1 * c3 + s1 * s2 * s3),
				scaleIn.y * (c2 * s3),
				0.0f,
			},
			{
				scaleIn.z * (s1 * s3 + c1 * c3 * s2),
				scaleIn.z * (c3 * s1 * s2 - c1 * s3),
				scaleIn.z * (c2 * c3),
				0.0f,
			},
			{translationIn.x, translationIn.y, translationIn.z, 1.0f}
		};
	}

	static glm::mat4 makeMatrixDef(const Vec& rotationIn, const Vec& scaleIn,
		const Vec& translationIn = { 0.f, 0.f, 0.f })
	{
		/* returns Translation * Rx * Ry * Rz * Scale
		* Tait-bryan angles X(1)-Y(2)-Z(3) rotation order
		* https://en.wikipedia.org/wiki/Euler_angles#Rotation_matrix */
		const float c3 = glm::cos(rotationIn.z);
		const float s3 = glm::sin(rotationIn.z);
		const float c2 = glm::cos(rotationIn.y);
		const float s2 = glm::sin(rotationIn.y);
		const float c1 = glm::cos(rotationIn.x);
		const float s1 = glm::sin(rotationIn.x);
		return glm::mat4
		{
			{
				scaleIn.x * (c2 * c3),
				scaleIn.x * (c1 * s3 + c3 * s1 * s2),
				scaleIn.x * (s1 * s3 - c1 * c3 * s2),
				0.0f,
			},
			{
				scaleIn.y * -(c2 * s3),
				scaleIn.y * (c1 * c3 - s1 * s2 * s3),
				scaleIn.y * (c3 * s1 + c1 * s2 * s3),
				0.0f,
			},
			{
				scaleIn.z * (s2),
				scaleIn.z * -(c2 * s1),
				scaleIn.z * (c1 * c2),
				0.0f,
			},
			{translationIn.x, translationIn.y, translationIn.z, 1.0f}
		};
	}

	static glm::mat4 makeMatrixLve(const Vec& rotationIn, const Vec& scaleIn,
		const Vec& translationIn = { 0.f, 0.f, 0.f })
	{
		const float c3 = glm::cos(rotationIn.z);
		const float s3 = glm::sin(rotationIn.z);
		const float c2 = glm::cos(rotationIn.x);
		const float s2 = glm::sin(rotationIn.x);
		const float c1 = glm::cos(rotationIn.y);
		const float s1 = glm::sin(rotationIn.y);
		return glm::mat4{
			{
				scaleIn.x * (c1 * c3 + s1 * s2 * s3),
				scaleIn.x * (c2 * s3),
				scaleIn.x * (c1 * s2 * s3 - c3 * s1),
				0.0f,
			},
			{
				scaleIn.y * (c3 * s1 * s2 - c1 * s3),
				scaleIn.y * (c2 * c3),
				scaleIn.y * (c1 * c3 * s2 + s1 * s3),
				0.0f,
			},
			{
				scaleIn.z * (c2 * s1),
				scaleIn.z * (-s2),
				scaleIn.z * (c1 * c2),
				0.0f,
			},
			{translationIn.x, translationIn.y, translationIn.z, 1.0f} };
	}

	static glm::mat4 makeMatrix(const Vec& rotationIn) { return makeMatrix(rotationIn, { 1.f, 1.f, 1.f }); }

	Vec getForwardVector() const
	{
		// get "forward" unit vector (x axis with rotation applied)
		return Transform::rotateVector(Vec{ 1.f, 0.f, 0.f }, rotation);
	}

	static double degToRad(const double& degrees) { return (degrees * 0.01745329251); }

	static Vec rotateVector(const Vec& v3, const Vec& rot)
	{
		// returns vector rotated by rotation matrix
		glm::vec4 r = Transform::makeMatrix(rot) * glm::vec4{ v3.x, v3.y, v3.z, 0.f };
		return Vec(r.x, r.y, r.z);
	}

	static Vec rotateVectorQuaternion(const Vec& v3, const glm::vec4& rot)
	{
		const glm::vec4 v4 = { v3.x, v3.y, v3.z, 0.f };
		glm::vec4 qw = { rot.w, rot.w, rot.w, rot.w };
		// cross product of vector and quat-rotation, times 2
		glm::vec4 t = crossVec4(rot, v4);
		t = t + t;
		const glm::vec4 r = ((qw * t) + v4) + crossVec4(rot, t);
		return { r.x, r.y, r.z };
	}

	// replacement for missing overload of cross() that accepts 4-component vectors (w=0)
	static glm::vec4 crossVec4(const glm::vec4& a, const glm::vec4& b)
	{
		const auto r = Vec::cross({ a.x, a.y, a.z }, { b.x, b.y, b.z });
		return glm::vec4(r.x, r.y, r.z, 0.f);
	}

	// assumes yaw = z, pitch = y, roll = x
	static glm::vec4 quaternionFromRotation(const Vec& v)
	{
		// abbreviations for the angular functions
		const auto cy = cos(v.z); // in original formula all components were * 0.5 (halved)
		const auto sy = sin(v.z);
		const auto cp = cos(v.y);
		const auto sp = sin(v.y);
		const auto cr = cos(v.x);
		const auto sr = sin(v.x);

		glm::vec4 q{};
		q.w = cr * cp * cy + sr * sp * sy;
		q.x = sr * cp * cy - cr * sp * sy;
		q.y = cr * sp * cy + sr * cp * sy;
		q.z = cr * cp * sy - sr * sp * cy;

		return q;
	}

};