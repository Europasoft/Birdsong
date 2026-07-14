#include "include/b3cpp_body.h"
#include "include/b3cpp_types.h"
#include "include/b3cpp_world.h"
#include "include/b3cpp_shape.h"

#include "private/b3cpp_utils.h"
#include "box3d/include/box3d/box3d.h" // Box3D types used internally

#include <cstdint>
#include <cstring> // for std::memcpy

namespace b3cpp
{
	Body::Body(World& world, BodyDef def)
		: world(world)
	{
		b3BodyDef groundBodyDef = b3DefaultBodyDef();
		def.type.applyIfNotNull(groundBodyDef.type, static_cast<b3BodyType>(def.type.val()));
		def.position.applyIfNotNull(groundBodyDef.position, util::vectorToB3Pos(def.position.val()));
		def.rotation.applyIfNotNull(groundBodyDef.rotation, util::vectorToB3Quat(def.position.val()));
		def.linearVelocity.applyIfNotNull(groundBodyDef.linearVelocity, util::vectorToB3(def.position.val()));
		def.angularVelocity.applyIfNotNull(groundBodyDef.angularVelocity, util::vectorToB3(def.position.val()));
		def.linearDamping.applyIfNotNull(groundBodyDef.linearDamping);
		def.angularDamping.applyIfNotNull(groundBodyDef.angularDamping);
		def.gravityScale.applyIfNotNull(groundBodyDef.gravityScale);
		def.sleepThreshold.applyIfNotNull(groundBodyDef.sleepThreshold);
		def.enableSleep.applyIfNotNull(groundBodyDef.enableSleep);
		def.isAwake.applyIfNotNull(groundBodyDef.isAwake);
		def.isBullet.applyIfNotNull(groundBodyDef.isBullet);
		def.isEnabled.applyIfNotNull(groundBodyDef.isEnabled);
		def.allowFastRotation.applyIfNotNull(groundBodyDef.allowFastRotation);
		def.enableContactRecycling.applyIfNotNull(groundBodyDef.enableContactRecycling);
		b3BodyId id = b3CreateBody(util::getId(&world), &groundBodyDef);
		util::setId(this, id);
	}

    Body::~Body()
    {
		// no need to manually destroy shapes here
		if (isIdValid()) b3DestroyBody(util::getId(this));
	}

	bool Body::isIdValid()
	{
		return b3Body_IsValid(util::getId(this));
	}
}