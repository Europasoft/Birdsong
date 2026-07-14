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

struct Transform
{
	Vec translation{};
	Vec rotation{};
	Vec scale{ 1.f, 1.f, 1.f };

	Transform() = default;
	Transform(const Vec& t, const Vec& r = Vec::zero(), const Vec& s = Vec::zero())
		: translation{ t }, rotation{ r }, scale{ s } {};

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

struct ActorTransform
{
	const Transform& get() const { return transform; }
	void set(const Transform& tf) { transform = tf; updated = true; }
	void resetUpdatedFlag() { updated = false; }
	const bool& wasUpdated() const { return updated; }
private:
	Transform transform{};
	bool updated = true;
};

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
	return (std::filesystem::current_path() / "../Src/Core/DevResources" / std::filesystem::path(pathIn)).string();
}