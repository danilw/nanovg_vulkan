// Danil, 2022+ https://github.com/danilw/nanovg_vulkan

// this is Nanovg demo without using any libraries (no GLFW)
// scroll to <main> functions to see implementation 
// Nanovg integration is just single function init_nanovg_vulkan where VKNVGCreateInfo create_info initialized

// glfw replased by minimal code(inilializing vulkan surface(x11 and Win32)), code come from https://github.com/danilw/vulkan-shader-launcher



#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <vulkan/vulkan.h>

#if defined(VK_USE_PLATFORM_XCB_KHR)
#include <unistd.h>
#endif

#ifdef _WIN32
#pragma comment(linker, "/subsystem:windows")
#endif

#include <time.h>

#ifndef DEMO_ANTIALIAS
#   define DEMO_ANTIALIAS 1
#endif
#ifndef DEMO_STENCIL_STROKES
#   define DEMO_STENCIL_STROKES 1
#endif
#ifndef DEMO_VULKAN_VALIDATON_LAYER
#   define DEMO_VULKAN_VALIDATON_LAYER 0
#endif

#include "nanovg.h"
#include "nanovg_vk.h"

#include "demo.h"
#include "perf.h"

#include "vulkan_util.h"



#if defined(VK_USE_PLATFORM_WIN32_KHR)
#include <shellapi.h>
#endif

#if defined(VK_USE_PLATFORM_WIN32_KHR)

#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
#endif

struct app_data_struct{
    int iResolution[2]; //resolution
    int iMouse[2]; //mouse in window, it always updated (not like iMouse on shadertoy)
    int iMouse_lclick[2]; //mouse left click pos (its -[last pos] when left mosue not clicked)
    int iMouse_rclick[2]; //mouse right click pos (its -[last pos] when right mosue not clicked)
    bool iMouse_click[2]; //is mouse button clicked(left/right)
    float iTime; //time
    float iTimeDelta; //time delta
    int iFrame; //frames

    bool pause; //pause clicked
    bool quit; //quit clicked/happend
    bool blowup; // nanovg demo test (space hotkey)
};

#define APP_NAME_STR_LEN 80
struct app_os_window {
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    HINSTANCE connection;
    HWND window;
    POINT minsize;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    Display *display;
    xcb_connection_t *connection;
    xcb_screen_t *screen;
    xcb_window_t xcb_window;
    xcb_intern_atom_reply_t *atom_wm_delete_window;
#endif
    char name[APP_NAME_STR_LEN];
    bool fps_lock; //key pressed event

    bool is_minimized; //window controled events

    struct app_data_struct app_data;
};

struct my_time_struct{
    int msec;
    int sec;
    int min;
    int hour;
    int day;
    int month;
    int year;
};

#if defined(VK_USE_PLATFORM_WIN32_KHR)
void process_msg(MSG* msg, bool* done);
LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
static void app_create_window(struct app_os_window* os_window);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
#include <X11/Xutil.h>
#include <X11/Xlib-xcb.h>
static void app_handle_xcb_event(struct app_os_window *os_window, const xcb_generic_event_t *event);
static void app_init_connection(struct app_os_window *os_window);
static void app_create_xcb_window(struct app_os_window *os_window);
#endif

struct app_os_window os_window;
VkResult vk_init_ext(VkInstance *vk, const char *ext_names[], uint32_t ext_count);
VkResult vk_create_surface(VkInstance vk, VkSurfaceKHR *surface, struct app_os_window *os_window);
VkPhysicalDevice init_vk_gpu(VkInstance instance, VkSurfaceKHR *surface);
void update_params(struct app_data_struct *app_data, bool fps_lock);
void get_local_time(struct my_time_struct *my_time);
float update_fps_delta();
void sleep_ms(int milliseconds);
void FPS_LOCK(int fps);

void init_win_params(struct app_os_window *os_window)
{
    os_window->app_data.iResolution[0] = 1280;
    os_window->app_data.iResolution[1] = 720;
    os_window->app_data.iFrame = 0;
    os_window->app_data.iMouse[0] = 0;
    os_window->app_data.iMouse[1] = 0;
    os_window->app_data.iMouse_click[0] = false;
    os_window->app_data.iMouse_click[1] = false;
    os_window->app_data.iMouse_lclick[0] = 0;
    os_window->app_data.iMouse_lclick[1] = 0;
    os_window->app_data.iTime = 0;
    os_window->app_data.pause = false;
    os_window->app_data.blowup = false;
    os_window->app_data.quit = false;
    os_window->fps_lock = false;
    os_window->is_minimized = false;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    os_window->connection = NULL;
    os_window->window = NULL;
    os_window->minsize.x = 1;
    os_window->minsize.y = 1;
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    os_window->atom_wm_delete_window = NULL;
    os_window->xcb_window = 0;
    os_window->screen = NULL;
    os_window->connection = NULL;
    os_window->display = NULL;
#endif
    strncpy(os_window->name, "Nanovg demo", APP_NAME_STR_LEN);
}



// ----------- nanovg Vulkan related functions

bool resize_event = false;

