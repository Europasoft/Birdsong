#include "Core/Primitive.h"
#include "Core/GPU/Material.h"

#include <cassert>
#include <cstring>

#define TINYOBJLOADER_IMPLEMENTATION // mesh file loader
#include "Core/ThirdParty/tiny_obj_loader.h"

namespace EngineCore
{
	Primitive::Primitive(EngineDevice& device, const MeshBuilder& builder) : device{ device }
	{
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	Primitive::Primitive(EngineDevice& device, const std::vector<Vertex>& vertices) : device{ device }
	{
		createVertexBuffers(vertices);
	}

	Primitive::Primitive(EngineDevice& device) : device{ device }
	{
		Primitive::MeshBuilder builder{};
		builder.makeCubeMesh();
		createVertexBuffers(builder.vertices);
		createIndexBuffers(builder.indices);
	}

	void Primitive::setMaterial(std::shared_ptr<Material> newMaterial) { material = newMaterial; }

	void Primitive::setMaterial(const MaterialCreateInfo& info) { material = std::make_shared<Material>(info, device); }

	std::shared_ptr<Material> Primitive::getMaterial() const { return material; }

    bool Primitive::isPointInsideOOBB(const Vec& point)
    {
		assert(false && "not implemented (TODO)");
		return false;
    }

    void Primitive::createVertexBuffers(const std::vector<Vertex>& vertices)
	{
		generateOOBB(vertices);
		vertexCount = static_cast<uint32_t>(vertices.size());
		assert(vertexCount >= 3 && "vertexCount cannot be below 3");
		VkDeviceSize bufferSize = sizeof(vertices[0]) * vertexCount;
		uint32_t vertexSize = sizeof(vertices[0]);
		// temporary buffer to transfer from CPU (host) to GPU (device)
		GBuffer stagingBuffer
		{
			device, vertexSize, vertexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)vertices.data()); // write vertices

		// destination buffer, GPU only for speed (not host accessible)
		vertexBuffer = std::make_unique<GBuffer>(device, vertexSize, vertexCount,
			VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

		device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), bufferSize);
	}

	void Primitive::createIndexBuffers(const std::vector<uint32_t>& indices)
	{
		indexCount = static_cast<uint32_t>(indices.size());
		hasIndexBuffer = indexCount > 0;
		if (!hasIndexBuffer) { return; }
		VkDeviceSize bufferSize = sizeof(indices[0]) * indexCount;
		uint32_t indexSize = sizeof(indices[0]);
		// same as for vertex buffer
		GBuffer stagingBuffer
		{
			device, indexSize, indexCount,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT
		};
		stagingBuffer.map();
		stagingBuffer.writeToBuffer((void*)indices.data());

		indexBuffer = std::make_unique<GBuffer>(device, indexSize, indexCount,
			VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT); // note INDEX_BUFFER_BIT

		device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), bufferSize);
	}

	void Primitive::generateOOBB(const std::vector<Vertex>& vertices)
	{
		for (const auto& v : vertices)
		{
			extent.x = std::max(extent.x, std::abs(v.position.x));
			extent.y = std::max(extent.y, std::abs(v.position.y));
			extent.z = std::max(extent.z, std::abs(v.position.z));
		}
	}

	void Primitive::bind(VkCommandBuffer commandBuffer)
	{
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		if (hasIndexBuffer) { vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32); }
	}

	void Primitive::draw(VkCommandBuffer commandBuffer)
	{
		if (hasIndexBuffer) { vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0); }
		else { vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0); }
	}

	void Primitive::MeshBuilder::loadFromFile(const std::string& path)
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

	void Primitive::MeshBuilder::makeCubeMesh()
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

	void Primitive::MeshBuilder::makeCubeMeshWireframe()
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

	std::vector<VkVertexInputBindingDescription> Primitive::Vertex::getBindingDescriptions()
	{
		std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
		bindingDescriptions[0].binding = 0;
		bindingDescriptions[0].stride = sizeof(Vertex);
		bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
		return bindingDescriptions;
	}

	std::vector<VkVertexInputAttributeDescription> Primitive::Vertex::getAttributeDescriptions()
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

}