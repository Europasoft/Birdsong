#pragma once
#include <glm/glm.hpp>
#include "core/types/CommonTypes.h"
#include "core/include/shared/Transform.h"

#ifdef GLM_VERSION
namespace EngineCore
{
	class cglm
	{
	public:
		static glm::vec3 vectorToGLM(const Vec& v);
		static Vec vectorFromGLM(const glm::vec3& v);

		static glm::mat4 makeMatrixDef(const Vec& rotationIn, const Vec& scaleIn,
			const Vec& translationIn = { 0.f, 0.f, 0.f });

		static glm::mat4 makeMatrix(const Vec& rotationIn, const Vec& scaleIn,
			const Vec& translationIn = { 0.f, 0.f, 0.f });

		static glm::mat4 transformToGLMmat4(const Transform& t);

		static glm::mat4 makeMatrixZYX(const Vec& rotationIn, const Vec& scaleIn,
			const Vec& translationIn = { 0.f, 0.f, 0.f });

		static glm::mat4 makeMatrixLve(const Vec& rotationIn, const Vec& scaleIn,
			const Vec& translationIn = { 0.f, 0.f, 0.f });

		static glm::mat4 makeMatrix(const Vec& rotationIn);

		static Vec rotateVector(const Vec& v3, const Vec& rot);

		// replacement for missing overload of cross() that accepts 4-component vectors (w=0)
		static glm::vec4 crossVec4(const glm::vec4& a, const glm::vec4& b);

		static Vec rotateVectorQuaternion(const Vec& v3, const glm::vec4& rot);

		static Vec getForwardVector(const Transform& t);

		static double degToRad(const double& degrees);

		// assumes yaw = z, pitch = y, roll = x
		static glm::vec4 quaternionFromRotation(const Vec& v);
	};

}
#endif