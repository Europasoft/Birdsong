#include "core/engine/MeshData.h"

#define TINYOBJLOADER_IMPLEMENTATION // mesh file loader
#include "thirdparty/tiny_obj_loader.h"

#include <stdexcept>

namespace EngineCore
{
	std::vector<VkVertexInputBindingDescription> Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Vertex::getAttributeDescriptions()
	{
		return
		{
			// location, binding, format, offset
			{ 0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position) },
			{ 1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color) },
			{ 2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal) },
			{ 3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv) }
		};
	}

	void MeshBuilder::loadFromFile(const std::string& path)
	{
		// TODO: support different mesh formats
		// OBJ format mesh loader, using TinyObjLoader (for now)
		tinyobj::attrib_t attrib;
		std::vector<tinyobj::shape_t> shapes;
		std::vector<tinyobj::material_t> materials;
		std::string warn, err;
		if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path.c_str()))
		{
			throw std::runtime_error("error loading mesh from file: " + warn + err);
		}
		vertices.clear();
		indices.clear(); // indices = 0 for OBJ format to indicate non-indexed primitive
		for (const auto& shape : shapes)
		{
			for (const auto& index : shape.mesh.indices)
			{
				Vertex vert{};
				if (index.vertex_index >= 0)
				{
					vert.position = { attrib.vertices[3 * index.vertex_index],
									attrib.vertices[3 * index.vertex_index + 1],
									attrib.vertices[3 * index.vertex_index + 2] };
				}
				if (index.normal_index >= 0)
				{
					vert.normal = { attrib.normals[3 * index.normal_index],
									attrib.normals[3 * index.normal_index + 1],
									attrib.normals[3 * index.normal_index + 2] };
				}
				if (index.texcoord_index >= 0)
				{
					vert.uv = { attrib.texcoords[2 * index.texcoord_index],
								1 - attrib.texcoords[2 * index.texcoord_index + 1] };
				}
				vertices.push_back(vert); // add vertex
			}
		}
	}

	void MeshBuilder::makeCubeMesh()
	{
		vertices = {
			// -X red
			{{-.5f, -.5f, -.5f}, {.8f, .1f, .1f}},
			{{-.5f, .5f, .5f}, {.8f, .1f, .1f}},
			{{-.5f, -.5f, .5f}, {.8f, .1f, .1f}},
			{{-.5f, .5f, -.5f}, {.8f, .1f, .1f}},

			// X red
			{{.5f, -.5f, -.5f}, {.8f, .05f, .05f}},
			{{.5f, .5f, .5f}, {.8f, .05f, .05f}},
			{{.5f, -.5f, .5f}, {.8f, .05f, .05f}},
			{{.5f, .5f, -.5f}, {.8f, .05f, .05f}},

			// -Y green
			{{-.5f, -.5f, -.5f}, {.1f, .8f, .1f}},
			{{.5f, -.5f, .5f}, {.1f, .8f, .1f}},
			{{-.5f, -.5f, .5f}, {.1f, .8f, .1f}},
			{{.5f, -.5f, -.5f}, {.1f, .8f, .1f}},

			// Y green
			{{-.5f, .5f, -.5f}, {.05f, .8f, .05f}},
			{{.5f, .5f, .5f}, {.05f, .8f, .05f}},
			{{-.5f, .5f, .5f}, {.05f, .8f, .05f}},
			{{.5f, .5f, -.5f}, {.05f, .8f, .05f}},

			// Z blue
			{{-.5f, -.5f, 0.5f}, {.05f, .05f, .8f}},
			{{.5f, .5f, 0.5f}, {.05f, .05f, .8f}},
			{{-.5f, .5f, 0.5f}, {.05f, .05f, .8f}},
			{{.5f, -.5f, 0.5f}, {.05f, .05f, .8f}},

			// -Z blue
			{{-.5f, -.5f, -0.5f}, {.1f, .1f, .8f}},
			{{.5f, .5f, -0.5f}, {.1f, .1f, .8f}},
			{{-.5f, .5f, -0.5f}, {.1f, .1f, .8f}},
			{{.5f, -.5f, -0.5f}, {.1f, .1f, .8f}},
		};
		indices = { 0,  1,  2,  0,  3,  1,  4,  5,  6,  4,  7,  5,  8,  9,  10, 8,  11, 9,
								12, 13, 14, 12, 15, 13, 16, 17, 18, 16, 19, 17, 20, 21, 22, 20, 23, 21 };
	}

	void MeshBuilder::makeCubeMeshWireframe()
	{
		const auto v = .5f;
		vertices = {
			{{v, v, -v}},{{v, -v, -v}},{{-v, -v, -v}},{{-v, v, -v}},
			{{v, v, v}},{{v, -v, v}},{{-v, -v, v}},{{-v, v, v}}
		};
		indices = { 0,1,0, 1,2,1, 2,3,2, 3,0,3,		// floor
					0,4,0, 1,5,1, 2,6,2, 3,7,3,		// pillars
					4,5,4, 5,6,5, 6,7,6, 7,4,7 };	// ceiling
	}

	
}