void prepareFrame(VkDevice device, VkCommandBuffer cmd_buffer, FrameBuffers *fb) {
    VkResult res;

    // Get the index of the next available swapchain image:
    res = vkAcquireNextImageKHR(device, fb->swap_chain, UINT64_MAX,
                                fb->present_complete_semaphore[fb->current_frame],
                                VK_NULL_HANDLE,
                                &fb->current_buffer);
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        resize_event = true;
        res = 0;
        return;
    }
    assert(res == VK_SUCCESS);

    const VkCommandBufferBeginInfo cmd_buf_info = {VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO};
    res = vkBeginCommandBuffer(cmd_buffer, &cmd_buf_info);
    assert(res == VK_SUCCESS);

    VkClearValue clear_values[2];
    clear_values[0].color.float32[0] = 0.3f;
    clear_values[0].color.float32[1] = 0.3f;
    clear_values[0].color.float32[2] = 0.32f;
    clear_values[0].color.float32[3] = 1.0f;
    clear_values[1].depthStencil.depth = 1.0f;
    clear_values[1].depthStencil.stencil = 0;

    VkRenderPassBeginInfo rp_begin;
    rp_begin.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    rp_begin.pNext = NULL;
    rp_begin.renderPass = fb->render_pass;
    rp_begin.framebuffer = fb->framebuffers[fb->current_buffer];
    rp_begin.renderArea.offset.x = 0;
    rp_begin.renderArea.offset.y = 0;
    rp_begin.renderArea.extent.width = fb->buffer_size.width;
    rp_begin.renderArea.extent.height = fb->buffer_size.height;
    rp_begin.clearValueCount = 2;
    rp_begin.pClearValues = clear_values;

    vkCmdBeginRenderPass(cmd_buffer, &rp_begin, VK_SUBPASS_CONTENTS_INLINE);

    VkViewport viewport;
    viewport.width = (float) fb->buffer_size.width;
    viewport.height = (float) fb->buffer_size.height;
    viewport.minDepth = (float) 0.0f;
    viewport.maxDepth = (float) 1.0f;
    viewport.x = (float) rp_begin.renderArea.offset.x;
    viewport.y = (float) rp_begin.renderArea.offset.y;
    vkCmdSetViewport(cmd_buffer, 0, 1, &viewport);

    VkRect2D scissor = rp_begin.renderArea;
    vkCmdSetScissor(cmd_buffer, 0, 1, &scissor);
}

void submitFrame(VkDevice device, VkQueue queue, VkCommandBuffer cmd_buffer, FrameBuffers *fb) {
    VkResult res;

    vkCmdEndRenderPass(cmd_buffer);

    VkImageMemoryBarrier image_barrier = {
            .sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
            .srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
            .dstAccessMask = 0,
            .oldLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
            .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
            .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
            .image = fb->swap_chain_buffers[fb->current_buffer].image,
            .subresourceRange = {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
            },
    };
    vkCmdPipelineBarrier(cmd_buffer,
                         VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
                         VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
                         0,
                         0, NULL,
                         0, NULL,
                         1, &image_barrier);

    vkEndCommandBuffer(cmd_buffer);

    VkPipelineStageFlags pipe_stage_flags = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;

    VkSubmitInfo submit_info = {VK_STRUCTURE_TYPE_SUBMIT_INFO};
    submit_info.pNext = NULL;
    submit_info.waitSemaphoreCount = 1;
    submit_info.pWaitSemaphores = &fb->present_complete_semaphore[fb->current_frame];
    submit_info.pWaitDstStageMask = &pipe_stage_flags;
    submit_info.commandBufferCount = 1;
    submit_info.pCommandBuffers = &cmd_buffer;
    submit_info.signalSemaphoreCount = 1;
    submit_info.pSignalSemaphores = &fb->render_complete_semaphore[fb->current_frame];

    /* Queue the command buffer for execution */
    res = vkQueueSubmit(queue, 1, &submit_info, 0);
    assert(res == VK_SUCCESS);

    /* Now present the image in the window */

    VkPresentInfoKHR present = {VK_STRUCTURE_TYPE_PRESENT_INFO_KHR};
    present.pNext = NULL;
    present.swapchainCount = 1;
    present.pSwapchains = &fb->swap_chain;
    present.pImageIndices = &fb->current_buffer;
    present.waitSemaphoreCount = 1;
    present.pWaitSemaphores = &fb->render_complete_semaphore[fb->current_frame];

    fb->current_frame = fb->current_buffer;

    res = vkQueuePresentKHR(queue, &present);
    if (res == VK_ERROR_OUT_OF_DATE_KHR)
    {
        res = vkQueueWaitIdle(queue);
        resize_event = true;
        res = 0;
        return;
    }
    assert(res == VK_SUCCESS);

    res = vkQueueWaitIdle(queue);
}

void init_nanovg_vulkan(VkPhysicalDevice gpu, VkSurfaceKHR *surface, int winWidth, int winHeight, VkQueue *queue, NVGcontext **vg,
                        FrameBuffers *fb, VkCommandBuffer **cmd_buffer, VulkanDevice **device, PerfGraph *fps, DemoData *data){
    *device = createVulkanDevice(gpu);

    vkGetDeviceQueue((*device)->device, (*device)->graphicsQueueFamilyIndex, 0, queue);
    *fb = createFrameBuffers((*device), *surface, *queue, winWidth, winHeight, 0);

    (*cmd_buffer) = createCmdBuffer((*device)->device, (*device)->commandPool, fb->swapchain_image_count);
    VKNVGCreateInfo create_info = {0};
    create_info.device = (*device)->device;
    create_info.gpu = (*device)->gpu;
    create_info.renderpass = fb->render_pass;
    create_info.cmdBuffer = (*cmd_buffer);
    create_info.swapchainImageCount = fb->swapchain_image_count;
    create_info.currentFrame = &fb->current_frame;

    int flags = 0;
#ifndef NDEBUG
    flags |= NVG_DEBUG; // unused in nanovg_vk
#endif
#if DEMO_ANTIALIAS
    flags |= NVG_ANTIALIAS;
#endif
#if DEMO_STENCIL_STROKES
    flags |= NVG_STENCIL_STROKES;
#endif

    *vg = nvgCreateVk(create_info, flags, *queue);

    if (loadDemoData(*vg, data) == -1)
        exit(-1);

    initGraph(fps, GRAPH_RENDER_FPS, "Frame Time");
}

