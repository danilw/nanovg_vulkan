cmake_minimum_required(VERSION 3.1)

project(nanovg_example)

set (BUILD_SHARED_LIBS OFF)
set( GLFW_BUILD_DOCS OFF CACHE BOOL ""  )
set( GLFW_BUILD_TESTS OFF CACHE BOOL ""  )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL ""  )
set( GLFW_BUILD_INSTALL OFF CACHE BOOL ""  )

add_subdirectory(../glfw glfw)

option(BUILD_USE_GLAD "Use glad instead of glew" ON)

option(BUILD_EXAMPLE_FBO "Build FBO Example (GL3)" OFF)
option(BUILD_EXAMPLE_GL2 "Build Example using OpenGL2" OFF)
option(BUILD_EXAMPLE_GL3 "Build Example using OpenGL3" ON)

option(BUILD_EXAMPLE_VULKAN "Build Example using Vulkan GLFW" ON)
option(BUILD_EXAMPLE_VULKAN_MIN "Build minimal Example using Vulkan without GLFW" ON)
option(BUILD_EXAMPLE_VULKAN_GLFW_INTEGRATION "Build render pass integration Example using GLFW" ON)

option(DEMO_ANTIALIAS "Use NVG_ANTIALIAS" ON)
option(DEMO_MSAA "Use MSAA instead of NVG_ANTIALIAS" OFF)
option(DEMO_STENCIL_STROKES "NVG_STENCIL_STROKES" ON)

add_compile_definitions(DEMO_MSAA=$<BOOL:${DEMO_MSAA}>)
add_compile_definitions(DEMO_ANTIALIAS=$<BOOL:${DEMO_ANTIALIAS}>)
add_compile_definitions(DEMO_STENCIL_STROKES=$<BOOL:${DEMO_STENCIL_STROKES}>)

set(COMMON_INCLUDE ../src)

set(COMMON_SRC
    ../src/fontstash.h
    ../src/nanovg.c
    ../src/nanovg.h
    ../src/stb_image.h
    ../src/stb_truetype.h
    ../src/nanovg.c
    demo.c
    perf.c
)


