#pragma once

#include "core/gpu/Buffer.h"

#include <vector>
#include <memory>
#include <unordered_map>

namespace EngineCore
{
	class EngineDevice;
	class DescriptorSetLayout;
	class DescriptorPool;

	enum class uelem { scalar, vec2, vec3, vec4, mat4 };
	// intermediate representation of a uniform buffer structure tree, used as a precursor to generate a UBO_Layout 
	class UBO_Struct
	{
	public:
		// adds a single data type to this structure (or an array containing that type)
		void add(uelem t, const size_t& arrayLength = 1);
		// adds a nested structure to this structure (or an array containing that structure)
		void add(const std::vector<uelem>& t, const size_t& arrayLength = 1);

	private:
		friend class UBO_Layout;
		// innermost ("leaf") layer in the structure tree - a nested structure or single data element
		struct UBO_StructLeaf
		{
			std::vector<uelem> elems{};
			size_t arrlen;
			// note that the array length is not the same as the number of elements
			UBO_StructLeaf(const std::vector<uelem>& t, const size_t& arrl);
		};

		std::vector<UBO_StructLeaf> fields; // elements and nested structures added to this structure
	};

	// the actual memory layout information for a uniform buffer structure
	class UBO_Layout
	{
		// memory offsets generated from a UBO_Struct::UBO_StructLeaf
		struct Leaf
		{
			std::vector<size_t> offsets{}; // element start offsets (all relative to buffer)
			std::vector<size_t> sizes{};
			size_t stride = 0; // instance size (includes inter-element alignment padding)
			size_t arrlen = 0; // number of instances in the array (total size = stride * arrlen)
		};

		std::vector<Leaf> fields; // offset and size information for all elements in the uniform buffer
		size_t bufferSize = 0; // required size for data + alignment padding

		void align(UBO_Struct::UBO_StructLeaf f, const size_t& startOffset, 
				std::vector<size_t>& offsetsOut, std::vector<size_t>& sizesOut, size_t& strideOut) const;
		void getAlignmentForElementType(uelem e, size_t& sizeOut, size_t& alignmentOut) const;
	public:
		UBO_Layout(const UBO_Struct& typeLayout);
		const size_t& getBufferSize() const { return bufferSize; }
		// field index, array index, element index
		struct ElementAccessor { size_t i, a, e; };
		void accessElement(ElementAccessor loc, size_t& sizeOut, size_t& offsetOut);
	};


	/* uniform buffer abstraction - this represents a specialized GPU buffer for in-shader (descriptor set) use */
	class UBO
	{
	public:
		UBO(const UBO_Layout& sLayout, uint32_t numBuffers, EngineDevice& device);
		GBuffer* getBuffer(uint32_t index) { return buffers[index].get(); }
	private:
		friend class DescriptorSet;
		
		UBO_Layout structLayout;
		std::vector<std::unique_ptr<GBuffer>> buffers;

		void createBuffers(EngineDevice& device, uint32_t numBuffers);
		void writeMember(const UBO_Layout::ElementAccessor& loc, void* data, const size_t& dataSize,
						uint32_t bufferIndex, bool flush);
	};

	struct ImageArrayDescriptor
	{
		// add an image to the array, takes multiple views to switch between for each frame in flight
		void addImage(const std::vector<VkImageView>& views);
		size_t getArrayLength() const { return arrays[0].size(); }
		VkDescriptorImageInfo* getArray(uint32_t frameIndex) { return arrays[frameIndex].data(); }
		// copies of the same image array, one for each frame in flight
		std::vector<std::vector<VkDescriptorImageInfo>> arrays;
	};

	// descriptor set abstraction, this enables descriptor sets to be managed as self-contained objects, 
	// and allows descriptors to be easily defined and bound at runtime
	class DescriptorSet
	{
	public:
		DescriptorSet(EngineDevice& device);
		DescriptorSet(EngineDevice& device, uint32_t numBuffers);
		DescriptorSet(const DescriptorSet&) = delete;
		DescriptorSet& operator=(const DescriptorSet&) = delete;

		// add a descriptor to the set, actual binding indices depend on the order in the finalize function
		void addUBO(const UBO_Struct& structureLayout, EngineDevice& device);
		void addCombinedImageSampler(const VkImageView& view, const VkSampler& sampler);
		void addImageArray(const ImageArrayDescriptor& imageArray);
		void addSampler(const VkSampler& sampler);

		void finalize(); // allocates descriptors, builds the set layout and VkDescriptorSets  

		template<typename T> // user-friendly uniform buffer data push function
		void writeUBOMember(uint32_t uboIndex, T& data, const UBO_Layout::ElementAccessor& position,
							uint32_t frameIndex, bool flush = true)
		{ 
			getUBO(uboIndex).writeMember(position, (void*)&data, sizeof(T), frameIndex, flush); 
		}

		UBO& getUBO(uint32_t uboIndex);
		VkDescriptorSetLayout getLayout() const;
		VkDescriptorSet getDescriptorSet(uint32_t frameIndex) const 
		{ 
			return sets[frameIndex]; 
		}

	private:
		std::unique_ptr<DescriptorPool> pool{};
		std::unique_ptr<DescriptorSetLayout> layout; // layout of this set
		std::vector<VkDescriptorSet> sets; // per frame (identical layout)
		std::vector<std::unique_ptr<UBO>> ubos; // managed ubo (each has internal per-frame buffers)
		std::vector<std::unique_ptr<VkDescriptorBufferInfo>> bufferInfos; // descriptor infos necessary to preserve pointers for vulkan
		std::vector<std::unique_ptr<VkDescriptorImageInfo>> samplerImageInfos;
		std::vector<ImageArrayDescriptor> imageArraysInfos;
		uint32_t numImagesTotal = 0;
		std::vector<std::unique_ptr<VkDescriptorImageInfo>> samplerInfos;
		
		EngineDevice& device;
		// num copies to create of each buffer, usually MAX_FRAMES_IN_FLIGHT, 
		// but may instead be the swapchain image count, when reading from an attachment image in a shader
		uint32_t framesInFlight;

		bool finalized = false; // catch error if we forget to call finalize() on the descriptor set before trying to render it
	};

}
