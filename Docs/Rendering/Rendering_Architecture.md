## Common classes
- `Material` class: manages a `VkPipeline`, a set of `VkShaderModule`s, (and *in some cases* a material-specific `DescriptorSet`, a wrapper around`VkDescriptorSet`).
- `GBuffer` class: an abstraction around a GPU buffer. These may contain images (textures), or other data to be read by shaders.

## Descriptor sets
Descriptor sets provide data to the shaders. This data flows from the CPU (C++ code) to the GPU (GLSL/SPIR-V shaders).<br>
They must be allocated and bound before rendering. The final buffer ideally lives entirely in VRAM (GPU memory).<br>
The engine has custom utilities that greatly reduce the complexity of creating, binding, and updating descriptors.<br>
There are limitations to how many descriptor sets can be bound at one time, so we use these sparingly.<br>
In addition, there are better ways to pass data to shaders - read more below.

## Push constants
Push constants are small pieces of data posted directly into the command buffer prior to a draw command.<br>
These are very fast and much easier to manage than descriptor sets, but they are limited in size.

## Buffer Device Address (BDA)
BDA allows shaders to read from a GPU buffer through a pointer-like address.<br>
This combined with SSBOs (Shader Storage Buffer Objects) enables acces to large buffers, without requiring descriptor set bindings.

## Data sources
### Global descriptor set
The **Scene-global descriptor set** (maintained by the WorldSystem `Scene` class) handles data that **all** shaders will have access to.
### Instance buffer
The instance buffer `EngineCore::InstanceBuffer` is maintained by the `Scene` class.<br>
At the start of every frame, it is populated with instance-specific data (like transformation matrices for meshes).<br>
This buffer is an SSBO, and using BDA means it does not need to be put into a descriptor set.<br>
Push constants are used to give each draw call access to the buffer, the push constant contains a buffer address (pointer) and an instance ID.<br>
The address is used in the shader to read from the buffer, the instance ID is used to find the right offset.<br>
<br>
With this approach we only need the following descriptor sets:
- The Scene global set, which contains general frame data, such as the camera's view matrix.
- Another set for textures.

Note that most draw calls use only the resources listed above, however, some systems in the engine use other descriptors and/or push constants, but these are not exposed to the game at all.

> TODO: fully describe how rendering works and where the relevant classes live
