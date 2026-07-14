// Copyright 2026 Simon Liimatainen. All rights reserved.
// do not include Box3D headers here, to avoid leaking C types into the global scope where this file is included

#pragma once
#include <cstdint>

namespace b3cpp
{
	template<typename T>
	class NullableValue
	{
	public:
		NullableValue() : value(T{}), nullFlag(true) {};
		NullableValue(const T& val) : value(val), nullFlag(false) {}
		NullableValue& operator=(const T& val)
		{
			value = val;
			nullFlag = false;
			return *this;
		}
		const T& val() const { return value; }
		bool isNull() const { return nullFlag; }
		void applyIfNotNull(T& target) const { if (!isNull()) target = value; }
		template<typename X>
		void applyIfNotNull(X& target, const X& v) const { if (!isNull()) target = v; }

	protected:
		T value;
		bool nullFlag = true;
	};

	class Vector
	{
	public:
		double x, y, z, w;
		Vector() : x(0), y(0), z(0), w(0) {};
	};

	struct WorldDef
	{
		// Gravity vector. Box3D has no up-vector defined.
		NullableValue<Vector> gravity;

		// Restitution speed threshold, usually in m/s. Collisions above this
		// speed have restitution applied (will bounce).
		NullableValue<float> restitutionThreshold;

		// Hit event speed threshold, usually in m/s. Collisions above this
		// speed can generate hit events if the shape also enables hit events.
		NullableValue<float> hitEventThreshold;

		// Contact stiffness. Cycles per second. Increasing this increases the speed of overlap recovery, but can introduce jitter.
		NullableValue<float> contactHertz;

		// Contact bounciness. Non-dimensional. You can speed up overlap recovery by decreasing this with
		// the trade-off that overlap resolution becomes more energetic.
		NullableValue<float> contactDampingRatio;

		// This parameter controls how fast overlap is resolved and usually has units of meters per second. This only
		// puts a cap on the resolution speed. The resolution speed is increased by increasing the hertz and/or
		// decreasing the damping ratio.
		NullableValue<float> contactSpeed;

		// Maximum linear speed. Usually meters per second.
		NullableValue<float> maximumLinearSpeed;

		// Can bodies go to sleep to improve performance
		NullableValue<bool> enableSleep;

		// Enable continuous collision
		NullableValue<bool> enableContinuous;
	};

	enum class EBodyType : int
	{
		/// zero mass, zero velocity, may be manually moved
		StaticBody = 0,

		/// zero mass, velocity set by user, moved by solver
		KinematicBody = 1,

		/// positive mass, velocity determined by forces, moved by solver
		DynamicBody = 2,
	};

	struct BodyDef
	{
		/// The body type: static, kinematic, or dynamic.
		NullableValue<EBodyType> type;

		/// The initial world position of the body. Bodies should be created with the desired position.
		/// @note Creating bodies at the origin and then moving them nearly doubles the cost of body creation, especially
		/// if the body is moved after shapes have been added.
		NullableValue<Vector> position;

		/// The initial world rotation of the body.
		NullableValue<Vector> rotation;

		/// The initial linear velocity of the body's origin. Usually in meters per second.
		NullableValue<Vector> linearVelocity;

		/// The initial angular velocity of the body. Radians per second.
		NullableValue<Vector> angularVelocity;

		/// Linear damping is used to reduce the linear velocity. The damping parameter
		/// can be larger than 1 but the damping effect becomes sensitive to the
		/// time step when the damping parameter is large.
		/// Generally linear damping is undesirable because it makes objects move slowly
		/// as if they are floating.
		NullableValue<float> linearDamping;

		/// Angular damping is used to reduce the angular velocity. The damping parameter
		/// can be larger than 1.0f but the damping effect becomes sensitive to the
		/// time step when the damping parameter is large.
		/// Angular damping can be used to slow down rotating bodies.
		NullableValue<float> angularDamping;

		/// Scale the gravity applied to this body. Non-dimensional.
		NullableValue<float> gravityScale;

		/// Sleep speed threshold, default is 0.05 meters per second
		NullableValue<float> sleepThreshold;

		/// Motions locks to restrict linear and angular movement
		//b3MotionLocks motionLocks;

		/// Set this flag to false if this body should never fall asleep.
		NullableValue<bool> enableSleep;

		/// Is this body initially awake or sleeping?
		NullableValue<bool> isAwake;

		/// Treat this body as a high speed object that performs continuous collision detection
		/// against dynamic and kinematic bodies, but not other bullet bodies.
		/// @warning Bullets should be used sparingly. They are not a solution for general dynamic-versus-dynamic
		/// continuous collision. They do not guarantee accurate collision if both bodies are fast moving because
		/// the bullet does a continuous check after all non-bullet bodies have moved. You could get unlucky and have
		/// the bullet body end a time step very close to a non-bullet body and the non-bullet body then moves over
		/// the bullet body. In continuous collision, initial overlap is ignored to avoid freezing bodies in place.
		/// I do not recommend using them for game projectiles if precise collision timing is needed. Instead consider
		/// using a ray or shape cast. You can use a marching ray or shape cast for projectile that moves over time.
		/// If you want a fast moving projectile to collide with a fast moving target, you need to consider the relative
		/// movement in your ray or shape cast. This is out of the scope of Box3D.
		/// So what are good use cases for bullets? Pinball games or games with dynamic containers that hold other objects.
		/// It should be a use case where it doesn't break the game if there is a collision missed, but having them
		/// captured improves the quality of the game.
		NullableValue<bool> isBullet;

		/// Used to disable a body. A disabled body does not move or collide.
		NullableValue<bool> isEnabled;

		/// This allows this body to bypass rotational speed limits. Should only be used
		/// for circular objects, like wheels.
		NullableValue<bool> allowFastRotation;

		/// Enable contact recycling. True by default. Leaving this enabled improves performance
		/// but may lead to ghost collision that should be avoided on characters.
		NullableValue<bool> enableContactRecycling;
	};

	struct ShapeDef // TODO
	{};

}