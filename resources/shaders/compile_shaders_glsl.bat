@echo off
title GLSL to SPIR-V shader compiler
@echo on

D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe shader.vert -o shader.vert.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe shader.frag -o shader.frag.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe sky.vert -o sky.vert.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe sky.frag -o sky.frag.spv

D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe fx_test.vert -o fx_test.vert.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe fx_test.frag -o fx_test.frag.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe fullscreen.vert -o fullscreen.vert.spv
D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe fullscreen.frag -o fullscreen.frag.spv

D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe pbr.frag -o pbr.frag.spv

D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe shader2test.frag -o shader2test.frag.spv

D:\VulkanDev\VulkanSDK\1.3.246.1\Bin\glslc.exe shaderDifferentColor.frag -o shaderDifferentColor.frag.spv

@echo off
echo.
echo GLSLC completed

timeout /t 30 /NOBREAK > nul
exit