if (BUILD_EXAMPLE_VULKAN)

    find_package(Vulkan)

    if (Vulkan_FOUND)
        message(STATUS "Vulkan found ${Vulkan_LIBRARIES}")
    else()
        message(FATAL "Vulkan not found")
    endif()

    add_executable(
        example-vk
        ${COMMON_SRC}
        ../src/nanovg_vk.h
        example_vulkan.c
        vulkan_util.h
    )

    target_include_directories(example-vk PRIVATE ${COMMON_INCLUDE} ${Vulkan_INCLUDE_DIR})
    target_compile_definitions(example-vk PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
    target_link_libraries(example-vk PRIVATE glfw ${Vulkan_LIBRARY})
    
    if (BUILD_EXAMPLE_VULKAN_MIN)
      add_executable(
          example-vk_min_no_glfw
          ${COMMON_SRC}
          ../src/nanovg_vk.h
          example_vulkan_min_no_glfw.c
          vulkan_util.h
      )
      
      if("${CMAKE_C_COMPILER_ID}" STREQUAL "MSVC")
        add_definitions(-DVK_USE_PLATFORM_WIN32_KHR )
        target_include_directories(example-vk_min_no_glfw PRIVATE ${COMMON_INCLUDE} ${Vulkan_INCLUDE_DIR})
        target_compile_definitions(example-vk_min_no_glfw PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
        target_link_libraries(example-vk_min_no_glfw PRIVATE ${Vulkan_LIBRARY})
      elseif("${CMAKE_C_COMPILER_ID}" STREQUAL "GNU")
        add_definitions(-DVK_USE_PLATFORM_XCB_KHR -O3)
        target_include_directories(example-vk_min_no_glfw PRIVATE ${COMMON_INCLUDE} ${Vulkan_INCLUDE_DIR})
        target_compile_definitions(example-vk_min_no_glfw PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
        target_link_libraries(example-vk_min_no_glfw PRIVATE m xcb ${Vulkan_LIBRARY})
      else()
        message(WARNING "Unknown compiler '${CMAKE_C_COMPILER_ID}'!")
      endif()
    endif()
    
    if (BUILD_EXAMPLE_VULKAN_GLFW_INTEGRATION)
      add_executable(
          example-vk_glfw_integration
          ${COMMON_SRC}
          ../src/nanovg_vk.h
          example_vulkan_glfw_integration.cpp
          vulkan_util.h
      )
      
      set(VK_TUT_INCLUDE 
        files_example_vulkan_glfw_integration/glm
      )
      
      target_include_directories(example-vk_glfw_integration PRIVATE ${COMMON_INCLUDE} ${VK_TUT_INCLUDE} ${Vulkan_INCLUDE_DIR})
      target_compile_definitions(example-vk_glfw_integration PRIVATE NANOVG_VULKAN_IMPLEMENTATION)
      target_link_libraries(example-vk_glfw_integration PRIVATE glfw ${Vulkan_LIBRARY})
      
    endif()
    
endif()

set(GL_EXAMPLE_SRC
  ${COMMON_SRC}
  ../src/nanovg_gl.h
  ../src/nanovg_gl_utils.h
)
set(GL_EXAMPLE_LIBS glfw)
set(GL_EXAMPLE_INCLUDES ${COMMON_INCLUDE})

if (BUILD_EXAMPLE_GL3 OR BUILD_EXAMPLE_GL2)
    if (BUILD_USE_GLAD)
      set(GL_EXAMPLE_DEFS NANOVG_GLAD)
      list(APPEND GL_EXAMPLE_SRC ../glad-3.2-compat/src/glad.c)
      list(APPEND GL_EXAMPLE_INCLUDES ../glad-3.2-compat/include)
    else()
      # Build glew
      set( ONLY_LIBS ON CACHE BOOL ""  )
      set( glew-cmake_BUILD_SHARED OFF CACHE BOOL ""  )
      set( glew-cmake_BUILD_STATIC ON CACHE BOOL ""  )
      set(GL_EXAMPLE_DEFS NANVG_GLEW)
      add_subdirectory(glew-cmake glew)
      list(APPEND GL_EXAMPLE_LIBS libglew_static)
    endif()
endif()

if (BUILD_EXAMPLE_GL3)
  add_executable(
      example-gl3
      example_gl3.c
      ${GL_EXAMPLE_SRC}
  )
  target_include_directories(example-gl3 PRIVATE ${GL_EXAMPLE_INCLUDES})
  target_compile_definitions(example-gl3 PRIVATE ${GL_EXAMPLE_DEFS})
  target_link_libraries(example-gl3 PRIVATE ${GL_EXAMPLE_LIBS})
endif()

if (BUILD_EXAMPLE_GL2)
  add_executable(
      example-gl2
      example_gl2.c
      ${GL_EXAMPLE_SRC}
  )
  target_include_directories(example-gl2 PRIVATE ${GL_EXAMPLE_INCLUDES})
  target_compile_definitions(example-gl2 PRIVATE ${GL_EXAMPLE_DEFS})
  target_link_libraries(example-gl2 PRIVATE ${GL_EXAMPLE_LIBS})
endif()

if (BUILD_EXAMPLE_FBO)
  add_executable(
      example-fbo
      example_fbo.c
      ${GL_EXAMPLE_SRC}
  )
  target_include_directories(example-fbo PRIVATE ${GL_EXAMPLE_INCLUDES})
  target_compile_definitions(example-fbo PRIVATE ${GL_EXAMPLE_DEFS})
  target_link_libraries(example-fbo PRIVATE ${GL_EXAMPLE_LIBS})
endif()

if (MSVC)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS )
    set_source_files_properties(
        example_vulkan.c
        example_vulkan_min_no_glfw.c
        example_vulkan_glfw_integration.cpp
        example_gl3.c
        example_gl2.c
        example_fbo.c
        demo.c
        PROPERTIES COMPILE_FLAGS
        /wd4244
    )
    set(BUILD_EXE_POSTFIX "-$<LOWER_CASE:${CMAKE_CXX_COMPILER_ID}-$<CONFIG>>" CACHE STRING "Append to executable name")

    set(ALL_TARGETS example-vk example-vk_min_no_glfw example-vk_glfw_integration example-gl3 example-gl2)
    foreach(TARGET_NAME ${ALL_TARGETS})
      if (TARGET ${TARGET_NAME})
        set_property(TARGET ${TARGET_NAME} PROPERTY OUTPUT_NAME ${TARGET_NAME}${BUILD_EXE_POSTFIX})
        set_property(TARGET ${TARGET_NAME} PROPERTY RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}$<0:...>)
      endif()
    endforeach()
endif()

set(CMAKE_VERBOSE_MAKEFILE ON)
