#include "core/draw/planets/LODSphere.h"

#include <cmath>
#include <cstdint>

namespace EngineCore::Planets
{
	std::array<float, 3> getPatchColor(uint32_t i)
	{
		static const std::array<std::array<float, 3>, 7> colors = { {{0.05,0.05,0.5}, {0.1,0.9,0.8}, {0.1,0.1,0.4}, {0.0,0.3,0.2}, {0.7,0.2,0.0}, {0.3,0.1,0.3}, {0.95, 0.75, 0.20}} };
		return colors[i % colors.size()];
	}

	// offset: 2D local face offset, e.g. {-0.5f, 0.5f}
	// scale: 2D local face size extent for this node (Root is 2.0f, LOD 1 is 1.0f, etc.)
	LargeGeometry generateSubFace(uint32_t faceIndex, uint32_t resolution, double radius, Vec264 offset, double scale, uint32_t lodLevel, bool isRootFace)
	{
		static uint32_t debugColorIdx = 0;
		debugColorIdx++;

		LargeGeometry mesh = {};
		// an nxn grid of quads requires (n+1) x (n+1) vertices.
		mesh.vertices.reserve((resolution + 1) * (resolution + 1));
		// each quad consists of 2 triangles = 6 indices. nxn quads * 6 = total indices
		mesh.indices.reserve(resolution * resolution * 6);

		// CUBE FACE BASIS VECTOR DEFINITIONS
		// orthonormal 3d axes defining the 2d plane coordinate system for each of the 6 cube faces
		// face layout: +x, -x, +y, -y, +z, -z
		static const std::array<Vec64, 6> rights = { {{0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0}} };
		static const std::array<Vec64, 6> ups = { {{0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0}} };
		static const std::array<Vec64, 6> forwards = { {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}} };

		// select the basis vectors corresponding to the specified faceIndex
		const auto& r = rights[faceIndex]; // local 2D x-axis on cube face
		const auto& u = ups[faceIndex]; // local 2D y-axis on cube face
		const auto& f = forwards[faceIndex]; // cube face normal vector (distance = 1 from origin)

		constexpr double PI_OVER_4 = 0.78539816339f; // constant PI / 4 used for equiangular warping: tan(PI/4) = 1, tan(-PI/4) = -1

		// VERTEX GENERATION LOOP
		for (uint32_t y = 0; y <= resolution; ++y)
		{
			// convert y loop index to local normalized coordinate [0.0, 1.0]
			double local_y = static_cast<double>(y) / resolution;

			// apply scale and quadtree offset to compute coordinate in range [-1.0, 1.0]
			double my = offset.x + local_y * scale;

			// apply equiangular (tangent) distortion mapping to y axis
			// this converts uniform linear spacing into uniform angular spacing on the sphere,
			// preventing area compression/distortion at cube corners
			double tan_y = std::tan(my * PI_OVER_4);

			for (uint32_t x = 0; x <= resolution; ++x)
			{
				// convert x loop index to local normalized coordinate [0.0, 1.0]
				double local_x = static_cast<double>(x) / resolution;

				// !! TODO: ASAP: looks like x and y  are flipped here:

				// (map into local face space [-1, 1])
				// map to cube face range [-1.0, 1.0] using offset and scale
				double mx = offset.y + local_x * scale;

				// apply equiangular distortion mapping to x-axis
				double tan_x = std::tan(mx * PI_OVER_4);

				// construct 3D point (cx, cy, cz) on the surface of the unit cube:
				// center_point + (right_vector * tan_x) + (up_vector * tan_y)
				double cx = f.x + r.x * tan_x + u.x * tan_y;
				double cy = f.y + r.y * tan_x + u.y * tan_y;
				double cz = f.z + r.z * tan_x + u.z * tan_y;

				
				// projects point onto unit sphere by using inverse magnitude to normalize the vector
				double inv_len = 1.0f / std::sqrt(cx * cx + cy * cy + cz * cz);
				// unit vector pointing outwards from sphere center (also doubles as surface normal)
				Vec64 n = { cx * inv_len, cy * inv_len, cz * inv_len };
				double nx = cx * inv_len;
				double ny = cy * inv_len;
				double nz = cz * inv_len;

				LargeVertex v;
				// scale unit vector by radius to place vertex at actual world-space sphere radius
				v.position = n * radius;
				v.normal = { static_cast<float>(n.x), static_cast<float>(n.y), static_cast<float>(n.z) };

				auto col = getPatchColor(debugColorIdx);
				v.color = { col[0], col[1], col[2] };

				mesh.vertices.push_back(v);
			}
		}

		// INDEX GENERATION LOOP (TRIANGULATION)
		// distance in vertex array between adjacent vertical rows — indices are relative to this patch's local mesh
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

	// uses the same math as the face generator to map a 2D local coordinate back to a 3D point on the unit cube and project it to the sphere radius
	Vec64 projectToSphere(uint32_t faceIndex, Vec264 localCenter2D, double radius)
	{
		static const std::array<std::array<float, 3>, 6> rights = { {{0,0,1}, {0,0,-1}, {1,0,0}, {1,0,0}, {1,0,0}, {-1,0,0}} };
		static const std::array<std::array<float, 3>, 6> ups = { {{0,1,0}, {0,1,0}, {0,0,1}, {0,0,-1}, {0,1,0}, {0,1,0}} };
		static const std::array<std::array<float, 3>, 6> forwards = { {{1,0,0}, {-1,0,0}, {0,1,0}, {0,-1,0}, {0,0,1}, {0,0,-1}} };

		const auto& r = rights[faceIndex];
		const auto& u = ups[faceIndex];
		const auto& f = forwards[faceIndex];

		constexpr float PI_OVER_4 = 0.78539816339f;

		float mx = localCenter2D.x;
		float my = localCenter2D.y;
		float tan_x = std::tan(mx * PI_OVER_4);
		float tan_y = std::tan(my * PI_OVER_4);

		// point on local unit cube
		float cx = f[0] + r[0] * tan_x + u[0] * tan_y;
		float cy = f[1] + r[1] * tan_x + u[1] * tan_y;
		float cz = f[2] + r[2] * tan_x + u[2] * tan_y;

		// project to sphere surface
		float inv_len = 1.0f / std::sqrt(cx * cx + cy * cy + cz * cz);
		float nx = cx * inv_len;
		float ny = cy * inv_len;
		float nz = cz * inv_len;

		return Vec64{ nx * radius, ny * radius, nz * radius };
	}

	EngineCore::MeshBuilder LargeGeometry::toSinglePrecision() const
	{
		EngineCore::MeshBuilder m = {};
		m.vertices.reserve(vertices.size());
		m.indices.reserve(indices.size());
		for (const auto& v : vertices)
		{
			Vertex vert{};
			vert.position.x = static_cast<float>(v.position.x);
			vert.position.y = static_cast<float>(v.position.y);
			vert.position.z = static_cast<float>(v.position.z);
			vert.normal.x = v.normal.x;
			vert.normal.y = v.normal.y;
			vert.normal.z = v.normal.z;
			vert.color.x = v.color.x;
			vert.color.y = v.color.y;
			vert.color.z = v.color.z;
			vert.uv = { 0, 0 };
			m.vertices.push_back(vert);
		}
		for (const uint32_t& i : indices)
		{
			m.indices.push_back(i);
		}

		return m;
	}
}