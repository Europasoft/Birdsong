#include "core/draw/planets/LODSphere.h"

namespace EngineCore::Planets
{
	MeshBuilder generateCubeFace(int face_index, int resolution, float radius)
	{
		MeshBuilder mesh = {};
		mesh.vertices.reserve((resolution + 1) * (resolution + 1));
		mesh.indices.reserve(resolution * resolution * 6);

		// Face orientation basis vectors (Right, Up, Forward)
		static const std::array<std::array<float, 3>, 6> rights = { {{0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0}} };
		static const std::array<std::array<float, 3>, 6> ups = { {{0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0}} };
		static const std::array<std::array<float, 3>, 6> forwards = { {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}} };

		const auto& r = rights[face_index];
		const auto& u = ups[face_index];
		const auto& f = forwards[face_index];

		constexpr float PI_OVER_4 = 0.78539816339f;

		// Generate Vertices
		for (int y = 0; y <= resolution; ++y)
		{
			float my = (static_cast<float>(y) / resolution) * 2.0f - 1.0f; // [-1, 1]
			float tan_y = std::tan(my * PI_OVER_4);

			for (int x = 0; x <= resolution; ++x)
			{
				float mx = (static_cast<float>(x) / resolution) * 2.0f - 1.0f; // [-1, 1]
				float tan_x = std::tan(mx * PI_OVER_4);

				// Point on local unit cube
				float cx = f[0] + r[0] * tan_x + u[0] * tan_y;
				float cy = f[1] + r[1] * tan_x + u[1] * tan_y;
				float cz = f[2] + r[2] * tan_x + u[2] * tan_y;

				// Project to sphere surface
				float inv_len = 1.0f / std::sqrt(cx * cx + cy * cy + cz * cz);
				float nx = cx * inv_len;
				float ny = cy * inv_len;
				float nz = cz * inv_len;

				Vertex v;
				v.position[0] = nx * radius;
				v.position[1] = ny * radius;
				v.position[2] = nz * radius;

				v.normal[0] = nx;
				v.normal[1] = ny;
				v.normal[2] = nz;

				static const std::array<std::array<float, 3>, 6> colors = { {{0.1,0.3,0.2}, {0.0,0.2,0.4}, {0.3,0.1,0.3}, {0.0,0.6,0.2}, {0.7,0.5,0.0}, {0.1,0.9,0.8}} };
				v.color = { colors[face_index][0], colors[face_index][1], colors[face_index][2] };

				mesh.vertices.push_back(v);
			}
		}

		// Generate Indices
		int stride = resolution + 1;
		for (int y = 0; y < resolution; ++y)
		{
			for (int x = 0; x < resolution; ++x)
			{
				uint32_t i0 = x + y * stride;
				uint32_t i1 = (x + 1) + y * stride;
				uint32_t i2 = x + (y + 1) * stride;
				uint32_t i3 = (x + 1) + (y + 1) * stride;

				// Triangle 1
				mesh.indices.push_back(i0);
				mesh.indices.push_back(i2);
				mesh.indices.push_back(i1);

				// Triangle 2
				mesh.indices.push_back(i1);
				mesh.indices.push_back(i2);
				mesh.indices.push_back(i3);
			}
		}

		return mesh;
	}

	// offset: 2D local face offset, e.g. {-0.5f, 0.5f}
	// scale: 2D local face size extent for this node (Root is 2.0f, LOD 1 is 1.0f, etc.)
	MeshBuilder generateSubFace(int face_index, int resolution, float radius,
					std::array<float, 2> offset, float scale)
	{
		MeshBuilder mesh = {};
		mesh.vertices.reserve((resolution + 1) * (resolution + 1));
		mesh.indices.reserve(resolution * resolution * 6);

		static const std::array<std::array<float, 3>, 6> rights = { {{0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0}} };
		static const std::array<std::array<float, 3>, 6> ups = { {{0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0}} };
		static const std::array<std::array<float, 3>, 6> forwards = { {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}} };

		const auto& r = rights[face_index];
		const auto& u = ups[face_index];
		const auto& f = forwards[face_index];

		constexpr float PI_OVER_4 = 0.78539816339f;

		for (int y = 0; y <= resolution; ++y)
		{
			// 1. Convert grid index to normalized node space [0, 1]
			float local_y = static_cast<float>(y) / resolution;

			// 2. Map into local face space [-1, 1] using offset and scale
			float my = offset[1] + local_y * scale;
			float tan_y = std::tan(my * PI_OVER_4);

			for (int x = 0; x <= resolution; ++x)
			{
				float local_x = static_cast<float>(x) / resolution;

				// Map into local face space [-1, 1]
				float mx = offset[0] + local_x * scale;
				float tan_x = std::tan(mx * PI_OVER_4);

				// Point on local unit cube
				float cx = f[0] + r[0] * tan_x + u[0] * tan_y;
				float cy = f[1] + r[1] * tan_x + u[1] * tan_y;
				float cz = f[2] + r[2] * tan_x + u[2] * tan_y;

				// Normalize and project to sphere surface
				float inv_len = 1.0f / std::sqrt(cx * cx + cy * cy + cz * cz);
				float nx = cx * inv_len;
				float ny = cy * inv_len;
				float nz = cz * inv_len;

				Vertex v;
				v.position = { nx * radius, ny * radius, nz * radius };
				v.normal = { nx, ny, nz };
				v.color = { nx, ny, nz };

				mesh.vertices.push_back(v);
			}
		}

		// Index generation stays identical — indices are relative to this patch's local mesh
		int stride = resolution + 1;
		for (int y = 0; y < resolution; ++y)
		{
			for (int x = 0; x < resolution; ++x)
			{
				uint32_t i0 = x + y * stride;
				uint32_t i1 = (x + 1) + y * stride;
				uint32_t i2 = x + (y + 1) * stride;
				uint32_t i3 = (x + 1) + (y + 1) * stride;

				mesh.indices.push_back(i0);
				mesh.indices.push_back(i2);
				mesh.indices.push_back(i1);

				mesh.indices.push_back(i1);
				mesh.indices.push_back(i2);
				mesh.indices.push_back(i3);
			}
		}

		return mesh;
	}
}