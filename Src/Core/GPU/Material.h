#pragma once

#include "Core/GPU/Device.h"
#include "Core/GPU/Descriptors.h"
#include "Core/EngineSettings.h"

#include <glm/glm.hpp>

#include <string>
#include <vector>
#include <memory>

namespace EngineCore 
{
	struct PipelineConfig
	{
		PipelineConfig() = default;
		PipelineConfig(const PipelineConfig&) = delete;
		PipelineConfig& operator=(const PipelineConfig&) = delete;

		VkPipelineViewportStateCreateInfo viewportInfo{};
		VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo{};
		VkPipelineRasterizationStateCreateInfo rasterizationInfo{};
		VkPipelineMultisampleStateCreateInfo multisampleInfo{};
		VkPipelineColorBlendAttachmentState colorBlendAttachment{};
		VkPipelineColorBlendStateCreateInfo colorBlendInfo{};
		VkPipelineDepthStencilStateCreateInfo depthStencilInfo{};
		std::vector<VkDynamicState> dynamicStateEnables{};
		VkPipelineDynamicStateCreateInfo dynamicStateInfo{};
		VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
		VkPipelineLayout pipelineLayout = nullptr;
	};

	struct ShaderFilePaths
	{
		std::string vertPath;
		std::string fragPath;
		ShaderFilePaths() = default;
		ShaderFilePaths(const std::string& vert, const std::string& frag) : vertPath{ vert }, fragPath{ frag } {};
	};

	// holds common material-specific properties
	struct MaterialShadingProperties
	{
		VkPrimitiveTopology primitiveType = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
		VkPolygonMode polygonMode = VK_POLYGON_MODE_FILL;
		VkCullModeFlags cullModeFlags = VK_CULL_MODE_BACK_BIT; // backface culling
		float lineWidth = 1.f;
		bool useVertexInput = true; // enable when using vertex buffers
		bool enableDepth = true; // enables reads and writes to the depth attachment
	};

	// format info for dynamic rendering (VK_KHR_dynamic_rendering)
	struct RenderingFormats
	{
		std::vector<VkFormat> colorFormats;
		VkFormat depthFormat = VK_FORMAT_UNDEFINED;
		VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
	};

	// holds all properties needed to create a material object (used to generate a pipeline config)
	struct MaterialCreateInfo 
	{
		MaterialCreateInfo(const ShaderFilePaths& shadersIn, const std::vector<VkDescriptorSetLayout>& setLayoutsIn, 
						VkSampleCountFlagBits samples, const RenderingFormats& formats, size_t pushConstSize)
			: shaderPaths(shadersIn), descriptorSetLayouts(setLayoutsIn), samples{ samples }, 
			  renderingFormats{ formats }, pushConstSize{ pushConstSize } {};
		// the shading properties hold common settings like backface culling and polygon fill mode
		MaterialShadingProperties shadingProperties{};
		ShaderFilePaths shaderPaths; // SPIR-V shaders
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		VkSampleCountFlagBits samples;
		RenderingFormats renderingFormats; // for VK_KHR_dynamic_rendering
		size_t pushConstSize;
	};

	// a material object is mainly an abstraction around a VkPipeline
	class Material 
	{
	public:
		Material(const MaterialCreateInfo& matInfo, EngineDevice& device);
		~Material();
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		VkPipelineLayout getPipelineLayout() const { return pipelineLayout; }

		// binds this material's pipeline to the specified command buffer
		void bindToCommandBuffer(VkCommandBuffer commandBuffer) const;

		template<typename T>
		void writePushConstants(VkCommandBuffer cmdBuf, T& data) const 
		{
			vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
								sizeof(T), (void*)&data);
		}

		void setMaterialSpecificDescriptorSet(const std::shared_ptr<DescriptorSet>& set) { descriptorSet = set; }
		DescriptorSet* getMaterialSpecificDescriptorSet() { return descriptorSet.get(); }

	private:
		MaterialCreateInfo materialCreateInfo;

		EngineDevice& device;
		VkShaderModule vertexShaderModule;
		VkShaderModule fragmentShaderModule;
		VkPipelineLayout pipelineLayout;
		VkPipeline pipeline;

		std::shared_ptr<DescriptorSet> descriptorSet = nullptr; // material-specific descriptor set

		static void getDefaultPipelineConfig(PipelineConfig& cfg);
		static void applyMatPropsToPipelineConfig(const MaterialShadingProperties& mp, PipelineConfig& cfg);

		void createShaderModule(const std::string& path, VkShaderModule* shaderModule);
		void createPipelineLayout();
		void createPipeline();

	};

	namespace ShaderPushConstants 
	{
		struct MeshPushConstants
		{
			glm::mat4 transform{1.f};
			glm::mat4 normalMatrix{1.f};
		};

		struct InterfaceElementPushConstants
		{
			glm::vec2 position;
			glm::vec2 size;
			float timeSinceHover;
			float timeSinceClick;
		};

		struct DebugPrimitivePushConstants
		{
			glm::mat4 transform{ 1.f };
			glm::vec4 color;
		};
	}

}
