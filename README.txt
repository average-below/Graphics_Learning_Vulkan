This project requires the Vulkan SDK and glfw-3.5.1.bin.WIN64, and glm. They are not included in the repository due to its size. Users are expected to download these files themselves and modify the ".env.cmake" file to set the correct paths to those files.

By default, the project expects the users to create a "Dependencies" folder and store the GLFW and GLM folders. For Vulkan SDK, users are expected to have in their C drive. 

Resource Links: 
1. Vulkan SDK: https://vulkan.lunarg.com/sdk/home
2. GLFW: https://www.glfw.org/download
3. GLM: https://github.com/g-truc/glm.git

For Windows: 
When building, always go to "build" folder (or create and be in a "build" folder) and open cmd.
When in cmd, type "cmake -S ../ -B . ". This should build the project.
After the build, opening the LveEngine solution file. 
Set LveEngine as the start up project. 
Then build with MSVC. 

When debugging, they should show a window with visual. 
