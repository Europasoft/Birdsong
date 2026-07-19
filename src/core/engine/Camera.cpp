#include "core/engine/Camera.h"
#include "core/include/shared/Transform.h"

#include <glm/gtc/constants.hpp>
#include <iostream>
#include <algorithm>

namespace EngineCore
{
	Camera::Camera(float fieldOfViewDeg, float nearDistance, float farDistance)
	{
		near = nearDistance;
		far = farDistance;
		setFieldOfView(fieldOfViewDeg);
		createConversionMatrices();
	}

	void Camera::createConversionMatrices()
	{
		auto& m1 = blenderToVulkanMatrix1;
		auto& m2 = blenderToVulkanMatrix2;
		m1 = m2 = glm::mat4(0.f); // zero-initialize

		// to swap x and z
		// a,b,c -> c,b,a
		// 0, 0, 1, 0
		// 0, 1, 0, 0
		// 1, 0, 0, 0
		// 0, 0, 0, 1
		m1[2][0] = 1.f;
		m1[1][1] = 1.f;
		m1[0][2] = 1.f;
		m1[3][3] = 1.f;

		// to swap and invert y and z
		// c,b,a -> c,-a,-b
		// 1, 0, 0, 0
		// 0, 0,-1, 0
		// 0,-1, 0, 0
		// 0, 0, 0, 1
		m2[0][0] = 1.f;
		m2[2][1] = -1.f;
		m2[1][2] = -1.f;
		m2[3][3] = 1.f;
	}

	glm::mat4 Camera::getProjectionMatrix()
	{
		glm::mat4 m{ 0.f };

		// old method, stretches the image with the viewport
		// m[0][0] = aspectRatio / tan(fov / 2);
		// m[1][1] = 1 / tan(fov / 2);
		// https://www.gamedev.net/forums/topic/695655-trouble-with-my-perspective-projection-matrix/5373548/

		m[0][0] = 1.f / (fov / 2.f);
		m[1][1] = 1.f / ((fov / 2.f) / aspectRatio);

		m[2][2] = far / (far - near);
		m[3][2] = -((near * far) / (far - near));
		m[2][3] = 1.f;

		return m;
	}

	glm::mat4 Camera::getViewMatrix() const
	{
		return glm::inverse(Transform::makeMatrixZYX(transform.rotation, transform.scale, transform.translation));
	}

	glm::mat4 Camera::getProjectionViewMatrix(bool inverse)
	{
		glm::mat4 projectionMatrix = getProjectionMatrix();
		if (!inverse)
		{
			return projectionMatrix * blenderToVulkanMatrix1 * blenderToVulkanMatrix2 * getViewMatrix();
		}
		else
		{
			return glm::inverse(getViewMatrix()) * glm::inverse(blenderToVulkanMatrix1) * glm::inverse(blenderToVulkanMatrix2) * glm::inverse(projectionMatrix);
		}
	}

	void Camera::moveInPlaneXY(const Vector2D<double>& lookInput, const float& moveFwd, const float& moveRight, 
								const float& moveUp, const bool& extraSpeed, const float& deltaTime)
	{
		float lookSpeed = 6.8f;
		float moveSpeed = 30.f;
		//if (extraSpeed) { moveSpeed *= 80.f; }
		if (extraSpeed) { moveSpeed *= 100.f; }

		transform.rotation.y += (float)lookInput.y * lookSpeed * deltaTime;      // pitch
		transform.rotation.z += (float)(-lookInput.x) * lookSpeed * deltaTime;   // yaw

		// limit pitch values to exactly 85 degrees
		transform.rotation.y = glm::clamp(transform.rotation.y, 
				(float)Transform::degToRad(-85.f), (float)Transform::degToRad(85.f));
		// prevent overflow from continous yawing
		transform.rotation.z = glm::mod(transform.rotation.z, glm::two_pi<float>());

		glm::vec4 fwd4 = Transform::makeMatrixZYX(transform.rotation, transform.scale, transform.translation) * glm::vec4{ 1.f, 0.f, 0.f, 0.f };
		const Vec forwardDir = Vec{ fwd4.x, fwd4.y, fwd4.z };
		const Vec rightDir = Vec{ forwardDir.y, -forwardDir.x, 0.f };
		const Vec upDir{ 0.f, 0.f, 1.f };

		Vec moveDir{ 0.f };
		if (moveFwd > 0.f) { moveDir += forwardDir; }
		else if (moveFwd < 0.f) { moveDir -= forwardDir; }

		if (moveRight > 0.f) { moveDir += rightDir; }
		else if (moveRight < 0.f) { moveDir -= rightDir; }

		if (moveUp > 0.f) { moveDir += upDir; }
		else if (moveUp < 0.f) { moveDir -= upDir; }

		transform.translation += moveDir * moveSpeed * deltaTime;
		//std::cout << "CAM: " << (Vec::distance(Vec(), transform.translation) / 100) / 1000 << "km\n";
		//std::cout << "\n" << "x:" << transform.translation.x << "y:" << transform.translation.y << "z:" << transform.translation.z;
	}

	void Camera::moveInPlaneXYN(const Vector2D<double>& lookInput, const float& moveFwd, const float& moveRight, 
								const float& moveUp, const bool& extraSpeed, const float& deltaTime)
	{
		//(GLFWwindow * window, float dt, LveGameObject & gameObject)
		float lookSpeed = 6.f;
		glm::vec3 rotate{ 0.f, lookInput.y, lookInput.x };

		if (glm::dot(rotate, rotate) > std::numeric_limits<float>::epsilon())
		{
			//transform.rotation += lookSpeed * deltaTime * glm::normalize(rotate);
		}

		// limit pitch values between about +/- 85ish degrees
		//transform.rotation.y = glm::clamp(transform.rotation.x, -1.5f, 1.5f);
		//transform.rotation.z = glm::mod(transform.rotation.y, glm::two_pi<float>());

		float yaw = transform.rotation.y;
		const glm::vec3 forwardDir{ sin(yaw), 0.f, cos(yaw) };
		const glm::vec3 rightDir{ forwardDir.z, 0.f, -forwardDir.x };
		const glm::vec3 upDir{ 0.f, -1.f, 0.f };

		//glm::vec3 moveDir{ 0.f };
		//if (glfwGetKey(window, keys.moveForward) == GLFW_PRESS) moveDir += forwardDir;
		//if (glfwGetKey(window, keys.moveBackward) == GLFW_PRESS) moveDir -= forwardDir;
		//if (glfwGetKey(window, keys.moveRight) == GLFW_PRESS) moveDir += rightDir;
		//if (glfwGetKey(window, keys.moveLeft) == GLFW_PRESS) moveDir -= rightDir;
		//if (glfwGetKey(window, keys.moveUp) == GLFW_PRESS) moveDir += upDir;
		//if (glfwGetKey(window, keys.moveDown) == GLFW_PRESS) moveDir -= upDir;
		//
		//if (glm::dot(moveDir, moveDir) > std::numeric_limits<float>::epsilon()) 
		//{
		//	gameObject.transform.translation += moveSpeed * dt * glm::normalize(moveDir);
		//}
	}




}

