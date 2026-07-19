#pragma once

#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"
#include "core/types/glm_conversions.h"

namespace EngineCore
{ 
	/* a virtual camera, this is you */
	class Camera
	{
	public:
		Camera() = default;
		Camera(float fieldOfViewDeg, float nearDistance, float farDistance);

		Camera& operator=(const Camera&) = default;
		Camera& operator=(Camera&&) = default;

		Transform transform;
		glm::mat4 blenderToVulkanMatrix1, blenderToVulkanMatrix2;
		float fov = 1.3f; // radians
		float near = 2.f;
		float far = 6000.f;
		float aspectRatio = 1.777f;

		float testValue = 1.f; // debug test value, set by user input

		void createConversionMatrices();

		void setFieldOfView(float deg) { fov = (float)cglm::degToRad(deg); }
		void setAspectRatio(float a) { aspectRatio = a; }

		bool flip = false;
		glm::mat4 getProjectionMatrix();

		glm::mat4 getViewMatrix() const;

		glm::mat4 getProjectionViewMatrix(bool inverse = false);

		void moveInPlaneXY(const Vector2D<double>& lookInput, const float& moveFwd, const float& moveRight,
							const float& moveUp, const bool& extraSpeed, const float& deltaTime);

		void moveInPlaneXYN(const Vector2D<double>& lookInput, const float& moveFwd, const float& moveRight,
							const float& moveUp, const bool& extraSpeed, const float& deltaTime);
	};

}