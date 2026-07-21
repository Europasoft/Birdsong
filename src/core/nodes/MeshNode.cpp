#include "core/nodes/MeshNode.h"
#include "core/engine/MeshData.h"
#include "core/gpu/Device.h"
#include "core/gpu/Material.h"
#include "core/gpu/Buffer.h"

#include "deps/box3d-cpp/include/b3cpp.h"

#include <cassert>
#include <cstring>

namespace Nodes
{
	MeshNode::MeshNode()
	{}

	MeshNode::~MeshNode()
	{}

	void MeshNode::build(const std::filesystem::path& meshFilePath)
	{
		EngineCore::MeshBuilder builder{};
		if (!meshFilePath.empty())
			builder.loadFromFile(makePath(meshFilePath));
		else
			builder.makeCubeMesh();
		build(builder);
	}

	void MeshNode::build(const MeshBuilder& meshBuilder)
	{
		createVertexBuffers(meshBuilder.vertices);
		createIndexBuffers(meshBuilder.indices);
	}

	void MeshNode::addPhysicsBody(b3cpp::BodyDef def, b3cpp::World& w)
	{
		physicsBody = w.createBody(def);
		b3cpp::SphereShape& s = physicsBody->createShape<b3cpp::SphereShape>();
		b3cpp::ShapeDef shapeDef;
		assert(physicsBody->isIdValid());
		s.activate(shapeDef);
	}

	void MeshNode::physicsTick()
	{
		if (not physicsBody) return;
		
		if (teleported)
		{
			// the transform was manually changed, inform the physics engine
			const auto& p = transform.translation;
			const auto& r = transform.rotation;
			physicsBody->setTransform({ p.x, p.y, p.z }, { r.x, r.y, r.z, transform.rotation_w });
			// reset so this doesn't happen every tick
			teleported = false;
		}
		else
		{
			// update transform with data from physics engine
			const b3cpp::Vector pos = physicsBody->getPosition();
			const b3cpp::Vector rot = physicsBody->getRotationQuat();
			transform.translation.x = pos.x;
			transform.translation.y = pos.y;
			transform.translation.z = pos.z;
			transform.rotation.x = rot.x;
			transform.rotation.y = rot.y;
			transform.rotation.z = rot.z;
			transform.rotation_w = rot.w;
		}
	}

	void MeshNode::createVertexBuffers(const std::vector<EngineCore::Vertex>& vertices)
	{
		EngineCore::EngineDevice& device = getDevice();
		//generateOOBB(vertices);
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

	void MeshNode::createIndexBuffers(const std::vector<uint32_t>& indices)
	{
		EngineCore::EngineDevice& device = getDevice();
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

	//void MeshNode::generateOOBB(const std::vector<Vertex>& vertices)
	//{
	//	for (const auto& v : vertices)
	//	{
	//		extent.x = std::max(extent.x, std::abs(v.position.x));
	//		extent.y = std::max(extent.y, std::abs(v.position.y));
	//		extent.z = std::max(extent.z, std::abs(v.position.z));
	//	}
	//}

	void MeshNode::bind(VkCommandBuffer commandBuffer) const
	{
		VkBuffer buffers[] = { vertexBuffer->getBuffer() };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);
		if (hasIndexBuffer) { vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0, VK_INDEX_TYPE_UINT32); }
	}

	void MeshNode::draw(VkCommandBuffer commandBuffer) const
	{
		if (hasIndexBuffer) { vkCmdDrawIndexed(commandBuffer, indexCount, 1, 0, 0, 0); }
		else { vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0); }
	}

	

}