// -----end of nanovg Vulkan related functions


// --- main functions for Windows OS and Linux X11 (Vulkan sufaces)

#if defined(VK_USE_PLATFORM_WIN32_KHR)

VkInstance instance;
VkSurfaceKHR surface;
VulkanDevice* device;
VkQueue queue;
NVGcontext* vg;
FrameBuffers fb;
VkCommandBuffer* cmd_buffer;
DemoData data;
PerfGraph fps;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR pCmdLine, int nCmdShow)
{
    MSG msg;
    bool done;

    VkResult res;
    int retval = EXIT_FAILURE;
    init_win_params(&os_window);

    os_window.connection = hInstance;

    const char *extension_names[] = {
            VK_KHR_SURFACE_EXTENSION_NAME,
            VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
    };
    res = vk_init_ext(&instance, extension_names, sizeof extension_names / sizeof *extension_names);

    app_create_window(&os_window);
    res = vk_create_surface(instance, &surface, &os_window);
    if (VK_SUCCESS != res)
    {
        printf("CreateSurface failed\n");

        vkDestroyInstance(instance, NULL);

        return retval;
    }

    VkPhysicalDevice gpu = init_vk_gpu(instance, &surface);

    int winWidth = os_window.app_data.iResolution[0];
    int winHeight = os_window.app_data.iResolution[1];
    init_nanovg_vulkan(gpu, &surface, winWidth, winHeight, &queue, &vg, &fb, &cmd_buffer, &device, &fps, &data);

    done = false;
    while (!done)
    {
        PeekMessage(&msg, 0, 0, 0, PM_REMOVE);
        process_msg(&msg, &done);
        RedrawWindow(os_window.window, NULL, NULL, RDW_INTERNALPAINT);
    }

    res = vkQueueWaitIdle(queue);
    assert(res == VK_SUCCESS);

    freeDemoData(vg, &data);
    nvgDeleteVk(vg);
    destroyFrameBuffers(device, &fb, queue);
    vkDestroySurfaceKHR(instance, surface, NULL);
    destroyVulkanDevice(device);


    vkDestroyInstance(instance, NULL);
    free(cmd_buffer);

    return (int)msg.wParam;
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)

int main(int argc, char **argv)
{
    VkResult res;
    int retval = EXIT_FAILURE;
    init_win_params(&os_window);

    srand(time(NULL));
  
    
    VkInstance instance;
    VkSurfaceKHR surface;
    
    const char *extension_names[] = {
      VK_KHR_SURFACE_EXTENSION_NAME,
  #if defined(VK_USE_PLATFORM_WIN32_KHR)
      VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
  #elif defined(VK_USE_PLATFORM_XCB_KHR)
      VK_KHR_XCB_SURFACE_EXTENSION_NAME,
  #endif
    };
    res = vk_init_ext(&instance, extension_names, sizeof extension_names / sizeof *extension_names);

    printf("Init XCB\n");
    app_init_connection(&os_window);
    app_create_xcb_window(&os_window);
    res = vk_create_surface(instance, &surface, &os_window);
    if (VK_SUCCESS != res)
    {
        printf("CreateSurface failed\n");
        
        xcb_destroy_window(os_window.connection, os_window.xcb_window);
        xcb_disconnect(os_window.connection);
        free(os_window.atom_wm_delete_window);
        
        vkDestroyInstance(instance, NULL);
        
        return retval;
    }

  VkPhysicalDevice gpu = init_vk_gpu(instance, &surface);
  
  int winWidth = os_window.app_data.iResolution[0];
  int winHeight = os_window.app_data.iResolution[1];
  VulkanDevice *device;
  VkQueue queue;
  NVGcontext *vg;
  FrameBuffers fb;
  VkCommandBuffer* cmd_buffer;
  DemoData data;
  PerfGraph fps;
  init_nanovg_vulkan(gpu, &surface, winWidth, winHeight, &queue, &vg, &fb, &cmd_buffer, &device, &fps, &data);

    while (!os_window.app_data.quit)
    {
        xcb_generic_event_t *event;

        if (os_window.app_data.pause)
        {
            event = xcb_wait_for_event(os_window.connection);
        }
        else
        {
            event = xcb_poll_for_event(os_window.connection);
        }
        while (event)
        {
            app_handle_xcb_event(&os_window, event);
            free(event);
            event = xcb_poll_for_event(os_window.connection);
        }
        float pxRatio;

        int cwinWidth = os_window.app_data.iResolution[0];
        int cwinHeight = os_window.app_data.iResolution[1];
        if ((resize_event)||(winWidth != cwinWidth || winHeight != cwinHeight)) {
          winWidth = cwinWidth;
          winHeight = cwinHeight;
          destroyFrameBuffers(device, &fb, queue);
          fb = createFrameBuffers(device, surface, queue, winWidth, winHeight, 0);
          resize_event=false;
        }else{
          
          if (!os_window.is_minimized){
            prepareFrame(device->device, cmd_buffer[fb.current_frame], &fb);
            if(resize_event)continue;
          }
          updateGraph(&fps, os_window.app_data.iTimeDelta);
          pxRatio = (float)fb.buffer_size.width / (float)winWidth;

          int mx = os_window.app_data.iMouse[0];
          int my = os_window.app_data.iResolution[1] - os_window.app_data.iMouse[1];
        
          nvgBeginFrame(vg, (float)winWidth, (float)winHeight, pxRatio);
          renderDemo(vg, (float)mx, (float)my, (float)winWidth, (float)winHeight, (float)os_window.app_data.iTime, (int)os_window.app_data.blowup, &data);
          renderGraph(vg, 5, 5, &fps);

          nvgEndFrame(vg);
          
          if (os_window.is_minimized)
          { // I do not delete everything on minimize, only stop rendering
              sleep_ms(10);
          }
          else {
            submitFrame(device->device, queue, cmd_buffer[fb.current_frame], &fb);
          }
          update_params(&os_window.app_data, os_window.fps_lock);
        }
    }
    
    freeDemoData(vg, &data);
    nvgDeleteVk(vg);
    destroyFrameBuffers(device, &fb, queue);
    vkDestroySurfaceKHR(instance, surface, NULL);
    destroyVulkanDevice(device);
    
    xcb_destroy_window(os_window.connection, os_window.xcb_window);
    xcb_disconnect(os_window.connection);
    free(os_window.atom_wm_delete_window);
    
    vkDestroyInstance(instance, NULL);
    
    retval = 0;

    return retval;
}

