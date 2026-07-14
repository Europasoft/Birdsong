// Copyright 2026 Simon Liimatainen. All rights reserved.
// these are internal utilities, this should not be included outside of private wrapper code

#pragma once
#include "include/b3cpp.h"

#include "box3d/include/box3d/box3d.h" // Box3D types used internally

#include <cstdint>
#include <math_functions.h>
#include <cstring> // for std::memcpy

namespace b3cpp
{
	namespace util
	{
		// world id
		inline b3WorldId getId(const b3cpp::World* w)
		{
			return b3LoadWorldId(w->getIdBuffer()[0]);
		}

		inline void setId(b3cpp::World* w, b3WorldId id)
		{
			w->getIdBuffer()[0] = b3StoreWorldId(id);
		}

		// body id
		inline b3BodyId getId(const b3cpp::Body* w)
		{
			uint64_t packed64 = 0;
			std::memcpy(&packed64, w->getIdBuffer(), sizeof(uint64_t));
			return b3LoadBodyId(packed64);
		}

		inline void setId(b3cpp::Body* w, b3BodyId id)
		{
			uint64_t packed64 = b3StoreBodyId(id);
			std::memcpy(w->getIdBuffer(), &packed64, sizeof(uint64_t));
		}

		// contact id
		inline b3ContactId getId(const b3cpp::Contact* w)
		{
			return b3LoadContactId(const_cast<uint32_t*>(w->getIdBuffer()));
		}

		inline void setId(b3cpp::Contact* w, b3ContactId id)
		{
			b3StoreContactId(id, w->getIdBuffer());
		}

		// shape id
		inline b3ShapeId getId(const b3cpp::Shape* w)
		{
			uint64_t packed64 = 0;
			std::memcpy(&packed64, w->getIdBuffer(), sizeof(uint64_t));
			return b3LoadShapeId(packed64);
		}

		inline void setId(b3cpp::Shape* w, b3ShapeId id)
		{
			uint64_t packed64 = b3StoreShapeId(id);
			std::memcpy(w->getIdBuffer(), &packed64, sizeof(uint64_t));
		}



		// vector conversions
		inline b3Vec3 vectorToB3(const Vector& v)
		{
			return b3Vec3
			{
				.x = static_cast<decltype(b3Vec3{}.x) > (v.x),
				.y = static_cast<decltype(b3Vec3{}.y) > (v.y),
				.z = static_cast<decltype(b3Vec3{}.z) > (v.z)
			};
		}

		inline b3Pos vectorToB3Pos(const Vector& v)
		{
			return b3Pos
			{
				.x = static_cast<decltype(b3Pos{}.x) > (v.x),
				.y = static_cast<decltype(b3Pos{}.y) > (v.y),
				.z = static_cast<decltype(b3Pos{}.z) > (v.z)
			};
		}

		inline b3Quat vectorToB3Quat(const Vector& v)
		{
			return b3Quat
			{
				.v = vectorToB3(v),
				.s = static_cast<decltype(b3Quat{}.s) > (v.w)
			};
		}
	};
};