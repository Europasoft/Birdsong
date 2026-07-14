// Copyright 2026 Simon Liimatainen. All rights reserved.
// do not include Box3D headers here, to avoid leaking C types into the global scope where this file is included

#pragma once
#include "include/b3cpp_base.h"
#include "include/b3cpp_types.h"
#include "include/b3cpp_shape.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <concepts>

namespace b3cpp
{
	class World;

	class Body : public Wrapper
	{
	public:
		Body(World& world, BodyDef def = BodyDef());
		~Body();

		/*
		* Example:
		* Body b = physWorld.createBody();
		* SphereShape& shape = b.createShape<SphereShape>();
		* shape.radius = 1337.f;
		* shape.activate(shapeDef);
		*/
		template <typename T>
		requires std::derived_from<T, Shape>
		T& createShape()
		{
			shapes.push_back(std::unique_ptr<T>(new T(*this)));
			return *static_cast<T*>(shapes.back().get());
		}

	protected:
		bool isIdValid() override;

		World& world;
		std::vector<std::unique_ptr<Shape>> shapes;
	};
}