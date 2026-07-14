
## Building
A build system is included, this project does not use CMake. [Read more about EuropaBuild here](https://github.com/Europasoft/EuropaBuild).<br><br>
1. Download the following dependencies:
    - [Vulkan SDK](https://vulkan.lunarg.com/sdk/home)
    - [GLFW](https://www.glfw.org/)
2. Change the paths in `EuropaBuildDebug.json` to match the locations of Vulkan and GLFW on your system.<br>
You will find these under "include paths" and "lib paths".
3. Run `./EuropaBuild.exe -c EuropaBuildDebug.json`
4. Compile the demo shaders: this is done with the Python script in `Src/Core/DevResources/Shaders/CompileShaders.py` (you will need to edit the paths inside this script before running it).
5. Launch Birdsong.exe

<img width="1208" height="893" alt="Birdsong_KQpQerGNSI" src="https://github.com/user-attachments/assets/ef25b3e9-230e-44ea-b197-1aef83ea2a7b" />