#endif


// ---- next code is glfw library replacement by minimal functions


void update_params(struct app_data_struct *app_data, bool fps_lock)
{
    float rdelta = 0;
    if (fps_lock)
        FPS_LOCK(30);
    float delta = update_fps_delta();
    if (!app_data->pause)
    {
        app_data->iFrame++;
        app_data->iTime += delta;
    }
    app_data->iTimeDelta = delta;
}


VkResult vk_init_ext(VkInstance *vk, const char *ext_names[], uint32_t ext_count)
{
    VkApplicationInfo app_info = {
            .sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
            .pApplicationName = "Nanovg demo",
            .applicationVersion = 0x010000,
            .pEngineName = "Nanovg demo",
            .engineVersion = 0x010000,
            .apiVersion = VK_API_VERSION_1_0,
    };
    VkInstanceCreateInfo info;

    info = (VkInstanceCreateInfo){
            .sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
            .pApplicationInfo = &app_info,
            .enabledExtensionCount = ext_count,
            .ppEnabledExtensionNames = ext_names,
    };

    return vkCreateInstance(&info, NULL, vk);
}

VkResult vk_create_surface(VkInstance vk, VkSurfaceKHR *surface, struct app_os_window *os_window){
    VkResult res;
#if defined(VK_USE_PLATFORM_WIN32_KHR)
    VkWin32SurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.hinstance = os_window->connection;
    createInfo.hwnd = os_window->window;

    res = vkCreateWin32SurfaceKHR(vk, &createInfo, NULL, surface);
#elif defined(VK_USE_PLATFORM_XCB_KHR)
    VkXcbSurfaceCreateInfoKHR createInfo;
    createInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
    createInfo.pNext = NULL;
    createInfo.flags = 0;
    createInfo.connection = os_window->connection;
    createInfo.window = os_window->xcb_window;
    
    res = vkCreateXcbSurfaceKHR(vk, &createInfo, NULL, surface);
#endif
    return res;
}

// selecting Vulkan GPu with priority to Diiscrete GPU, gpu[idx] is selected gpu
// to select by hands set idx value(0/1/2/etc) by hands
VkPhysicalDevice init_vk_gpu(VkInstance instance, VkSurfaceKHR *surface){
    VkResult res;
    uint32_t gpu_count = 0;

    res = vkEnumeratePhysicalDevices(instance, &gpu_count, NULL);
    if (VK_SUCCESS != res && res != VK_INCOMPLETE) {
        printf("vkEnumeratePhysicalDevices failed %d \n", res);
        exit(-1);
    }
    if (gpu_count < 1){
        printf("No Vulkan device found.\n");
        exit(-1);
    }

    VkPhysicalDevice gpu[32];
    res = vkEnumeratePhysicalDevices(instance, &gpu_count, gpu);
    if (res != VK_SUCCESS && res != VK_INCOMPLETE) {
        printf("vkEnumeratePhysicalDevices failed %d \n", res);
        exit(-1);
    }

    uint32_t idx = 0;
    bool use_idx = false;
    bool discrete_idx = false;
    for (uint32_t i = 0; i < gpu_count && (!discrete_idx); i++)
    {
        uint32_t qfc = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(gpu[i], &qfc, NULL);
        if (qfc < 1)continue;

        VkQueueFamilyProperties *queue_family_properties;
        queue_family_properties = malloc(qfc * sizeof(VkQueueFamilyProperties));

        vkGetPhysicalDeviceQueueFamilyProperties(gpu[i], &qfc, queue_family_properties);

        for (uint32_t j = 0; j < qfc; j++)
        {
            VkBool32 supports_present;
            vkGetPhysicalDeviceSurfaceSupportKHR(gpu[i], j, *surface, &supports_present);

            if ((queue_family_properties[j].queueFlags & VK_QUEUE_GRAPHICS_BIT) && supports_present)
            {

                VkPhysicalDeviceProperties pr;
                vkGetPhysicalDeviceProperties(gpu[i], &pr);
                idx = i;
                use_idx = true;
                if(pr.deviceType==VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU){
                    discrete_idx = true;
                }
                break;
            }
        }
        free(queue_family_properties);
    }
    if (!use_idx){
        printf("Not found suitable queue which supports graphics.\n");
        exit(-1);
    }

    printf("Using GPU device %lu\n", (unsigned long) idx);
    return gpu[idx];
}


