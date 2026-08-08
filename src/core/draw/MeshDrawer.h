#pragma once
#include "core/draw/DrawBase.h"
#include "core/gpu/Material.h"

#include <glm/gtc/matrix_transform.hpp> // glm
#include <memory>
#include <vector>
#include <cmath> // only used in perspective calculation

class Camera;
struct Transform;

namespace WorldSystem 
{ 
	class World;
	class Mesh;
}

namespace EngineCore
{
	class MeshDrawer : public DrawBase
	{
	public:
		MeshDrawer(EngineDevice& device, const DrawContext& d);
		~MeshDrawer();

		virtual void render(const FrameContext& f) override;

	private:
		struct DrawMeshContext;
		void renderOne(DrawMeshContext& ctx);

		static glm::mat4 orthographicMatrix(const float& n, const float& f)
		{
			float r = 1.f;
			float l = -1.f;
			float t = -1.f;
			float b = 1.f;

			float x1 = 2 / (r - l);
			float x2 = -(r + l) / (r - l);
			float x3 = 2 / (b - t); /* swap b and t for y axis up (default is y = down, b-t) */
			float x4 = -(b + t) / (b - t);
			float x5 = 1 / (f - n);
			float x6 = -n / (f - n);

			glm::mat4 projection 
			{
				x1,		0.f,	0.f,	x2,
				0.f,	x3,		0.f,	x4,
				0.f,	0.f,	x5,		x6,
				0.f,	0.f,	0.f,	1.f, 
			};

			return projection;
		}

		static glm::mat4 lerpMat4(float t, glm::mat4 matA, glm::mat4 matB);

		static float lerp(const double& a, const double& b, const double& t)
		{
			return static_cast<float>((a * (1.0 - t)) + (b * t));
		}
	};

}