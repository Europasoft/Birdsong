#include "Core/GPU/Descriptors.h"

#include "Core/GPU/Device.h"
#include "Core/GPU/Image.h"
#include "Core/Types/Math.h"
#include "Core/GPU/Swapchain.h"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <iostream>

namespace EngineCore
{

	// *************** Descriptor Set Layout Builder *********************

	DescriptorSetLayout::Builder& DescriptorSetLayout::Builder::addBinding(
		uint32_t binding, VkDescriptorType descriptorType,
		VkShaderStageFlags stageFlags, uint32_t count)
	{
		assert(bindings.count(binding) == 0 && "Binding already in use");
		VkDescriptorSetLayoutBinding layoutBinding{};
		layoutBinding.binding = binding;
		layoutBinding.descriptorType = descriptorType;
		layoutBinding.descriptorCount = count; // array length
		layoutBinding.stageFlags = stageFlags;
		bindings[binding] = layoutBinding;
		return *this;
	}

	std::unique_ptr<DescriptorSetLayout> DescriptorSetLayout::Builder::build() const
	{
		return std::make_unique<DescriptorSetLayout>(device, bindings);
	}

	// *************** Descriptor Set Layout *********************

	DescriptorSetLayout::DescriptorSetLayout(
		EngineDevice& device, std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> bindings)
		: device{ device }, bindings{ bindings }
	{
		std::vector<VkDescriptorSetLayoutBinding> setLayoutBindings{};
		for (auto kv : bindings)
		{
			setLayoutBindings.push_back(kv.second);
		}

		VkDescriptorSetLayoutCreateInfo descriptorSetLayoutInfo{};
		descriptorSetLayoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
		descriptorSetLayoutInfo.bindingCount = static_cast<uint32_t>(setLayoutBindings.size());
		descriptorSetLayoutInfo.pBindings = setLayoutBindings.data();

		if (vkCreateDescriptorSetLayout(
			device.device(),
			&descriptorSetLayoutInfo,
			nullptr,
			&descriptorSetLayout) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to create descriptor set layout!");
		}
	}

	DescriptorSetLayout::~DescriptorSetLayout() {
		vkDestroyDescriptorSetLayout(device.device(), descriptorSetLayout, nullptr);
	}

	// *************** Descriptor Pool Builder *********************

	DescriptorPool::Builder& DescriptorPool::Builder::addPoolSize(
		VkDescriptorType descriptorType, uint32_t count)
	{
		poolSizes.push_back({ descriptorType, count });
		return *this;
	}

	DescriptorPool::Builder& DescriptorPool::Builder::setPoolFlags(
		VkDescriptorPoolCreateFlags flags)
	{
		poolFlags = flags;
		return *this;
	}
	DescriptorPool::Builder& DescriptorPool::Builder::setMaxSets(uint32_t count)
	{
		maxSets = count;
		return *this;
	}

	std::unique_ptr<DescriptorPool> DescriptorPool::Builder::build() const
	{
		return std::make_unique<DescriptorPool>(device, maxSets, poolFlags, poolSizes);
	}

	// *************** Descriptor Pool *********************

	DescriptorPool::DescriptorPool(EngineDevice& device, uint32_t maxSets,
		VkDescriptorPoolCreateFlags poolFlags,
		const std::vector<VkDescriptorPoolSize>& poolSizes)
		: device{ device }
	{
		VkDescriptorPoolCreateInfo descriptorPoolInfo{};
		descriptorPoolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
		descriptorPoolInfo.pPoolSizes = poolSizes.data();
		descriptorPoolInfo.maxSets = maxSets;
		descriptorPoolInfo.flags = poolFlags;

		if (vkCreateDescriptorPool(device.device(), &descriptorPoolInfo, nullptr, &descriptorPool) !=
			VK_SUCCESS) {
			throw std::runtime_error("failed to create descriptor pool!");
		}
	}

	DescriptorPool::~DescriptorPool()
	{
		vkDestroyDescriptorPool(device.device(), descriptorPool, nullptr);
	}

	bool DescriptorPool::allocateDescriptor(
		const VkDescriptorSetLayout descriptorSetLayout, VkDescriptorSet& descriptor) const
	{
		VkDescriptorSetAllocateInfo allocInfo{};
		allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
		allocInfo.descriptorPool = descriptorPool;
		allocInfo.pSetLayouts = &descriptorSetLayout;
		allocInfo.descriptorSetCount = 1;

		// Might want to create a "DescriptorPoolManager" class that handles this case, and builds
		// a new pool whenever an old pool fills up. But this is beyond our current scope
		if (vkAllocateDescriptorSets(device.device(), &allocInfo, &descriptor) != VK_SUCCESS)
		{
			return false;
		}
		return true;
	}

