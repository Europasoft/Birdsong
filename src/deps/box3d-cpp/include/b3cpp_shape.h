// Copyright 2026 Simon Liimatainen. All rights reserved.
// do not include Box3D headers here, to avoid leaking C types into the global scope where this file is included

#pragma once
#include "include/b3cpp_base.h"
#include "include/b3cpp_types.h"
#include <cstdint>
#include <vector>

namespace b3cpp
{
	class Body;
	class World;

	// The shape goes into a shape press that presses the shape into a pressed shape
	class Shape : public Wrapper
	{
	public:
		virtual ~Shape() {};
		virtual void activate(ShapeDef shapeDef) = 0;

	protected:
		// constructor is protected, Shapes are created only by a Body object
		friend class Body;
		Shape(Body& body) : body(body) {};
		void destroy();

		bool isIdValid() override;

		Body& body;
		void* data = nullptr;
	};

	class BoxHullShape : public Shape
	{
	public:
		float halfWidthX = 100.f;
		float halfWidthY = 100.f;
		float halfWidthZ = 100.f;
		virtual void activate(ShapeDef def = ShapeDef()) override;
	protected:
		friend class Body; // grants Body access to the constructor
		using Shape::Shape; // inherits the constructor from the base class
	};

	class SphereShape : public Shape
	{
	public:
		Vector localCenter;
		float radius = 100.f;
		virtual void activate(ShapeDef def = ShapeDef()) override;
	protected:
		friend class Body;
		using Shape::Shape;
	};

	class CapsuleShape : public Shape
	{
	public:
		Vector center1;
		Vector center2;
		float radius = 100.f;
		virtual void activate(ShapeDef def = ShapeDef()) override;
	protected:
		friend class Body;
		using Shape::Shape;
	};

	class HullShape : public Shape
	{
	public:
		std::vector<Vector> vertices;
		virtual void activate(ShapeDef def = ShapeDef()) override;
		~HullShape() override;
	protected:
		friend class Body;
		using Shape::Shape;
	};

	class HeightfieldShape : public Shape
	{
	public:
		virtual void activate(ShapeDef def = ShapeDef()) override;
		~HeightfieldShape() override;
	protected:
		friend class Body;
		using Shape::Shape;
	};

	class MeshShape : public Shape
	{
	public:
		virtual void activate(ShapeDef def = ShapeDef()) override;
		~MeshShape() override;
	protected:
		friend class Body;
		using Shape::Shape;
	};

}