#include "include/b3cpp_shape.h"
#include "include/b3cpp_world.h"
#include "include/b3cpp_types.h"
#include "include/b3cpp_body.h"

#include "private/b3cpp_utils.h"
#include "box3d/include/box3d/box3d.h" // Box3D types used internally

#include <cstdint>
#include <cassert>

namespace 
{
	b3ShapeDef getShapeDef(b3cpp::ShapeDef def)
	{
		b3ShapeDef shapeDef = b3DefaultShapeDef();
		// TODO: apply values
		return shapeDef;
	}
}

namespace b3cpp
{
	void Shape::destroy()
	{
		if (isIdValid()) b3DestroyShape(util::getId(this), true);
	}

	void BoxHullShape::activate(ShapeDef def)
	{
		b3BoxHull hull = b3MakeBoxHull(halfWidthX, halfWidthY, halfWidthZ);
		b3ShapeDef d = getShapeDef(def);
		util::setId(this, b3CreateHullShape(b3cpp::util::getId(&body), &d, &hull.base));
	}

	void SphereShape::activate(ShapeDef def)
	{
		b3Sphere sphere = { .center = util::vectorToB3(localCenter), .radius = radius };
		b3ShapeDef d = getShapeDef(def);
		util::setId(this, b3CreateSphereShape(b3cpp::util::getId(&body), &d, &sphere));
	}

	void CapsuleShape::activate(ShapeDef def)
	{
		b3Capsule capsule = { .center1 = util::vectorToB3(center1), .center2 = util::vectorToB3(center2), .radius = radius };
		b3ShapeDef d = getShapeDef(def);
		util::setId(this, b3CreateCapsuleShape(b3cpp::util::getId(&body), &d, &capsule));
	}

	void HullShape::activate(ShapeDef def)
	{
		std::vector<b3Vec3> points;
		for (const auto& v : vertices) { points.push_back(util::vectorToB3(v)); }
		b3HullData* hullData = b3CreateHull(points.data(), static_cast<int>(points.size()), 255);
		data = hullData; // save address of allocated data so it can be freed when the shape is disposed
		b3ShapeDef d = getShapeDef(def);
		util::setId(this, b3CreateHullShape(b3cpp::util::getId(&body), &d, hullData));
	}

	HullShape::~HullShape()
	{
		// this presumably only happens when the owning Body is removed, so no need to call destroy()
		if (data) b3DestroyHull(reinterpret_cast<b3HullData*>(data));
	}

	void HeightfieldShape::activate(ShapeDef def)
	{
		assert(0 && "TODO: shape not implemented");
	}

	HeightfieldShape::~HeightfieldShape()
	{
		// TODO
	}

	void MeshShape::activate(ShapeDef def)
	{
		assert(0 && "TODO: shape not implemented");
	}

	MeshShape::~MeshShape()
	{
		// TODO
	}

	bool Shape::isIdValid()
	{
		return b3Shape_IsValid(util::getId(this));
	}
}