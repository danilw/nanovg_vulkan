added vulkan build, and cmake files, removed premake4.lua

**Why I made this** - I made it as a "test task for Vulkan related job" a year ago, for me this project has no real use. I would recommend for everyone use [**imgui**](https://github.com/ocornut/imgui) if you look for production-ready Vulkan UI. I will not support or update this project at all, the last update was just to fix bugs and Validation errors.

**Description** - nanovg in C, no external dependencies. GLFW used only by one example (look examples description, there is example without using GLFW).

### Contact: [**Join discord server**](https://discord.gg/JKyqWgt)
___

To launch - copy builded binary to *example* folder(or launch when this folder is current as on build example commands below). Because required fonts and images/shaders to load.
___

# Build:
(use cmake to build on Windows, Windows/Linux supported and tested)

```
git clone --recursive https://github.com/danilw/nanovg_vulkan
cd nanovg_vulkan/example
mkdir build
cd build
cmake ../
make
cd ../
./build/example-vk
./build/example-vk_min_no_glfw
```

Look *Examples description* below there link to repository with *C only minimal example* without dependencies not using any library.

**MoltenVK note** - after `TOPOLOGY_TRIANGLE_LIST` update(look below) this Vulkan port does work on MoltenVK(Mac/etc) but I dont have it to test so you should make cmake config to build it and launch by yourself.
___

# Examples description:

MAX_FRAMES_IN_FLIGHT

*Multiple frames in flight* - only ***nanovg-vulkan-glfw-integration-demo***(link below) has example of using multiple frames in flight. Everything else use single frame, *this is reason of low FPS compare to OpenGL*.

**example_vulkan.c** - minimal NanoVG example that use GLFW.

**example_vulkan_min_no_glfw.c** - same as above but not using GLFW, supported Linux and Windows.

### Two external examples:

**[nanovg-vulkan-min-integration-demo](https://github.com/danilw/nanovg-vulkan-min-integration-demo)** - repository with minimal RenderPass integration example. Not using any libraries, no GLFW, only NanoVG code and C. **Look description and screenshot on link**.

**[nanovg-vulkan-glfw-integration-demo](https://github.com/danilw/nanovg-vulkan-glfw-integration-demo)** - repository with example from Vulkan-tutorial Depth buffering modified adding NanoVG integration. Using C++ and GLFW.

*Remember NanoVG is not GUI*, and examples about is just examples of this NanoVG integration, not GUI examples.

About *RenderPass integration* - copy paste code from *integration* examples above after your `vkCmdEndRenderPass` (or before `vkCmdBeginRenderPass`) and everything should work.

*Framebuffer integration*, where UI rendered in its own Framebuffer - I did not add example for this case, because it should be obvious - just replace framebuffer RenderPass with RenderPass of NanoVG *integration* examples above(look linked commit).
___

### 2022 update 2 - few people ask me for more examples, so I added more examples.
___

### 2022 update - thanks to [**@nidefawl**](https://github.com/nidefawl) [pull request](https://github.com/danilw/nanovg_vulkan/pull/5) with lots of changes:

In this PR:
1. Merge latest nanovg

2. Fix Stencil Strokes: 
I added a new enum to handle the 3 different pipelines used for stencil strokes.  
I wanted to also use enums for the fill-path (nvgFill) to make the code cleaner, but haven't done that yet.

3. I combined the 2 CMakeLists.txt into example/CMakeLists.txt and added some bits to make it easier to build and debug in MSVC/VSCode.

4. I added GLAD as default gl-loader for the GL3 example. It can be switched back to GLEW (I kept glew to allow easier merge with main nanovg)

5. I Increased the number of swapchain images to minimum 3 and updated the swapchain barrier to gain some extra performance.

6. Spacebar renders the demo multiple times to add some 'load' when comparing performance
___

### 2021 update - fixed all errors, this code has no Validation errors and work correctly everywhere.

Thanks to [**@fzwoch**](https://github.com/fzwoch) [commits](https://github.com/danilw/nanovg_vulkan/pull/1) **by default used `TOPOLOGY_TRIANGLE_LIST`**, because `TOPOLOGY_TRIANGLE_FAN` is optional in Vulkan. 

To enable *TOPOLOGY_TRIANGLE_FAN* edit `src/nanovg_vk.h` and set there `#define USE_TOPOLOGY_TRIANGLE_FAN`

Depth order bug on AMD fix by [**@leranger**](https://github.com/leranger) [6ee1009](https://github.com/danilw/nanovg_vulkan/commit/6ee100956134cab2aab67a6a8a7a5bda54c0f9ab).
___

**Screenshot** of Vulkan version from Linux:

![nvgvk](https://danilw.github.io/GLSL-howto/nvgvk.png)
___

Read original readme for more info about NanoVG API https://github.com/memononen/nanovg