	void DescriptorPool::freeDescriptors(std::vector<VkDescriptorSet>& descriptors) const
	{
		vkFreeDescriptorSets(device.device(), descriptorPool,
			static_cast<uint32_t>(descriptors.size()), descriptors.data());

	}

	void DescriptorPool::resetPool()
	{
		vkResetDescriptorPool(device.device(), descriptorPool, 0);
	}

	// *************** Descriptor Writer *********************

	DescriptorWriter::DescriptorWriter(DescriptorSetLayout& setLayout, DescriptorPool& pool)
		: setLayout{ setLayout }, pool{ pool } {}

	DescriptorWriter& DescriptorWriter::writeBuffer(uint32_t binding, VkDescriptorBufferInfo* bufferInfo) 
	{
		assert(setLayout.bindings.count(binding) == 1 && "Layout does not contain specified binding");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(bindingDescription.descriptorCount == 1 && "Binding single descriptor info, but binding expects multiple");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pBufferInfo = bufferInfo;
		write.descriptorCount = 1;

		writes.push_back(write);
		return *this;
	}

	DescriptorWriter& DescriptorWriter::writeImage(uint32_t binding, VkDescriptorImageInfo* imageInfo, uint32_t arrSize)
	{
		assert(setLayout.bindings.count(binding) == 1 && "failed to write descriptor binding, not present in layout");

		auto& bindingDescription = setLayout.bindings[binding];

		assert(arrSize == bindingDescription.descriptorCount && "failed to write array descriptor binding, count must match set layout");

		VkWriteDescriptorSet write{};
		write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		write.descriptorType = bindingDescription.descriptorType;
		write.dstBinding = binding;
		write.pImageInfo = imageInfo;
		write.descriptorCount = arrSize; // if binding multiple, imageInfo must be ptr to array

		writes.push_back(write);
		return *this;
	}

	bool DescriptorWriter::build(VkDescriptorSet& set)
	{
		bool success = pool.allocateDescriptor(setLayout.getDescriptorSetLayout(), set);
		if (!success) { return false; }
		overwrite(set);
		return true;
	}

	void DescriptorWriter::overwrite(VkDescriptorSet& set)
	{
		for (auto& write : writes) { write.dstSet = set; }
		vkUpdateDescriptorSets(pool.device.device(), static_cast<uint32_t>(writes.size()), writes.data(), 0, nullptr);
	}

	//		NEW DYNAMIC UBO IMPLEMENTATION (START)

	void UBO_Struct::add(uelem t, const size_t& arrayLength) { add(std::vector<uelem>{t}, arrayLength); }

	void UBO_Struct::add(const std::vector<uelem>& t, const size_t& arrayLength)
	{ fields.push_back(UBO_StructLeaf(std::vector<uelem>{t}, arrayLength)); }
	
	UBO_Struct::UBO_StructLeaf::UBO_StructLeaf(const std::vector<uelem>& t, const size_t& arrl)
											: elems{ t }, arrlen{ arrl } {};

	// generates the correct memory offsets for the ubo data fields
	UBO_Layout::UBO_Layout(const UBO_Struct& typeLayout) 
	{
		// for each top level field in the ubo structure (Vulkan spec: "OpTypeStruct member")
		for (auto& f : typeLayout.fields) 
		{
			Leaf field{};
			field.arrlen = f.arrlen;

			align(f, bufferSize, field.offsets, field.sizes, field.stride); // find alignments for field
			fields.push_back(field);
			bufferSize += field.stride * field.arrlen;
		}
	}

	void UBO_Layout::align(UBO_Struct::UBO_StructLeaf f, const size_t& startOffset, 
						std::vector<size_t>& offsetsOut, std::vector<size_t>& sizesOut, size_t& strideOut) const
	{
		sizesOut.clear();
		std::vector<size_t> alignments{};

		for (auto& e : f.elems)
		{
			size_t size, alignment;
			getAlignmentForElementType(e, size, alignment);
			sizesOut.push_back(size);
			alignments.push_back(alignment);
			// Vulkan spec: "a structure has a base alignment equal to the largest base alignment of any of its members"
			alignments[0] = std::max(alignment, alignments[0]);
		}

		offsetsOut.clear();
		strideOut = 0;
		auto seek = startOffset;

		for (size_t i = 0; i < alignments.size(); i++)
		{ 
			const size_t offset = Math::roundUpToClosestMultiple(seek, alignments[i]); // calculate offset
			offsetsOut.push_back(offset);
			seek += (offset - seek) + sizesOut[i];
		}
		strideOut = seek - startOffset;
	}

