#include "include/b3cpp_world.h"
#include "include/b3cpp_types.h"

#include "private/b3cpp_utils.h"
#include "box3d/include/box3d/box3d.h" // Box3D types used internally

#include <cstdint>

namespace b3cpp
{
	World::World(WorldDef def)
	{
		b3WorldDef worldDef = b3DefaultWorldDef();
		def.gravity.applyIfNotNull(worldDef.gravity, util::vectorToB3(def.gravity.val()));
		def.restitutionThreshold.applyIfNotNull(worldDef.restitutionThreshold);
		def.hitEventThreshold.applyIfNotNull(worldDef.hitEventThreshold);
		def.contactHertz.applyIfNotNull(worldDef.contactHertz);
		def.contactDampingRatio.applyIfNotNull(worldDef.contactDampingRatio);
		def.contactSpeed.applyIfNotNull(worldDef.contactSpeed);
		def.maximumLinearSpeed.applyIfNotNull(worldDef.maximumLinearSpeed);
		def.enableSleep.applyIfNotNull(worldDef.enableSleep);
		def.enableContinuous.applyIfNotNull(worldDef.enableContinuous);
		b3WorldId id = b3CreateWorld(&worldDef);
		util::setId(this, id);
	}

	World::~World()
	{
		b3DestroyWorld(util::getId(this));
	}

	Body World::createBody(BodyDef def)
	{
		return Body(*this, def);
	}

	bool World::isIdValid()
	{
		return b3World_IsValid(util::getId(this));
	}
}