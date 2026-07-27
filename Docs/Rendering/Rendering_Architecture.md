## Common classes
- `Material` class: manages a `VkPipeline`, a set of `VkShaderModule`s, (and *in some cases* a material-specific `DescriptorSet`, a wrapper around`VkDescriptorSet`).
- `GBuffer` class: an abstraction around a GPU buffer. These may contain images (textures), or other data to be read by shaders.
- `Image` class: an abstraction around a VkImage (texture). It manages its own memory, and has utilities to allocate, fill, change layout, and to create views and samplers. Images can be bound to descriptor sets or inserted into the unbounded texture buffer.

## Passing data to shaders
### Descriptor sets
Descriptor sets provide data to the shaders. This data flows from the CPU (C++ code) to the GPU (GLSL/SPIR-V shaders).<br>
They must be allocated and bound before rendering. The final buffer ideally lives entirely in VRAM (GPU memory).<br>
The engine has custom utilities that greatly reduce the complexity of creating, binding, and updating descriptors.<br>
There are limitations to how many descriptor sets can be bound at one time, so we use these sparingly.<br>
In addition, there are better ways to pass data to shaders - read more below.

### Set binding
- When a `Material` is created, the **layouts** of all **sets** used in its shaders must be specified (`Scene::getDescriptorSetLayouts`).

- Before submitting a draw command, all descriptor sets used by the shaders must be **bound** (`Scene::getDescriptorSets`).<br> 
These shared descriptor sets are maintained by the `Scene` class<br>(see , `Scene::getDescriptorSetLayouts`, `Scene::getDescriptorSets`).

### Push constants
Push constants are small pieces of data posted directly into the command buffer prior to a draw command.<br>
These are very fast and much easier to manage than descriptor sets, but they are limited in size.

### Buffer Device Address (BDA)
BDA allows shaders to read from a GPU buffer through a pointer-like address.<br>
This combined with SSBOs (Shader Storage Buffer Objects) enables acces to large buffers, without requiring descriptor set bindings.

## Data sources used
The engine uses the following ways to feed shaders with data:
### Global descriptor set (set 0)
The **Scene-global descriptor set** () handles data that **all** shaders will have access to, for example the camera's view matrix. (See `Scene::initGlobalDescriptorSet`, `Scene::updateDescriptors`).
### Texture descriptor set (set 1)
The second descriptor set contains an unbounded (sparse) array of textures. The intention is to store almost all textures here.<br>
An instance of the `BindlessTextureManager` class manages the texture array and this set, the instance is in turn retained by `Scene`.<br>



### Instance buffer
The instance buffer, an instance of the `EngineCore::InstanceBuffer` class is also maintained by the `Scene`.<br>
This buffer is an SSBO, and using BDA means it does not need to be put into a descriptor set.<br>
At the start of every frame, it is populated with instance-specific data (like transformation matrices for meshes).<br>See `Scene::updateInstanceData`.<br>
**Push constants** are used to give each draw call access to the buffer, the push constant contains a buffer address (pointer) and an instance ID.<br>
The address is used in the shader to read from the buffer, the instance ID is used to find the right offset.<br>
### In summary
- Descriptor set 0 - frame globals
- Descriptor set 1 - textures

These sets are managed by the `Scene` class , ).<br>
<br>


Note that most draw calls use only the resources listed above, however, some systems in the engine use other descriptors and/or push constants, but these are not exposed to the game at all.

> TODO: fully describe how rendering works and where the relevant classes live
