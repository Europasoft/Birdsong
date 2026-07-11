Future Architecture: Modernizing the Descriptor Pipeline
Our current descriptor system (Descriptors.cpp) uses traditional Vulkan 1.0 management (explicit descriptor sets, per-frame binding updates, and complex manual dynamic UBO memory alignment calculations). Upgrading the project to a modern Vulkan 1.2 / GLSL 460 architecture allows us to delete the vast majority of this boilerplate.

1. Delete Manual UBO Alignment Calculations
The Problem: We currently use UBO_Struct, UBO_Layout, and custom padding rules (std140) to make sure C++ structures line up safely with GLSL uniform blocks.

The Modern Fix: Enable the Vulkan 1.2 device feature scalarBlockLayout.

Shader Changes (#version 460): Use layout(scalar) on memory blocks.

The Result: The GPU interprets structures exactly like C++ does (no random 16-byte padding on vec3 or array fields). We can pass raw C++ structs via standard memcpy and completely delete the manual alignment calculation code.

2. Eliminate Per-Frame UBO Descriptors (Use Pointers)
The Problem: We create descriptor pools, sets, and layouts to pass basic frame and material data (UBOs) to shaders, generating massive host-side overhead tracking frames-in-flight.

The Modern Fix: Enable Buffer Device Address (bufferDeviceAddress feature in Vulkan 1.2).

Shader Changes (#version 460): Use #extension GL_EXT_buffer_reference : require.

The Result: Shaders can reference memory directly using raw 64-bit GPU memory addresses (pointers). We get the buffer address on creation with vkGetBufferDeviceAddress, pass it directly via Push Constants, and dereference it inside the shader like normal C++. We can throw away all uniform descriptor sets.

3. Replace Material Bindings with a Global Bindless Pool
The Problem: Our DescriptorSet::finalize() spends considerable logic tracking specific slots (numUBOs + numSamplerImages) and writing texture arrays on the fly. Changing a shader layout breaks the host-side pipeline index offsets.

The Modern Fix: Enable Descriptor Indexing (specifically descriptorBindingPartiallyBound and runtimeDescriptorArray features in Vulkan 1.2).

The Result: We create exactly one global descriptor set at engine initialization containing a single massive array of textures (e.g., sized at 10,000+ slots).

When an asset loads, it is written to an open index in this array once.

Shaders look up textures by an integer index passed via a push constant (globalTextures[push.textureId]).

DescriptorWriter and per-frame texture descriptors can be completely removed.

⚙️ Target Settings Required for the Upgrade
When ready to implement, verify the configuration:

GLSL Compiler: Ensure shaders start with #version 460.

Vulkan API Version: Initialize VkApplicationInfo::apiVersion to at least VK_API_VERSION_1_2.

Physical Device Features: Enable bufferDeviceAddress, scalarBlockLayout, and descriptorIndexing inside the device creation pNext chain.