#if defined(VK_USE_PLATFORM_WIN32_KHR)

void process_msg(MSG *msg, bool *done){
    static bool mouse_click[2]={false, false}; //click once control
    if (msg->message == WM_QUIT)
    {
        *done = true;
    }
    else {
        TranslateMessage(msg);
        DispatchMessage(msg);
    }
    if (msg->message == WM_KEYDOWN) {
        switch (msg->wParam)
        {
            case VK_SPACE:
                //os_window.app_data.pause = !os_window.app_data.pause;
                os_window.app_data.blowup = !os_window.app_data.blowup;
                break;
            case 0x31: // 1
                break;
            case 0x32: // 2
                os_window.fps_lock = !os_window.fps_lock;
                break;
            default: break;
        }
    }

    if (msg->message == WM_MOUSEMOVE) {
        os_window.app_data.iMouse[0] = (int)LOWORD(msg->lParam);
        os_window.app_data.iMouse[1] = os_window.app_data.iResolution[1] - (int)HIWORD(msg->lParam);
        switch (msg->wParam)
        {
            case WM_LBUTTONDOWN:os_window.app_data.iMouse_click[0] = true; break;
            case WM_MBUTTONDOWN:break;
            case WM_RBUTTONDOWN:os_window.app_data.iMouse_click[1] = true; break;
        }
        switch (msg->wParam)
        {
            case WM_LBUTTONUP:os_window.app_data.iMouse_click[0] = false; break;
            case WM_MBUTTONUP:break;
            case WM_RBUTTONUP:os_window.app_data.iMouse_click[1] = false; break;
        }
    }
    else {

        switch (msg->message)
        {
            case WM_LBUTTONDOWN:os_window.app_data.iMouse_click[0] = true; break;
            case WM_MBUTTONDOWN:break;
            case WM_RBUTTONDOWN:os_window.app_data.iMouse_click[1] = true; break;
        }
        switch (msg->message)
        {
            case WM_LBUTTONUP:os_window.app_data.iMouse_click[0] = false; break;
            case WM_MBUTTONUP:break;
            case WM_RBUTTONUP:os_window.app_data.iMouse_click[1] = false; break;
        }
    }
    if(!os_window.app_data.iMouse_click[0]&&(!mouse_click[0])){

    }else{
        if(os_window.app_data.iMouse_click[0]&&(!mouse_click[0])){
            mouse_click[0]=true;
            os_window.app_data.iMouse_lclick[0]=os_window.app_data.iMouse[0];
            os_window.app_data.iMouse_lclick[1]=os_window.app_data.iMouse[1];
        }else{
            if(!os_window.app_data.iMouse_click[0]&&(mouse_click[0])){
                mouse_click[0]=false;
                os_window.app_data.iMouse_lclick[0]=-abs(os_window.app_data.iMouse_lclick[0]);
                os_window.app_data.iMouse_lclick[1]=-abs(os_window.app_data.iMouse_lclick[1]);
            }
        }
    }
    if(!os_window.app_data.iMouse_click[1]&&(!mouse_click[1])){

    }else{
        if(os_window.app_data.iMouse_click[1]&&(!mouse_click[1])){
            mouse_click[1]=true;
            os_window.app_data.iMouse_rclick[0]=os_window.app_data.iMouse[0];
            os_window.app_data.iMouse_rclick[1]=os_window.app_data.iMouse[1];
        }else{
            if(!os_window.app_data.iMouse_click[1]&&(mouse_click[1])){
                mouse_click[1]=false;
                os_window.app_data.iMouse_rclick[0]=-abs(os_window.app_data.iMouse_rclick[0]);
                os_window.app_data.iMouse_rclick[1]=-abs(os_window.app_data.iMouse_rclick[1]);
            }
        }
    }

}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CLOSE:
            PostQuitMessage(0);
            break;
        case WM_PAINT:

            float pxRatio;

            int winWidth = os_window.app_data.iResolution[0];
            int winHeight = os_window.app_data.iResolution[1];
            if (resize_event) {
                destroyFrameBuffers(device, &fb, queue);
                fb = createFrameBuffers(device, surface, queue, winWidth, winHeight, 0);
                resize_event=false;
            }else{
                if (!os_window.is_minimized){
                    prepareFrame(device->device, cmd_buffer[fb.current_frame], &fb);
                    if(resize_event)break;
                }
                updateGraph(&fps, os_window.app_data.iTimeDelta);
                pxRatio = (float)fb.buffer_size.width / (float)winWidth;

                int mx = os_window.app_data.iMouse[0];
                int my = os_window.app_data.iResolution[1] - os_window.app_data.iMouse[1];

                nvgBeginFrame(vg, (float)winWidth, (float)winHeight, pxRatio);
                renderDemo(vg, (float)mx, (float)my, (float)winWidth, (float)winHeight, (float)os_window.app_data.iTime, (int)os_window.app_data.blowup, &data);
                renderGraph(vg, 5, 5, &fps);

                nvgEndFrame(vg);

                if (os_window.is_minimized)
                { // I do not delete everything on minimize, only stop rendering
                    sleep_ms(10);
                }
                else {
                    submitFrame(device->device, queue, cmd_buffer[fb.current_frame], &fb);
                }
                update_params(&os_window.app_data, os_window.fps_lock);
            }

            if(os_window.app_data.quit) PostQuitMessage(0);

            break;
        case WM_GETMINMAXINFO:
            ((MINMAXINFO *)lParam)->ptMinTrackSize = os_window.minsize;
            return 0;
        case WM_SIZE:
            if (wParam != SIZE_MINIMIZED) {
                int tsize[2]={0};
                tsize[0] = lParam & 0xffff;
                tsize[1] = (lParam & 0xffff0000) >> 16;
                os_window.is_minimized = false;
                if((tsize[0]!=os_window.app_data.iResolution[0])||(tsize[1]!=os_window.app_data.iResolution[1])){
                    os_window.app_data.iResolution[0]=tsize[0];
                    os_window.app_data.iResolution[1]=tsize[1];
                    if((os_window.app_data.iResolution[0]==0)||(os_window.app_data.iResolution[1]==0)){
                        os_window.is_minimized = true;
                    }else{
                        resize_event=true;
                    }
                }
            }
            else{
                if (wParam == SIZE_MINIMIZED){
                    os_window.is_minimized = true;
                }
            }
            break;
        default:
            break;
    }
    return (DefWindowProc(hWnd, uMsg, wParam, lParam));
}

