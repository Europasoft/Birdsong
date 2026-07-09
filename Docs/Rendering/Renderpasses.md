# VK_KHR_dynamic_rendering Migration Plan

## Old architecture

The project uses the traditional Vulkan renderpass approach:

- `Renderpass` class (`Render/Renderpass.h`) wraps `VkRenderPass` + `VkFramebuffer` creation
- `AttachmentUse` builds `VkAttachmentDescription2` for renderpass creation
- `Material` class (`GPU/Material.h`) requires a `VkRenderPass` handle for pipeline creation
- `Renderer` manages two renderpasses: `baseRenderpass` (MSAA) and `fxRenderpass` (post-processing). The fxRenderpass samples from the results of the baseRenderpass and renders the final result to the swapchain. 

## Why Switch to Dynamic Rendering

- **No more VkRenderPass objects** - eliminates upfront renderpass/framebuffer creation
- **No more VkFramebuffer objects** - attachments specified at render time
- **Simpler code** - fewer objects to manage and synchronize
- **More flexible** - easily change attachments without recreating pipelines
- **Better for modern techniques** - especially useful for variable-rate shading and ray tracing integration

## Migration Steps

### 1. Enable the Extension (Device.cpp)

Add to `deviceExtensions`:

```cpp
VK_KHR_DYNAMIC_RENDERING_EXTENSION_NAME
```

Enable the feature during device creation:

```cpp
VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
// chain into VkDeviceCreateInfo.pNext
```

### 2. Modify Pipeline Creation (Material.h/cpp)

Replace `VkRenderPass renderpass` in `MaterialCreateInfo` with rendering format info:

```cpp
struct RenderingFormats
{
    std::vector<VkFormat> colorFormats;
    VkFormat depthFormat = VK_FORMAT_UNDEFINED;
    VkFormat stencilFormat = VK_FORMAT_UNDEFINED;
};
```

In `createPipeline()`, use `VkPipelineRenderingCreateInfo`:

```cpp
VkPipelineRenderingCreateInfo renderingInfo{};
renderingInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
renderingInfo.colorAttachmentCount = colorFormats.size();
renderingInfo.pColorAttachmentFormats = colorFormats.data();
renderingInfo.depthAttachmentFormat = depthFormat;
// chain into VkGraphicsPipelineCreateInfo.pNext
```

Set `pipelineInfo.renderPass = VK_NULL_HANDLE`.

### 3. Replace Renderpass Class (Render/Renderpass.h/cpp)

Create a new lightweight `RenderingInfo` helper:

```cpp
// at render time, build VkRenderingAttachmentInfo for each attachment
VkRenderingAttachmentInfo colorAttachment{};
colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
colorAttachment.imageView = colorImageView;
colorAttachment.imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
colorAttachment.clearValue = {0.1f, 0.12f, 0.2f, 1.0f};
// for MSAA: set resolveMode, resolveImageView, resolveImageLayout

VkRenderingInfo renderingInfo{};
renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
renderingInfo.renderArea = {{0, 0}, extent};
renderingInfo.layerCount = 1;
renderingInfo.colorAttachmentCount = 1;
renderingInfo.pColorAttachments = &colorAttachment;
renderingInfo.pDepthAttachment = &depthAttachment;

vkCmdBeginRendering(cmdBuffer, &renderingInfo);
// ... draw commands ...
vkCmdEndRendering(cmdBuffer);
```

### 4. Handle Image Layout Transitions

Without renderpasses, layout transitions must be explicit. Add pipeline barriers before/after rendering:

```cpp
// transition color attachment to COLOR_ATTACHMENT_OPTIMAL before rendering
VkImageMemoryBarrier barrier{};
barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
barrier.newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
barrier.image = colorImage;
// ... set access masks and subresource range
vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
```

### 5. Update Renderer Class (Render/Renderer.cpp)

- Remove `baseRenderpass` and `fxRenderpass` members
- Keep `Attachment` objects for image management
- Replace `beginRenderpassBase()` / `beginRenderpassFx()` with new dynamic rendering begin functions
- Replace `endRenderpass()` with `vkCmdEndRendering()`
- Add explicit layout transitions between passes

### 6. Simplify Attachment Classes

- `Attachment` class can remain for image/view management
- `AttachmentUse` class can be simplified or removed (no more `VkAttachmentDescription2`)
- Framebuffer-related code can be deleted entirely

### 7. Update All Material Instantiations

Update all drawer classes that create materials to pass `RenderingFormats` instead of `VkRenderPass`:

- `MeshDrawer`, `SkyDrawer`, `FxDrawer`, `InterfaceDrawer`, `DebugDrawer`

## Files to Modify

| File | Changes |

|------|---------|

| `GPU/Device.cpp` | Enable extension + feature |

| `GPU/Material.h` | Replace renderpass with format info in `MaterialCreateInfo` |

| `GPU/Material.cpp` | Use `VkPipelineRenderingCreateInfo` in pipeline creation |

| `Render/Renderpass.h/cpp` | Deprecate or rewrite as helper |

| `Render/Renderer.h/cpp` | Use `vkCmdBeginRendering`, add barriers |

| `Render/Attachment.h/cpp` | Remove framebuffer-related code |

| `Draw/*.cpp` | Update material creation calls |

## Implementation Order

1. Enable extension in Device (keeps existing code working via compatibility)
2. Add new `RenderingFormats` struct to Material
3. Support both renderpass and dynamic rendering in pipeline creation (transition period)
4. Convert Renderer to use dynamic rendering
5. Update all drawers
6. Remove deprecated renderpass code