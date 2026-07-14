#pragma once
#include <array>
#include <cassert>

template <class T, unsigned int MAX>
class LinkedArraySeries 
{	
	struct LnkArr
	{
		LnkArr(const uint32_t& n) : arr[n]
		{ len = n; };
		array<T> arr;
		uint32_t len;
		LnkArr* next = nullptr;
		void linkTo(LnkArr& l) { next = &l; }
	};

public:
	// <T, AMAX> series holds at most MAX arrays (nodes) which can own objects of type T
	LinkedArraySeries(const uint32_t& firstArrayLength)
	{
		assert(initArrayLength > 0);
		elementSizeBytes = sizeof(T);
		series.push_back(std::move(new T[n]));
	}
	// deletes everything in the container
	LinkedArraySeries(const uint32_t& arrSize)
	{

	}
	void initSeries(const uint32_t& n) { series[n]; }

	size_t nodeSizeBytes;
	std::array<LnkArr*, MAX> series;


};