static void app_create_window(struct app_os_window *os_window) {
    WNDCLASSEX win_class;

    win_class.cbSize = sizeof(WNDCLASSEX);
    win_class.style = CS_HREDRAW | CS_VREDRAW;
    win_class.lpfnWndProc = WndProc;
    win_class.cbClsExtra = 0;
    win_class.cbWndExtra = 0;
    win_class.hInstance = os_window->connection;
    win_class.hIcon = LoadIcon(NULL, IDI_APPLICATION);
    win_class.hCursor = LoadCursor(NULL, IDC_ARROW);
    win_class.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
    win_class.lpszMenuName = NULL;
    win_class.lpszClassName = os_window->name;
    win_class.hIconSm = LoadIcon(NULL, IDI_WINLOGO);
    if (!RegisterClassEx(&win_class)) {
        printf("Unexpected error trying to start the application!\n");
        fflush(stdout);
        exit(1);
    }
    RECT wr = { 0, 0, os_window->app_data.iResolution[0], os_window->app_data.iResolution[1] };
    AdjustWindowRect(&wr, WS_OVERLAPPEDWINDOW, FALSE);
    os_window->window = CreateWindowEx(0,
                                       os_window->name,            // class name
                                       os_window->name,            // app name
                                       WS_OVERLAPPEDWINDOW |  // window style
                                       WS_VISIBLE | WS_SYSMENU,
                                       100, 100,            // x/y coords
                                       wr.right - wr.left,  // width
                                       wr.bottom - wr.top,  // height
                                       NULL,                // handle to parent
                                       NULL,                // handle to menu
                                       os_window->connection,    // hInstance
                                       NULL);               // no extra parameters
    if (!os_window->window) {
        printf("Cannot create a window in which to draw!\n");
        fflush(stdout);
        exit(1);
    }
    os_window->minsize.x = GetSystemMetrics(SM_CXMINTRACK);
    os_window->minsize.y = GetSystemMetrics(SM_CYMINTRACK) + 1;
}

#elif defined(VK_USE_PLATFORM_XCB_KHR)
static void print_modifiers(uint32_t mask)
{
    const char **mod, *mods[] = {
        "Shift", "Lock", "Ctrl", "Alt",
        "Mod2", "Mod3", "Mod4", "Mod5",
        "Button1", "Button2", "Button3", "Button4", "Button5"
    };
    printf("Modifier mask: ");
    for (mod = mods; mask; mask >>= 1, mod++)
        if (mask & 1)
            printf("%s ", *mod);
    putchar('\n');
}

