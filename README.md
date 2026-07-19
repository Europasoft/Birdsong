## Building
A build system is included, this project does not use CMake. [Read more about EuropaBuild here](https://github.com/Europasoft/EuropaBuild).<br>
To build it using EuropaBuild, you will need [Ninja](https://ninja-build.org/) and [Clang](https://clang.llvm.org/).

On Windows, these can be installed using the following PowerShell commands<br>
> `winget install Ninja-build.Ninja`<br>

> `winget install LLVM.LLVM`<br>

> `[Environment]::SetEnvironmentVariable("Path", [Environment]::GetEnvironmentVariable("Path", "User") + ";C:\Program Files\LLVM\bin", "User")`<br><br>

You will also need to install the [Vulkan SDK](https://vulkan.lunarg.com/sdk/home).<br>

### Build and run
1. Change the paths in `EuropaBuildDebug.json` to match the locations of Vulkan and [GLFW](https://www.glfw.org/) on your system.<br>You will find these under "include paths" and "lib paths".<br>[GLFW](https://www.glfw.org/) is bundled with the engine (in /src/thirdparty) 
2. Run `./EuropaBuild.exe -c EuropaBuildDebug.json`
3. Compile the demo shaders: this is done with the Python script in `Src/Core/DevResources/Shaders/CompileShaders.py`<br>(you will need to edit the paths inside this script before running it).
4. Launch Birdsong.exe

## File structure
```text
src/
├── core/
│   ├── engine/         Engine main internals.
│   ├── render/         Engine renderer and related files.
│   ├── gpu/            Engine graphics internals.
│   ├── draw/           Engine draw call dispatchers.
│   ├── input/          User input handling internals.
│   ├── types/          Common engine data structures and math.
│   ├── world/          Engine internals of the sector and scene systems.
│   ├── nodes/          Engine internals of the node system.
│   │               
│   └── include/    
│       ├── shared/     Public headers shared by game and engine code.
│       └── game/       Public headers to be included in game code only.
│                   
├── deps/               Git submodules of other repos required by the engine.
└── thirdparty/         Bundled third party libraries.
```
<img width="2055" height="820" alt="chrome_FB1lYEoXki" src="https://github.com/user-attachments/assets/9ec4f855-344c-404b-b7dc-3cfac14cd797" />
<br>
<img width="1208" height="893" alt="Birdsong_KQpQerGNSI" src="https://github.com/user-attachments/assets/ef25b3e9-230e-44ea-b197-1aef83ea2a7b" />