	void UBO_Layout::getAlignmentForElementType(uelem e, size_t& sizeOut, size_t& alignmentOut) const
	{
		// vulkan imposes alignment requirements for data used in uniform buffer descriptors
		size_t a = 0, s = 0;
		if (e == uelem::scalar) { a = s = 4; } // 1 * scalar
		else if (e == uelem::vec2) { a = s = 8; } // 2 * scalar
		else if (e == uelem::vec3) { a = 16, s = 12; } // 4 * scalar
		else if (e == uelem::vec4) { a = s = 16; } // 4 * scalar
		else if (e == uelem::mat4) { a = 16, s = 64; } // 4 * scalar
		assert(s > 0 && "failed to get alignment for uniform buffer member element, unknown type");
		alignmentOut = a, sizeOut = s;
	}

	void UBO_Layout::accessElement(ElementAccessor loc, size_t& sizeOut, size_t& offsetOut)
	{
		if (loc.i >= fields.size()) { throw std::runtime_error("index exceeds uniform buffer fields"); }
		auto& f = fields[loc.i];
		if (loc.a >= f.arrlen) { throw std::runtime_error("array index exceeds uniform buffer field array length"); }
		if (loc.e >= f.offsets.size()) { throw std::runtime_error("element index exceeds uniform buffer field elements"); }
		auto elementBaseOffset = f.offsets[loc.e];
		offsetOut = elementBaseOffset + f.stride * loc.a;
		sizeOut = f.sizes[loc.e];
	}

	//		NEW DYNAMIC UBO IMPLEMENTATION (END)

	// *************** Uniform Buffer wrapper *********************

	UBO::UBO(const UBO_Layout& sLayout, uint32_t numBuffers, EngineDevice& device) : structLayout{ sLayout }
	{
		createBuffers(device, numBuffers);
	}

	void UBO::writeMember(const UBO_Layout::ElementAccessor& loc, void* data, const size_t& dataSize,
						uint32_t bufferIndex, bool flush)
	{
		size_t dstSize, dstOffset;
		structLayout.accessElement(loc, dstSize, dstOffset);

		if (dataSize != dstSize) { throw std::runtime_error("cannot write to uniform buffer, incompatible data size"); }
		getBuffer(bufferIndex)->writeToBuffer(data, dataSize, dstOffset);
		if (flush) { getBuffer(bufferIndex)->flush(); }
	}

	void UBO::createBuffers(EngineDevice& device, uint32_t numBuffers)
	{
		auto minOffsetAlignment = device.properties.limits.minUniformBufferOffsetAlignment;
		for (uint32_t i = 0; i < numBuffers; i++) 
		{
			buffers.push_back(std::make_unique<GBuffer>(device, structLayout.getBufferSize(), 1,
						VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT,
						minOffsetAlignment));
			buffers.back()->map();
		}
	}

	// *************** Descriptor set wrapper *********************

	void ImageArrayDescriptor::addImage(const std::vector<VkImageView>& views)
	{
		const size_t num = views.size(); // num arrays, usually framesInFlight
		if (arrays.empty()) { arrays.resize(num); }
		assert(arrays.size() == num && "failed to add image to array descriptor, mismatched image counts");

		for (size_t i = 0; i < num; i++)
		{
			// add the i:th image info to the i:th "copy" of the array
			VkDescriptorImageInfo info{};
			info.imageView = views[i];
			info.sampler = nullptr;
			info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // correct layout assumed
			arrays[i].push_back(info);
		}
	}

	DescriptorSet::DescriptorSet(EngineDevice& device)
		: device{ device }, framesInFlight{ EngineSwapChain::MAX_FRAMES_IN_FLIGHT } {};

	DescriptorSet::DescriptorSet(EngineDevice& device, uint32_t numBuffers)
		: device{ device }, framesInFlight{ numBuffers } {};

	void DescriptorSet::addUBO(const UBO_Struct& structureLayout, EngineDevice& device)
	{
		ubos.push_back(std::make_unique<UBO>(UBO_Layout(structureLayout), framesInFlight, device));
	}

	void DescriptorSet::addCombinedImageSampler(const VkImageView& view, const VkSampler& sampler)
	{
		VkDescriptorImageInfo info{};
		info.imageView = view;
		info.sampler = sampler;
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; // correct layout assumed
		samplerImageInfos.push_back(std::make_unique<VkDescriptorImageInfo>(info));
	}

	void DescriptorSet::addImageArray(const ImageArrayDescriptor& imageArray)
	{
		assert(!imageArray.arrays.empty() && "tried to add empty image array descriptor");
		imageArraysInfos.push_back(imageArray);
		numImagesTotal += static_cast<uint32_t>(imageArray.getArrayLength() * imageArray.arrays.size());
	}