static void app_handle_xcb_event(struct app_os_window *os_window, const xcb_generic_event_t *event) {
    uint8_t event_code = event->response_type & 0x7f;
    switch (event_code) {
    case XCB_EXPOSE:
        break;
    case XCB_CLIENT_MESSAGE:
        if ((*(xcb_client_message_event_t *)event).data.data32[0] == (*os_window->atom_wm_delete_window).atom) {
            os_window->app_data.quit = true;
        }
        break;
    case XCB_KEY_RELEASE: {
        const xcb_key_release_event_t *key = (const xcb_key_release_event_t *)event;
        switch (key->detail) {
        case 0x9:  // Escape
            os_window->app_data.quit = true;
            break;
        case 0x71:  // left arrow key
            break;
        case 0x72:  // right arrow key
            break;
        case 0x41:  // space bar
            //os_window->app_data.pause = !os_window->app_data.pause;
            os_window->app_data.blowup = !os_window->app_data.blowup;
            break;
        case 0xa: //1
            break;
        case 0xb: //2
            os_window->fps_lock = !os_window->fps_lock;
            break;
        }
    } break;
    case XCB_MOTION_NOTIFY: {
        const xcb_motion_notify_event_t *ev = (const xcb_motion_notify_event_t *)event;
        //printf ("Mouse moved in window %ld, at coordinates (%d,%d)\n",ev->event, ev->event_x, ev->event_y);
        os_window->app_data.iMouse[0] = ev->event_x;
        os_window->app_data.iMouse[1] = os_window->app_data.iResolution[1] - ev->event_y;
    } break;
    case XCB_BUTTON_PRESS: {
        const xcb_button_press_event_t *ev = (const xcb_button_press_event_t *)event;
        //print_modifiers(ev->state);
        switch (ev->detail) {
        case 1:
            os_window->app_data.iMouse_click[0] = true;
            os_window->app_data.iMouse_lclick[0] = ev->event_x;
            os_window->app_data.iMouse_lclick[1] = os_window->app_data.iResolution[1] - ev->event_y;
            break;
        case 3:
            os_window->app_data.iMouse_click[1] = true;
            os_window->app_data.iMouse_rclick[0] = ev->event_x;
            os_window->app_data.iMouse_rclick[1] = os_window->app_data.iResolution[1] - ev->event_y;
            break;
        case 4:
            /*printf ("Wheel Button up in window %ld, at coordinates (%d,%d)\n",
            ev->event, ev->event_x, ev->event_y);*/
            break;
        case 5:
            /*printf ("Wheel Button down in window %ld, at coordinates (%d,%d)\n",
            ev->event, ev->event_x, ev->event_y);*/
            break;
        //default:
            /*printf ("Button %d pressed in window %ld, at coordinates (%d,%d)\n",
            ev->detail, ev->event, ev->event_x, ev->event_y);*/
        }
    } break;
    case XCB_BUTTON_RELEASE: {
        const xcb_button_release_event_t *ev = (const xcb_button_release_event_t *)event;
        switch (ev->detail) {
        case 1:
            os_window->app_data.iMouse_click[0] = false;
            os_window->app_data.iMouse_lclick[0] = -os_window->app_data.iMouse_lclick[0];
            os_window->app_data.iMouse_lclick[1] = -os_window->app_data.iMouse_lclick[1];
            break;
        case 3:
            os_window->app_data.iMouse_click[1] = false;
            os_window->app_data.iMouse_rclick[0] = -os_window->app_data.iMouse_rclick[0];
            os_window->app_data.iMouse_rclick[1] = -os_window->app_data.iMouse_rclick[1];
            break;
        }
        /*print_modifiers(ev->state);

        printf ("Button %d released in window %ld, at coordinates (%d,%d)\n",
        ev->detail, ev->event, ev->event_x, ev->event_y);*/
    } break;

    case XCB_CONFIGURE_NOTIFY: {
        const xcb_configure_notify_event_t *cfg = (const xcb_configure_notify_event_t *)event;
        if ((os_window->app_data.iResolution[0] != cfg->width) || (os_window->app_data.iResolution[1] != cfg->height)) {
            os_window->is_minimized = false;
            os_window->app_data.iResolution[0] = cfg->width;
            os_window->app_data.iResolution[1] = cfg->height;
            if((os_window->app_data.iResolution[0]==0)||(os_window->app_data.iResolution[1]==0)){
                os_window->is_minimized = true;
            }else{
                resize_event=true;
            }
        }
    } break;
    default:
        break;
    }
}

