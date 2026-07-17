#pragma once

#include <stdint.h>
#include <glm/glm.hpp>
#include "core/types/Math.h"

#include <string>
#include <filesystem>

template<typename T = float>
class Vector3D
{
public:
	Vector3D<T>(const T& x_, const T& y_, const T& z_) : x{ x_ }, y{ y_ }, z{ z_ } {};
	Vector3D<T>(const T& v) : x{ v }, y{ v }, z{ v } {};
	Vector3D() : x{ 0 }, y{ 0 }, z{ 0 } {};
	T x; T y; T z;
	// operator mess, can be ignored
	Vector3D<T> operator+(const Vector3D<T>& v) const { return Vector3D{ x + v.x, y + v.y, z + v.z }; } // +
	Vector3D<T> operator-(const Vector3D<T>& v) const { return Vector3D{ x - v.x, y - v.y, z - v.z }; } // -
	Vector3D<T> operator*(const Vector3D<T>& v) const { return Vector3D{ x * v.x, y * v.y, z * v.z }; } // *
	Vector3D<T> operator/(const Vector3D<T>& v) const { return Vector3D{ x / v.x, y / v.y, z / v.z }; } // /
	Vector3D<T> operator+=(const Vector3D<T>& v) { *this = *this + v; return *this; } // Vector += Vector
	Vector3D<T> operator-=(const Vector3D<T>& v) { *this = *this - v; return *this; } // Vector -= Vector
	Vector3D<T> operator*=(const Vector3D<T>& v) { *this = *this * v; return *this; } // Vector *= Vector

	Vector3D<float> operator+(const float& f) const { return Vector3D{ x + f, y + f, z + f }; } // Vector + float
	Vector3D<float> operator-(const float& f) const { return Vector3D{ x - f, y - f, z - f }; } // Vector - float
	Vector3D<float> operator*(const float& f) const { return Vector3D{ x * f, y * f, z * f }; } // Vector * float
	Vector3D<float> operator+=(const float& f) { *this = *this + f; return *this; } // Vector += float
	Vector3D<float> operator-=(const float& f) { *this = *this - f; return *this; } // Vector -= float
	Vector3D<float> operator*=(const float& f) { *this = *this * f; return *this; } // Vector *= float
	
#ifdef GLM_VERSION
	Vector3D<T>(const glm::vec3& g) : x{ g.x }, y{ g.y }, z{ g.z } {};
	operator glm::vec3() const { return glm::vec3( x, y, z ); }
#endif
	static auto dot(const Vector3D<T>& a, const Vector3D<T>& b) 
		{ return (a.x * b.x) + (a.y * b.y) + (a.z * b.z); }
	static Vector3D<T> cross(const Vector3D<T>& a, const Vector3D<T>& b) 
		{ return Vector3D<T>((a.y * b.z - b.y * a.z), (a.z * b.x - b.z * a.x), (a.x * b.y - b.x * a.y)); }
	Vector3D<T> getNormalized() const 
	{ 
		const auto sum = dot(*this, *this); // magnitude squared
		if (sum < EPSILON_F) { return Vector3D(); }
		return *this * Math::invSqrt(sum);
	}
	bool normalize(const T& tolerance = EPSILON_F)
	{
		const auto sum = dot(*this, *this); // magnitude squared
		if (sum > tolerance)
		{
			*this = *this * Math::invSqrt(sum);
			return true;
		}
		return false;
	}
	const T& getMagnitude() const { return std::sqrt(x * x + y * y + z * z); }
	static float distanceSquared(const Vector3D<T>& a, const Vector3D<T>& b) { return pow(b.x - a.x,2) + pow(b.y - a.y,2) + pow(b.z - a.z,2); }
	static float distance(const Vector3D<T>& a, const Vector3D<T>& b) { return sqrt(distanceSquared(a, b)); }
	static auto direction(Vector3D<float> a, Vector3D<float> b) { return Vector3D<float>(b - a).getNormalized(); }
	//Vector3D<T>& zero() { x = 0; y = 0; z = 0; return *this; }
	static Vector3D<T> zero() { return Vector3D<T>(); }
};
// shorthand (alias) for a 3D float Vector, always use this unless you need double precision
using Vec = Vector3D<float>;
using Vec64 = Vector3D<double>;
// additional float-Vector operators
//Vec operator+(Vec v, float f) { return v + Vec(f, f, f); } // Vector + float
//Vec operator-(Vec v, float f) { return v - Vec(f, f, f); } // Vector - float
//Vec operator*(Vec v, float f) { return v * Vec(f, f, f); } // Vector * float
//Vec operator+(float f, Vec v) { return Vec(v.x + f, v.y + f, v.z + f); } // float + Vector
//Vec operator*(float f, Vec v) { return Vec(v.x * f, v.y * f, v.z * f); } // float * Vector

template<typename T = float>
class Vector2D 
{
public:
	Vector2D<T>(const T& x_, const T& y_) : x{ x_ }, y{ y_ } {};
	Vector2D<T>(const T& v) : x{ v }, y{ v } {};
	Vector2D() : x{ 0 }, y{ 0 } {};
	T x; T y;
	Vector2D operator+(const Vector2D& other) { return Vector2D{ x + other.x, y + other.y }; }
	Vector2D operator-(const Vector2D& other) { return Vector2D{ x - other.x, y - other.y }; }
	Vector2D operator*(const Vector2D& other) { return Vector2D{ x * other.x, y * other.y }; }
	Vector2D operator/(const Vector2D& other) { return Vector2D{ x / other.x, y / other.y }; }
	friend bool operator==(const Vector2D& lh, const Vector2D& rh) { return lh.x == rh.x && lh.y == rh.y; }
	friend bool operator!=(const Vector2D& lh, const Vector2D& rh) { return !(lh == rh); }
#ifdef GLM_VERSION
	Vector2D<T>(const glm::vec2& g) : x{ g.x }, y{ g.y } {};
	operator glm::vec2() const { return glm::vec2(x, y); }
#endif
};

class VectorInt
{
public:
	VectorInt(const uint32_t& x_, const uint32_t& y_, const uint32_t& z_) : x{ x_ }, y{ y_ }, z{ z_ } {};
	VectorInt(const uint32_t& v) : x{ v }, y{ v }, z{ v } {};
	VectorInt() : x{ 0 }, y{ 0 }, z{ 0 } {};
	uint32_t x; uint32_t y; uint32_t z;
	VectorInt operator+(const VectorInt& other) { return VectorInt{ x + other.x, y + other.y, z + other.z }; }
	VectorInt operator-(const VectorInt& other) { return VectorInt{ x - other.x, y - other.y, z - other.z }; }
};

static std::string makePath(const char* pathIn)
{
	// assumes the application is running from a subdirectory under the project root
	return (std::filesystem::current_path() / "../resources" / std::filesystem::path(pathIn)).string();
}

static std::string makePath(std::filesystem::path p)
{
	return (std::filesystem::current_path() / "../resources" / p).string();
}