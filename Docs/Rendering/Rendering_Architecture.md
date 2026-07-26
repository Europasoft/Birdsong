TODO: fully describe how rendering works and where the relevant classes live

`Material` class: manages a `VkPipeline`, a set of `VkShaderModule`s, and a material-specific `DescriptorSet` (managing a `VkDescriptorSet`).

```text
src/
├── core/
│   ├── engine/         Engine main internals.
│   │   └── interop/     Internal engine-game interoperability.
│   ├── render/         Engine renderer and related files.
│   ├── gpu/            Engine graphics internals.
│   ├── draw/           Engine draw call dispatchers.
│   ├── nodes/          Engine internals of the node system.
│   │               
│   └── include/    
│       ├── shared/     Public headers shared by game and engine code.
│       └── game/       Public headers to be included in game code only.
│                   
├── deps/               Git submodules of other repos required by the engine.
└── thirdparty/         Bundled third party libraries.
```

## Descriptor sets
Descriptor sets provide data to the shaders. This data flows from the CPU (C++ code) to the GPU (GLSL/SPIR-V shaders).<br>
The **Scene-global descriptor set** (maintained by the WorldSystem `Scene` class) handles data that **all** shaders will have access to.