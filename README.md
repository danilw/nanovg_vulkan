added vulkan build, and cmake files, removed premake4.lua

**Why I made this** - I made it as a "test task for Vulkan related job" a year ago, for me this project has no real use. I would recommend for everyone use [**imgui**](https://github.com/ocornut/imgui) if you look for production-ready Vulkan UI. I will not support or update this project at all, the last update was just to fix bugs and Validation errors.

### Contact: [**Join discord server**](https://discord.gg/JKyqWgt)
___

**2022 update 2** - few people ask me for more examples, so I added more examples:

*Warning - for now only one "frame in flight" supported*. Il update examples and nanovg library latter to add missing support.

`example_vulkan.c` - minimal NanoVG example that use GLFW.

`example_vulkan_min_no_glfw.c` - same as above but not using GLFW, supported Linux and Windows.

[nanovg-vulkan-min-integration-demo](https://github.com/danilw/nanovg-vulkan-min-integration-demo) - repository with minimal RenderPass integration example. Not using any libraries, no GLFW, only NanoVG code and C.

`example_vulkan_glfw_integration.cpp` - this is example code from [Vulkan-tutorial Depth buffering](https://vulkan-tutorial.com/Depth_buffering) modified adding NanoVG integration. Look this commit to see code changes from original C++ tutorial code.
___

**2022 update** - thanks to [**@nidefawl**](https://github.com/nidefawl) [pull request](https://github.com/danilw/nanovg_vulkan/pull/5) with lots of changes:

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

**2021 update** - fixed all errors, this code has no Validation errors and work correctly everywhere.

Thanks to [**@fzwoch**](https://github.com/fzwoch) [commits](https://github.com/danilw/nanovg_vulkan/pull/1) **by default used `TOPOLOGY_TRIANGLE_LIST`**, because `TOPOLOGY_TRIANGLE_FAN` is optional in Vulkan. 

To enable *TOPOLOGY_TRIANGLE_FAN* edit `src/nanovg_vk.h` and set there `#define USE_TOPOLOGY_TRIANGLE_FAN`

Depth order bug on AMD fix by [**@leranger**](https://github.com/leranger) [6ee1009](https://github.com/danilw/nanovg_vulkan/commit/6ee100956134cab2aab67a6a8a7a5bda54c0f9ab).
___

# Build:

```
git clone --recursive https://github.com/danilw/nanovg_vulkan
cd nanovg_vulkan/example
mkdir build
cd build
cmake ../
make
cd ../
./build/example-vk
```

glfw used only to get Vulkan surface and Mouse input.

**Screenshot** of Vulkan version from Linux:

![nvgvk](https://danilw.github.io/GLSL-howto/nvgvk.png)
___
___

*This project is not actively maintained.*

NanoVG
==========

NanoVG is small antialiased vector graphics rendering library for OpenGL. It has lean API modeled after HTML5 canvas API. It is aimed to be a practical and fun toolset for building scalable user interfaces and visualizations.

## Screenshot

![screenshot of some text rendered witht the sample program](/example/screenshot-01.png?raw=true)

Usage
=====

The NanoVG API is modeled loosely on HTML5 canvas API. If you know canvas, you're up to speed with NanoVG in no time.

## Creating drawing context

The drawing context is created using platform specific constructor function. If you're using the OpenGL 2.0 back-end the context is created as follows:
```C
#define NANOVG_GL2_IMPLEMENTATION	// Use GL2 implementation.
#include "nanovg_gl.h"
...
struct NVGcontext* vg = nvgCreateGL2(NVG_ANTIALIAS | NVG_STENCIL_STROKES);
```

The first parameter defines flags for creating the renderer.

- `NVG_ANTIALIAS` means that the renderer adjusts the geometry to include anti-aliasing. If you're using MSAA, you can omit this flags. 
- `NVG_STENCIL_STROKES` means that the render uses better quality rendering for (overlapping) strokes. The quality is mostly visible on wider strokes. If you want speed, you can omit this flag.

Currently there is an OpenGL back-end for NanoVG: [nanovg_gl.h](/src/nanovg_gl.h) for OpenGL 2.0, OpenGL ES 2.0, OpenGL 3.2 core profile and OpenGL ES 3. The implementation can be chosen using a define as in above example. See the header file and examples for further info. 

*NOTE:* The render target you're rendering to must have stencil buffer.

## Drawing shapes with NanoVG

Drawing a simple shape using NanoVG consists of four steps: 1) begin a new shape, 2) define the path to draw, 3) set fill or stroke, 4) and finally fill or stroke the path.

```C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

Calling `nvgBeginPath()` will clear any existing paths and start drawing from blank slate. There are number of number of functions to define the path to draw, such as rectangle, rounded rectangle and ellipse, or you can use the common moveTo, lineTo, bezierTo and arcTo API to compose the paths step by step.

## Understanding Composite Paths

Because of the way the rendering backend is build in NanoVG, drawing a composite path, that is path consisting from multiple paths defining holes and fills, is a bit more involved. NanoVG uses even-odd filling rule and by default the paths are wound in counter clockwise order. Keep that in mind when drawing using the low level draw API. In order to wind one of the predefined shapes as a hole, you should call `nvgPathWinding(vg, NVG_HOLE)`, or `nvgPathWinding(vg, NVG_CW)` _after_ defining the path.

``` C
nvgBeginPath(vg);
nvgRect(vg, 100,100, 120,30);
nvgCircle(vg, 120,120, 5);
nvgPathWinding(vg, NVG_HOLE);	// Mark circle as a hole.
nvgFillColor(vg, nvgRGBA(255,192,0,255));
nvgFill(vg);
```

## Rendering is wrong, what to do?

- make sure you have created NanoVG context using one of the `nvgCreatexxx()` calls
- make sure you have initialised OpenGL with *stencil buffer*
- make sure you have cleared stencil buffer
- make sure all rendering calls happen between `nvgBeginFrame()` and `nvgEndFrame()`
- to enable more checks for OpenGL errors, add `NVG_DEBUG` flag to `nvgCreatexxx()`
- if the problem still persists, please report an issue!

## OpenGL state touched by the backend

The OpenGL back-end touches following states:

When textures are uploaded or updated, the following pixel store is set to defaults: `GL_UNPACK_ALIGNMENT`, `GL_UNPACK_ROW_LENGTH`, `GL_UNPACK_SKIP_PIXELS`, `GL_UNPACK_SKIP_ROWS`. Texture binding is also affected. Texture updates can happen when the user loads images, or when new font glyphs are added. Glyphs are added as needed between calls to  `nvgBeginFrame()` and `nvgEndFrame()`.

The data for the whole frame is buffered and flushed in `nvgEndFrame()`. The following code illustrates the OpenGL state touched by the rendering code:
```C
	glUseProgram(prog);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glEnable(GL_CULL_FACE);
	glCullFace(GL_BACK);
	glFrontFace(GL_CCW);
	glEnable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_SCISSOR_TEST);
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glStencilMask(0xffffffff);
	glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	glStencilFunc(GL_ALWAYS, 0, 0xffffffff);
	glActiveTexture(GL_TEXTURE0);
	glBindBuffer(GL_UNIFORM_BUFFER, buf);
	glBindVertexArray(arr);
	glBindBuffer(GL_ARRAY_BUFFER, buf);
	glBindTexture(GL_TEXTURE_2D, tex);
	glUniformBlockBinding(... , GLNVG_FRAG_BINDING);
```

## API Reference

See the header file [nanovg.h](/src/nanovg.h) for API reference.

## Ports

- [DX11 port](https://github.com/cmaughan/nanovg) by [Chris Maughan](https://github.com/cmaughan)
- [Metal port](https://github.com/ollix/MetalNanoVG) by [Olli Wang](https://github.com/olliwang)
- [bgfx port](https://github.com/bkaradzic/bgfx/tree/master/examples/20-nanovg) by [Branimir Karadžić](https://github.com/bkaradzic)

## Projects using NanoVG

- [Processing API simulation by vinjn](https://github.com/vinjn/island/blob/master/examples/01-processing/sketch2d.h)
- [NanoVG for .NET, C# P/Invoke binding](https://github.com/sbarisic/nanovg_dotnet)

## License
The library is licensed under [zlib license](LICENSE.txt)
Fonts used in examples:
- Roboto licensed under [Apache license](http://www.apache.org/licenses/LICENSE-2.0)
- Entypo licensed under CC BY-SA 4.0.
- Noto Emoji licensed under [SIL Open Font License, Version 1.1](http://scripts.sil.org/cms/scripts/page.php?site_id=nrsi&id=OFL)

## Discussions
[NanoVG mailing list](https://groups.google.com/forum/#!forum/nanovg)

## Links
Uses [stb_truetype](http://nothings.org) (or, optionally, [freetype](http://freetype.org)) for font rendering.
Uses [stb_image](http://nothings.org) for image loading.
