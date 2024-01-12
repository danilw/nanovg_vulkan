added vulkan build, and cmake files, removed premake4.lua

**Why I made this** - I made it as a "test task for Vulkan related job" a year ago, for me this project has no real use. I would recommend for everyone use [**imgui**](https://github.com/ocornut/imgui) if you look for production-ready Vulkan UI, also look on [**egui**](https://github.com/emilk/egui/). I will not support or update this project at all, the last update was just to fix bugs and Validation errors.

**Description** - nanovg in C, no external dependencies. GLFW used only by one example (look examples description, there is example without using GLFW).

### Contact: [**Join discord server**](https://discord.gg/JKyqWgt)
___

To launch - copy builded binary to *example* folder(or launch when this folder is current as on build example commands below). Because required fonts and images/shaders to load.
___



### Known bugs-related info:

- **Bug** - **DPI scale is broken** when it `!=1`, Currectly - DPI scale set to 1 always. Read https://github.com/danilw/nanovg-vulkan-glfw-integration-demo/issues/1
- **Bug** - **crash on resize on AMD-GPU only in Linux in Wayland**. Read https://github.com/danilw/nanovg-vulkan-glfw-integration-demo/issues/2
- Fix what Cppcheck pointing, unused variables etc, not bugs.
- Performance - after SubiyaCryolite pull https://github.com/danilw/nanovg_vulkan/pull/7 performance of Vulkan version very close to OpenGL.
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

*Multiple frames in flight* - *example_vulkan.c* is multiple frames in flight example, *example_vulkan_min_no_glfw.c* is single frame in flight. Clearly visible on FPS - with multiple frames about 3x better FPS.
___

**example_vulkan.c** - minimal NanoVG example that use GLFW.

**example_vulkan_min_no_glfw.c** - same as above but not using GLFW, supported Linux and Windows.

<!---
### Two external examples:

**[nanovg-vulkan-min-integration-demo](https://github.com/danilw/nanovg-vulkan-min-integration-demo)** - repository with minimal RenderPass integration example. Not using any libraries, no GLFW, only NanoVG code and C. **Look description and screenshot on link**.

**[nanovg-vulkan-glfw-integration-demo](https://github.com/danilw/nanovg-vulkan-glfw-integration-demo)** - repository with example from Vulkan-tutorial Depth buffering modified adding NanoVG integration. Using C++ and GLFW.

*Remember NanoVG is not GUI*, and examples about is just examples of this NanoVG integration, not GUI examples.

About *RenderPass integration* - copy paste code from *integration* examples above after your `vkCmdEndRenderPass` (or before `vkCmdBeginRenderPass`) and everything should work.

*Framebuffer integration*, where UI rendered in its own Framebuffer - I did not add example for this case, because it should be obvious - just replace framebuffer RenderPass with RenderPass of NanoVG *integration* examples above(look linked commit).
-->
___
## 2023 update - huge performance improvement thanks to [**@SubiyaCryolite**](https://github.com/SubiyaCryolite) 

SubiyaCryolite pull request [Optimizations > Cached Descriptor Sets, Implied Multiple Frames in Flight, Fencing for faster perf](https://github.com/danilw/nanovg_vulkan/pull/7)

-   Support for multiple command buffers, specifically one per swap-chain image / buffer
-   Caching of descriptor sets, removing the need to create new ones per draw func or to call `vkResetDescriptorPool` per frame.
-   Under `example_vulkan`, using `vkWaitForFences` to control rendering as opposed to `vkQueueWaitIdle` (seems to be the biggest perf booster). This change also has implied "multiple frames in flight" as dictated by the swap-chain image count.
-   Using persisted mapped buffers via `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT` instead of `VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT` , allows to skip calls to `vkMap/UnmapMemory` per frame

*Link to version before this change [5ba9d31](https://github.com/danilw/nanovg_vulkan/tree/5ba9d31165cebfb06abb9929ee9aa69a0abe86b1), this change [10d5211](https://github.com/danilw/nanovg_vulkan/commit/10d5211ac678f615a58462c270dff9eb7ffe4872).* 
___

<!---
___

### 2022 update 2 - few people ask me for more examples, so I added more examples.
-->
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
