#pragma once
#include <cmath>
#include <numeric>

namespace Math
{
	#define EPSILON_F std::numeric_limits<float>::epsilon()

	// returns multiple of x that is closest to v
	template<typename T = float>
	T closestMultiple(T v, const T& x) 
	{
		if (x > v) { return x; }
		v = v + (x / 2);
		v = v - fmod(v, x);
		return v;
	}

	// returns multiple of m that is closest to but >= v
	template<typename T = uint32_t>
	T roundUpToClosestMultiple(const T& v, const T& m) 
	{
		if (m == 0) { return v; }
		const T remainder = v % m;
		if (remainder == 0) { return v; }
		return v + m - remainder;
	}

	template<typename T = float>
	T invSqrt(const T& v) { return 1.0 / sqrt(v); }

}


