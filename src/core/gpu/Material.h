#pragma once

#include "core/gpu/Device.h"
#include "core/gpu/Descriptors.h"
#include "core/engine/EngineSettings.h"
#include "core/types/vk.h"

#include <glm/glm.hpp> // TODO: get rid of this

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

	enum class EMatSet : uint32_t { YES, NO };

	// holds all properties needed to create a material object (used to generate a pipeline config)
	struct MaterialCreateInfo 
	{
		MaterialCreateInfo(const ShaderFilePaths& shadersIn, const std::vector<VkDescriptorSetLayout>& setLayoutsIn, 
						VkSampleCountFlagBits samples, const RenderingFormats& formats, size_t pushConstSize, EMatSet createSet = EMatSet::NO)
			: shaderPaths(shadersIn), descriptorSetLayouts(setLayoutsIn), samples{ samples }, 
			renderingFormats{ formats }, pushConstSize{ pushConstSize }, createDescriptorSet{ createSet }
		{};
		// the shading properties hold common settings like backface culling and polygon fill mode
		MaterialShadingProperties shadingProperties{};
		ShaderFilePaths shaderPaths; // SPIR-V shaders
		std::vector<VkDescriptorSetLayout> descriptorSetLayouts;
		VkSampleCountFlagBits samples;
		RenderingFormats renderingFormats; // for VK_KHR_dynamic_rendering
		size_t pushConstSize;
		EMatSet createDescriptorSet;
	};

	// a material object is mainly an abstraction around a VkPipeline
	class Material 
	{
	public:
		Material(const MaterialCreateInfo& matInfo, EngineDevice& device);
		~Material();
		Material(const Material&) = delete;
		Material& operator=(const Material&) = delete;

		void finalize(); // must be called after the material's descriptor set has been set up

		VkPipelineLayout getPipelineLayout() const;

		// binds this material's pipeline to the specified command buffer
		void bindToCommandBuffer(VkCommandBuffer commandBuffer) const;

		template<typename T>
		void writePushConstants(VkCommandBuffer cmdBuf, T& data) const 
		{
			vkCmdPushConstants(cmdBuf, pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0,
								sizeof(T), (void*)&data);
		}

		bool hasDescriptorSet() const { return descriptorSet.get(); }
		DescriptorSet& getDescriptorSet() { return *descriptorSet.get(); }
		const MaterialCreateInfo& getMaterialCreateInfo() const { return materialCreateInfo; };

	private:
		MaterialCreateInfo materialCreateInfo;

		EngineDevice& device;
		VkShaderModule vertexShaderModule = VK_NULL_HANDLE;
		VkShaderModule fragmentShaderModule = VK_NULL_HANDLE;
		VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
		VkPipeline pipeline = VK_NULL_HANDLE;

		std::shared_ptr<DescriptorSet> descriptorSet = nullptr; // material-specific descriptor set

		bool finalized = false; // catch error if we forget to call finalize() on the material before trying to render it

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
			VkDeviceAddress instanceBufferAddress; // 8 bytes
			uint32_t instanceID; // 4 bytes
			uint32_t _pad; // 4 bytes, total 16 bytes
		};

		// used for meshes managed interally by the engine, not spawned by the game
		struct EngineMeshPushConstants
		{
			glm::mat4 transform{ 1.f };
			glm::mat4 normalMatrix{1.f};
		};

		// TODO: make this smaller so it fits within guaranteed minimum (128)
		struct PlanetMeshPushConstants
		{
			glm::mat4 transform{ 1.f };
			glm::mat4 normalMatrix{ 1.f };
			glm::vec4 cameraPositionAndLOD{ 0.f };
			glm::vec4 patchCenterDirectionAndPlanetRadius{ 0.f };
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

	namespace ShaderInstanceData
	{
		struct TextGlyphInstanceData
		{
			glm::vec4 uvs{ 0.f };
			glm::vec4 vertexBounds{ 0.f };
			glm::vec2 basePos{ 0.f };
			float fontScale = 0;
			uint32_t textureIndex = 0;
		};

		struct UIElementInstanceData
		{
			glm::vec4 positionAndSize{ 0.f };
			glm::vec4 backgroundColor{ 1.f };
			glm::vec4 cornerRadius{ 0.f };
		};
	}

}