	void DescriptorSet::addSampler(const VkSampler& sampler)
	{
		VkDescriptorImageInfo info{};
		info.sampler = sampler;
		info.imageView = VK_NULL_HANDLE; // just the sampler, no image assigned
		info.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		samplerInfos.push_back(std::make_unique<VkDescriptorImageInfo>(info));
	}

	void DescriptorSet::finalize()
	{
		assert(framesInFlight > 0 && "descriptor set must have framesInFlight set to a valid number");
		sets.resize(framesInFlight);

		uint32_t numUBOs = static_cast<uint32_t>(ubos.size());
		uint32_t numSamplerImages = static_cast<uint32_t>(samplerImageInfos.size());
		uint32_t numImageArrays = static_cast<uint32_t>(imageArraysInfos.size());
		uint32_t numSamplers = static_cast<uint32_t>(samplerInfos.size());
		
		DescriptorPool::Builder poolBuilder(device);
		if (numUBOs > 0) { poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, framesInFlight * numUBOs); }
		if (numSamplerImages > 0) { poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, numSamplerImages); }
		if (numImageArrays > 0) { poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, numImagesTotal); }
		if (numSamplers > 0) { poolBuilder.addPoolSize(VK_DESCRIPTOR_TYPE_SAMPLER, numSamplers); }
		
		pool = poolBuilder.build();
		
		DescriptorSetLayout::Builder layoutBuilder(device);
		// add uniform buffer bindings to layout
		for (uint32_t i = 0; i < numUBOs; i++) /* UBOs start at binding index 0 */
		{ layoutBuilder.addBinding(i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_ALL_GRAPHICS); }

		// add combined image sampler bindings to layout
		for (uint32_t i = 0; i < numSamplerImages; i++) /* place combined sampler bindings after UBOs */
		{ layoutBuilder.addBinding(i + numUBOs, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS); }

		// add image arrays to layout, one binding per array
		for (uint32_t i = 0; i < numImageArrays; i++) /* image arrays after combined image samplers */
		{
			layoutBuilder.addBinding(i + numUBOs + numSamplerImages,
				VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, VK_SHADER_STAGE_ALL_GRAPHICS, static_cast<uint32_t>(imageArraysInfos[i].getArrayLength()));
		}

		// add sampler-only bindings
		for (uint32_t i = 0; i < numSamplers; i++) /* last binding category */
		{ 
			layoutBuilder.addBinding(i + numUBOs + numSamplerImages + numImageArrays, 
			VK_DESCRIPTOR_TYPE_SAMPLER, VK_SHADER_STAGE_ALL_GRAPHICS);
		}
		
		layout = layoutBuilder.build();

		// create descriptors for each frame (UBOs have multiple internal buffers)
		for (uint32_t f = 0; f < framesInFlight; f++)
		{
			DescriptorWriter writer(*layout.get(), *pool);
			// add uniform buffers
			for (uint32_t u = 0; u < numUBOs; u++)
			{
				// this is required because vulkan keeps a handle to the buffer info,
				// reallocation of info objects causes bindings to fail, either silently or violently
				const auto dBufferInfo = getUBO(u).getBuffer(f)->descriptorInfo();
				bufferInfos.push_back(std::make_unique<VkDescriptorBufferInfo>(dBufferInfo));
				writer.writeBuffer(u, bufferInfos.back().get()); // sending pointer
			}

			// add combined image samplers
			for (uint32_t i = 0; i < numSamplerImages; i++)
			{
				writer.writeImage(i + numUBOs, samplerImageInfos[i].get());
			}

			// add image arrays
			for (uint32_t a = 0; a < numImageArrays; a++)
			{
				auto& infoArray = imageArraysInfos[a].arrays[f];
				writer.writeImage(a + numUBOs + numSamplerImages, infoArray.data(), static_cast<uint32_t>(infoArray.size()));
			}

			// add samplers
			for (uint32_t i = 0; i < numSamplers; i++)
			{
				writer.writeImage(i + numUBOs + numSamplerImages + numImageArrays, samplerInfos[i].get());
			}

			writer.build(sets[f]); // make descriptor set for frame
		}
	}

	VkDescriptorSetLayout DescriptorSet::getLayout() const
	{
		assert(layout.get() && "tried to get layout from uninitialized descriptor set");
		if (!layout.get()) { return VK_NULL_HANDLE; }
		return layout.get()->getDescriptorSetLayout();
	}

	UBO& DescriptorSet::getUBO(uint32_t uboIndex)
	{
		assert(uboIndex < ubos.size() && "ubo index out of range");
		return *ubos[uboIndex].get();
	}

	

}