static void app_init_connection(struct app_os_window *os_window) {
    const xcb_setup_t *setup;
    xcb_screen_iterator_t iter;
    int scr;

    const char *display_envar = getenv("DISPLAY");
    if (display_envar == NULL || display_envar[0] == '\0') {
        printf("Environment variable DISPLAY requires a valid value.\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    os_window->connection = xcb_connect(NULL, &scr);
    if (xcb_connection_has_error(os_window->connection) > 0) {
        printf("Cannot find a compatible Vulkan installable client driver (ICD).\nExiting ...\n");
        fflush(stdout);
        exit(1);
    }

    setup = xcb_get_setup(os_window->connection);
    iter = xcb_setup_roots_iterator(setup);
    while (scr-- > 0) xcb_screen_next(&iter);

    os_window->screen = iter.data;
}

static void app_create_xcb_window(struct app_os_window *os_window) {
    uint32_t value_mask, value_list[32];
    
    os_window->xcb_window = xcb_generate_id(os_window->connection);
    
    value_mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
    value_list[0] = os_window->screen->black_pixel;
    
    value_list[1] = XCB_EVENT_MASK_EXPOSURE | XCB_EVENT_MASK_BUTTON_PRESS | XCB_EVENT_MASK_BUTTON_RELEASE | XCB_EVENT_MASK_POINTER_MOTION |
        XCB_EVENT_MASK_KEY_PRESS | XCB_EVENT_MASK_KEY_RELEASE | XCB_EVENT_MASK_STRUCTURE_NOTIFY;

    xcb_create_window(os_window->connection, XCB_COPY_FROM_PARENT, os_window->xcb_window, os_window->screen->root, 0, 0, os_window->app_data.iResolution[0], os_window->app_data.iResolution[1],
        0, XCB_WINDOW_CLASS_INPUT_OUTPUT, os_window->screen->root_visual, value_mask, value_list);

    xcb_intern_atom_cookie_t cookie = xcb_intern_atom(os_window->connection, 1, 12, "WM_PROTOCOLS");
    xcb_intern_atom_reply_t *reply = xcb_intern_atom_reply(os_window->connection, cookie, 0);

    xcb_intern_atom_cookie_t cookie2 = xcb_intern_atom(os_window->connection, 0, 16, "WM_DELETE_WINDOW");
    os_window->atom_wm_delete_window = xcb_intern_atom_reply(os_window->connection, cookie2, 0);
    
    xcb_change_property (os_window->connection, XCB_PROP_MODE_REPLACE, os_window->xcb_window,
                       XCB_ATOM_WM_NAME, XCB_ATOM_STRING, 8,
                       strlen (os_window->name), os_window->name);
    
    xcb_change_property (os_window->connection, XCB_PROP_MODE_REPLACE, os_window->xcb_window,
                       XCB_ATOM_WM_ICON_NAME, XCB_ATOM_STRING, 8,
                       strlen(os_window->name), os_window->name);
    
    xcb_change_property(os_window->connection, XCB_PROP_MODE_REPLACE, os_window->xcb_window, (*reply).atom, 4, 32, 1,
        &(*os_window->atom_wm_delete_window).atom);
    free(reply);

    xcb_map_window(os_window->connection, os_window->xcb_window);

    const uint32_t coords[] = { 100, 100 };
    xcb_configure_window(os_window->connection, os_window->xcb_window, XCB_CONFIG_WINDOW_X | XCB_CONFIG_WINDOW_Y, coords);
    xcb_flush(os_window->connection);
}
#endif

#if defined(_WIN32)

#include <windows.h>
#define CLOCK_MONOTONIC_RAW 0
#define BILLION                             (1E9)

int clock_gettime(int dummy, struct timespec *ct)
{
    static BOOL g_first_time = 1;
    static LARGE_INTEGER g_counts_per_sec;
    LARGE_INTEGER count;
    if (g_first_time)
    {
        g_first_time = 0;
        if (0 == QueryPerformanceFrequency(&g_counts_per_sec))
        {
            g_counts_per_sec.QuadPart = 0;
        }
    }
    if ((NULL == ct) || (g_counts_per_sec.QuadPart <= 0) ||
        (0 == QueryPerformanceCounter(&count)))
    {
        return -1;
    }
    ct->tv_sec = count.QuadPart / g_counts_per_sec.QuadPart;
    ct->tv_nsec = ((count.QuadPart % g_counts_per_sec.QuadPart) * BILLION) / g_counts_per_sec.QuadPart;

    return 0;
}

void get_local_time(struct my_time_struct *my_time){
    SYSTEMTIME lt = {0};
    GetLocalTime(&lt);
    my_time->msec=lt.wMilliseconds;
    my_time->sec=lt.wSecond;
    my_time->min=lt.wMinute;
    my_time->hour=lt.wHour;
    my_time->day=lt.wDay;
    my_time->month=lt.wMonth;
    my_time->year=lt.wYear;
}


#else

#include <sys/time.h>

void get_local_time(struct my_time_struct *my_time){
    struct timeval te; 
    gettimeofday(&te, NULL);
    time_t T= time(NULL);
    struct tm tm = *localtime(&T);
    long long milliseconds = te.tv_sec*1000LL + te.tv_usec/1000;
    my_time->msec=(int)(milliseconds%(1000));
  my_time->sec=tm.tm_sec;
  my_time->min=tm.tm_min;
  my_time->hour=tm.tm_hour;
  my_time->day=tm.tm_mday;
  my_time->month=tm.tm_mon+1;
  my_time->year=tm.tm_year+1900;
    return;
}

#endif

double get_time_ticks(){
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    double ticks = (ts.tv_sec * 1000) + (ts.tv_nsec) / 1000000.0;
    return ticks;
}

float update_fps_delta(){
    static double before_time=0;
    if((int)before_time==0)before_time=get_time_ticks();

    double now_time = get_time_ticks();

    if ((long)before_time==(long)now_time) { //this happend on IMMEDIATE and MAILBOX
        return 1.0/9999.0;
    }
    double tdelta=(now_time-before_time);
    if(tdelta<=0)tdelta=1;
    float delta=(float)tdelta/1000.0;
    if(delta>1){
        delta=1;
    }
    if(delta<=0){
        delta=1.0/9999.0;
    }
    before_time=now_time;
    return delta;
}

// cross-platform sleep function
void sleep_ms(int milliseconds)
{
#ifdef WIN32
    Sleep(milliseconds);
#else
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
#endif
}

void FPS_LOCK(int fps){
    static double before_time=0;
    if(before_time==0)before_time=get_time_ticks();
    double now_time=get_time_ticks();
    if(before_time==now_time)return;
    if(fps<=0)fps=1;
    double wdelta=((double)(1.0/(double)fps)*(double)1000.0);
    double tdelta=(now_time-before_time);
    if(tdelta<=0)tdelta=1;
    double rdelta=wdelta-tdelta;
    if(rdelta<=0){
        before_time=now_time;
        return;
    }
    sleep_ms((int)rdelta);
    before_time=get_time_ticks();

    return;

}



