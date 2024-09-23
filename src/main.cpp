#include "MinWin.h"
#include "Window.h"
#include <vulkan/vulkan.hpp>
#ifdef _WIN32
#include <vulkan/vulkan_win32.h>
#endif
#include <stdio.h>
#include <stdint.h>
#include <vector>
#include "3dmath.h"
#include <chrono>
#include <unordered_map>
#include <map>

#include "image.h"

using namespace std;

int main();

#ifndef DEBUG
int WinMain(
  HINSTANCE hInstance,
  HINSTANCE hPrevInstance,
  LPSTR     lpCmdLine,
  int       nShowCmd
){
    return main();
}
#endif

#ifdef DEBUG
    #include <fstream>
    #include <ostream>
#endif

#include "assets.h"

#include "loader.h"
#include "Game.h"

#include "common.h"

#define UNWRAPPER_INNER(x) DEBUG_MSG("%s", #x)
#define UNWRAPPER(x) UNWRAPPER_INNER(x)

#ifdef DEBUG
typedef decltype(update_game) update_game_type;
static update_game_type* update_game_ptr;

typedef decltype(init_game) init_game_type;
static init_game_type* init_game_ptr;

void reload_game_dll(Game* gameIN);

std::string gameLoadLibName;
static void* gameDLL;

long long get_timestamp(const char* file){
  struct stat file_stat = {};
  stat(file, &file_stat);
  return file_stat.st_mtime;
}

bool platform_free_dynamic_library(void* dll)
{
  BOOL freeResult = FreeLibrary((HMODULE)dll);

  return (bool)freeResult;
}

void* platform_load_dynamic_library(const char* dll)
{
  HMODULE result = LoadLibraryA(dll);

  return result;
}

double platform_getTime(){
    // Get the number of milliseconds since the system started
    ULONGLONG currentTime = GetTickCount64();

    // Convert milliseconds to seconds and return
    return currentTime / 1000.0;
}

bool copy_file(const char* fileName, const char* outputName) {
    std::ifstream src(fileName, std::ios::binary);
    if (!src) {
        DEBUG_MSG("Error opening source file: %s\n",fileName);
        return false;
    }

    std::ofstream dst(outputName, std::ios::binary);
    if (!dst) {
        DEBUG_MSG("Error opening destination file: %s\n",outputName);
        return false;
    }

    dst << src.rdbuf();

    if (!dst) {
        DEBUG_MSG("Error writing to destination file: %s\n",outputName);
        return false;
    }

    return true;
}

void* platform_load_dynamic_function(void* dll, const char* funName)
{
  FARPROC proc = GetProcAddress((HMODULE)dll, funName);

  return (void*)proc;
}

void update_game(double dt, Game* gameIN){
    update_game_ptr(dt, gameIN);
}

void init_game(Game* gameIN){
    init_game_ptr(gameIN);
}


#define gameLibName "game.dll"
void reload_game_dll(Game* gameIN)
{
    static long long lastEditTimestampGameDLL;

    long long currentTimestampGameDLL = get_timestamp(gameLibName);
    if(currentTimestampGameDLL > lastEditTimestampGameDLL)
    {
        if(gameDLL)
        {
            bool freeResult = platform_free_dynamic_library(gameDLL);
            gameDLL = nullptr;
        }

        while(!copy_file(gameLibName, (char*)gameLoadLibName.c_str()))
        {
            Sleep(10);
        }
        gameDLL = platform_load_dynamic_library((char*)gameLoadLibName.c_str());
        update_game_ptr = (update_game_type*)platform_load_dynamic_function(gameDLL, "update_game");
        init_game_ptr = (init_game_type*)platform_load_dynamic_function(gameDLL, "init_game");
        lastEditTimestampGameDLL = currentTimestampGameDLL;


        init_game(gameIN);
    }
}
#endif

struct App;

App* app = nullptr;

struct Vertex{
    vec3 pos;
    vec2 uv;
};

struct Mesh{
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;
};

struct MeshView{
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    size_t size;

    MeshView(App* app, Mesh mesh);
    MeshView(){
        size = 0;
    };
};

struct Texture{
    std::pair<VkImage, VkDeviceMemory> image;
    VkImageView view;
    Texture(){
        view = nullptr;
    }
    Texture(std::pair<VkImage, VkDeviceMemory> image, VkImageView view): image(image), view(view) {}
};

struct App{

App(Window* window): window(window){
    window->resizeWindow = App::resizeWindowInner;
    window->app = this;
}

Window* window;
MSG msg;

void HandleWindowEvents(){
    while(PeekMessageA(&msg,NULL,0,0,PM_REMOVE)){
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
}


static void* VKAPI_PTR MyPFN_vkAllocationFunction(
    void* pUserData, 
    size_t size, 
    size_t alignment, 
    VkSystemAllocationScope allocationScope
) {
    // Allocate memory using malloc
    void* ptr = malloc(size);

    // Return the allocated memory pointer
    return ptr;
}

static void* VKAPI_PTR MyPFN_vkReallocationFunction(
    void* pUserData, 
    void* pOriginal, 
    size_t size, 
    size_t alignment, 
    VkSystemAllocationScope allocationScope
) {
    // Reallocate memory using realloc
    void* ptr = realloc(pOriginal, size);

    // Return the reallocated memory pointer
    return ptr;
}

static void VKAPI_PTR MyPFN_vkFreeFunction(
    void* pUserData, 
    void* pMemory
) {
    // Free the allocated memory
    free(pMemory);
}

static void VKAPI_PTR MyPFN_vkInternalAllocationNotification(
    void* pUserData, 
    size_t size, 
    VkInternalAllocationType allocationType, 
    VkSystemAllocationScope allocationScope
) {
    // This function is meant to notify about internal allocations.
    // You can implement logging or tracking of memory allocations here.
    // Example: Log the allocation (this is just a placeholder implementation).
    DEBUG_MSG("Vulkan internal allocation of size %zu occurred. Allocation type: %d, Scope: %d\n", 
           size, allocationType, allocationScope);
    // You can also use pUserData for any application-specific context or tracking.
}

static void VKAPI_PTR MyPFN_vkInternalFreeNotification(
    void* pUserData, 
    size_t size, 
    VkInternalAllocationType allocationType, 
    VkSystemAllocationScope allocationScope
) {
    // This function is meant to notify about internal memory deallocations.
    // You can implement logging or tracking of memory deallocations here.
    // Example: Log the deallocation (this is just a placeholder implementation).
    DEBUG_MSG("Vulkan internal free of size %zu occurred. Allocation type: %d, Scope: %d\n", 
           size, allocationType, allocationScope);
    // You can also use pUserData for any application-specific context or tracking.
}

VkAllocationCallbacks allocator;

void createAllocator(){
    allocator.pUserData = nullptr;
    allocator.pfnAllocation = MyPFN_vkAllocationFunction;
    allocator.pfnReallocation = MyPFN_vkReallocationFunction;
    allocator.pfnFree = MyPFN_vkFreeFunction;
    allocator.pfnInternalAllocation = MyPFN_vkInternalAllocationNotification;
    allocator.pfnInternalFree = MyPFN_vkInternalFreeNotification;
}

#ifdef DEBUG
static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT messageType,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData) {

    // Print debug messages
    DEBUG_MSG("%s\n", pCallbackData->pMessage);

    // Optionally, return VK_FALSE if you want to stop Vulkan from continuing after the message.
    // VK_TRUE means the message is handled, and Vulkan can continue as usual.
    return VK_FALSE;
}
#endif

VkInstance instance;
void initializeVulkan(){
    VkApplicationInfo appInfo = {};
    appInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pNext = nullptr;
    appInfo.pApplicationName = "Baller";
    appInfo.applicationVersion = 1;
    appInfo.pEngineName = "Creamer";
    appInfo.engineVersion = 1;
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo instanceInfo = {};
    instanceInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    instanceInfo.pNext = nullptr;
    instanceInfo.flags = 0;
    instanceInfo.pApplicationInfo = &appInfo;
#ifndef DEBUG
    instanceInfo.enabledLayerCount = 0;
    instanceInfo.ppEnabledLayerNames = nullptr;
    const char* extensions[] = {
        "VK_KHR_surface", "VK_KHR_win32_surface"
    };
    instanceInfo.enabledExtensionCount = 2;
    instanceInfo.ppEnabledExtensionNames = extensions;
#else
    const char* INFOvalidationLayers[] = {
        "VK_LAYER_KHRONOS_validation"
    };
    instanceInfo.enabledLayerCount = 1;
    instanceInfo.ppEnabledLayerNames = INFOvalidationLayers;
    const char* INFOextensions[] = {
        "VK_KHR_surface", "VK_KHR_win32_surface", VK_EXT_DEBUG_UTILS_EXTENSION_NAME
    };
    instanceInfo.enabledExtensionCount = 3;
    instanceInfo.ppEnabledExtensionNames = INFOextensions;
#endif

    VkResult result = vkCreateInstance(&instanceInfo, &allocator, &instance);

    // Handle vkCreateInstance result
    if (result != VK_SUCCESS) {
        // Handle error (e.g., print an error message)
        DEBUG_MSG(stderr, "Failed to create Vulkan instance: %d\n", result);
        exit(-1);
    }

#ifdef DEBUG
    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
    debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                       VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                   VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    debugCreateInfo.pfnUserCallback = debugCallback;

    VkDebugUtilsMessengerEXT debugMessenger;

    PFN_vkCreateDebugUtilsMessengerEXT functionHandle = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");

    functionHandle(instance, &debugCreateInfo, &allocator, &debugMessenger);
#endif
}

VkPhysicalDevice phyDevice;
VkPhysicalDeviceMemoryProperties memoryProperties;
void chooseDevice(){
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(instance,&deviceCount,nullptr);
    if(deviceCount == 0){
        DEBUG_MSG("no devices suitable for vulkan\n");
        exit(-1);
    }
    vector<VkPhysicalDevice> physicalDevices;
    physicalDevices.resize(deviceCount);
    vkEnumeratePhysicalDevices(instance,&deviceCount,physicalDevices.data());


    DEBUG_MSG("Devices:\n");
    VkPhysicalDevice* chosenDevice = nullptr;
    for(auto& device: physicalDevices){
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(device, &properties);

        DEBUG_MSG("- %s\n", properties.deviceName);
        if(properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) chosenDevice = &device;
    }
    if(chosenDevice == nullptr){
        chosenDevice = &physicalDevices[0];
    }
    #ifdef DEBUG
        VkPhysicalDeviceProperties properties;
        vkGetPhysicalDeviceProperties(*chosenDevice, &properties);

        DEBUG_MSG("Chosen Device: %s\n", properties.deviceName);
    #endif
    phyDevice = *chosenDevice;
    vkGetPhysicalDeviceMemoryProperties(phyDevice, &memoryProperties);
}

VkDevice device;
VkQueue graphicsQueue;
VkQueue presentQueue;
void getDevice() {
    uint32_t familyQueueCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &familyQueueCount, nullptr);
    if (familyQueueCount == 0) {
        DEBUG_MSG("No queue families found on the host device\n");
        exit(-1);
    }

    // Retrieve queue family properties
    std::vector<VkQueueFamilyProperties> familyProperties(familyQueueCount);
    vkGetPhysicalDeviceQueueFamilyProperties(phyDevice, &familyQueueCount, familyProperties.data());

    // Variables to store the selected queue family indices
    int graphicsQueueFamilyIndex = -1;
    int presentQueueFamilyIndex = -1;

    // Find suitable queue families for graphics and presentation
    for (uint32_t i = 0; i < familyQueueCount; ++i) {
        // Check for graphics support
        if (familyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            graphicsQueueFamilyIndex = i;
        }

        // Check for presentation support
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(phyDevice, i, surface, &presentSupport);
        if (presentSupport) {
            presentQueueFamilyIndex = i;
        }

        // Stop if we found both queues
        if (graphicsQueueFamilyIndex != -1 && presentQueueFamilyIndex != -1) {
            break;
        }
    }

    // Ensure we found suitable queue families
    if (graphicsQueueFamilyIndex == -1 || presentQueueFamilyIndex == -1) {
        DEBUG_MSG("Failed to find suitable queue families for graphics and presentation\n");
        exit(-1);
    }

    // Prepare queue create info structures
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::vector<float> queuePriority(1, 1.0f); // Priority for a single queue

    // Add graphics queue creation info
    VkDeviceQueueCreateInfo graphicsQueueCreateInfo = {};
    graphicsQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    graphicsQueueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    graphicsQueueCreateInfo.queueCount = 1; // We only need one queue for graphics
    graphicsQueueCreateInfo.pQueuePriorities = queuePriority.data();
    queueCreateInfos.push_back(graphicsQueueCreateInfo);

    // If the graphics and present queues are in different families, add another for present
    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex) {
        VkDeviceQueueCreateInfo presentQueueCreateInfo = {};
        presentQueueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        presentQueueCreateInfo.queueFamilyIndex = presentQueueFamilyIndex;
        presentQueueCreateInfo.queueCount = 1; // We only need one queue for presentation
        presentQueueCreateInfo.pQueuePriorities = queuePriority.data();
        queueCreateInfos.push_back(presentQueueCreateInfo);
    }

    // Create logical device
    VkDeviceCreateInfo deviceInfo = {};
    deviceInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    deviceInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    deviceInfo.pQueueCreateInfos = queueCreateInfos.data();
    deviceInfo.enabledLayerCount = 0;
    deviceInfo.ppEnabledLayerNames = nullptr;
    deviceInfo.enabledExtensionCount = 1;
    const char* extensions[] = {
        "VK_KHR_swapchain"
    };
    deviceInfo.ppEnabledExtensionNames = extensions;
    deviceInfo.pEnabledFeatures = nullptr;

    if (vkCreateDevice(phyDevice, &deviceInfo, nullptr, &device) != VK_SUCCESS) {
        DEBUG_MSG("Failed to create Vulkan device\n");
        exit(-1);
    }

    // Retrieve the graphics and presentation queues
    vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);
    vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);
}

VkCommandPool pool;
void createCommandPool(){
    VkCommandPoolCreateInfo poolInfo = {};
    poolInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.pNext = nullptr;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    poolInfo.queueFamilyIndex = 0;

    vkCreateCommandPool(device,&poolInfo,&allocator,&pool);
}

VkCommandBuffer buffer;
void allocateCommandBuffer(){
    VkCommandBufferAllocateInfo allocInfo = {};
    allocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    vkAllocateCommandBuffers(device,&allocInfo, &buffer);
}

#ifdef DEBUG
#define getBinary(data) getBinaryInner(data)
#else
#define getBinary(data) getBinaryInner(data, data##_LEN)
#endif

#ifdef DEBUG
std::vector<char> getBinaryInner(const char* filename){
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            DEBUG_MSG("failed to open file!\n");
            exit(-1);
        }

        size_t fileSize = (size_t) file.tellg();
        file.seekg(0);

        std::vector<char> data;
        data.resize(fileSize);


        file.read(data.data(),fileSize);

        file.close();

        return data;
}
#else
std::vector<char> getBinaryInner(unsigned char* data, unsigned int len){
    std::vector<char> vector;
    vector.resize(len);
    memcpy(vector.data(),data,len);
    return vector;
}
#endif

#ifndef DEBUG
#define createShaderModule(data) createShaderModuleInner(data, data##_LEN)
#else
#define createShaderModule(filename) createShaderModuleInner(filename)
#endif


VkShaderModule createShaderModuleInner
#ifndef DEBUG
(unsigned char* data, unsigned int len){
    auto ShaderContent = getBinaryInner(data, len);
#else
(const char* filename){
    auto ShaderContent = getBinaryInner(filename);
#endif
    VkShaderModuleCreateInfo ShaderInfo = {};
    ShaderInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    ShaderInfo.pNext = nullptr;
    ShaderInfo.flags = 0;
    ShaderInfo.codeSize = ShaderContent.size();
    ShaderInfo.pCode = reinterpret_cast<const uint32_t*>(ShaderContent.data());
    VkShaderModule Shader;
    vkCreateShaderModule(
        device,
        &ShaderInfo,
        &allocator,
        &Shader
    );
    return Shader;
}

VkSurfaceKHR surface;
void createSurface(){
    VkWin32SurfaceCreateInfoKHR surfaceAllocInfo = {};
    surfaceAllocInfo.sType = VkStructureType::VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    surfaceAllocInfo.pNext = nullptr;
    surfaceAllocInfo.flags = 0;
    surfaceAllocInfo.hinstance = window->windowClass.hInstance;
    surfaceAllocInfo.hwnd = window->handle;
    
    vkCreateWin32SurfaceKHR(
        instance,
        &surfaceAllocInfo,
        &allocator,
        &surface
    );
}

VkFormat swapChainImageFormat;
VkExtent2D swapchainExtent;
std::vector<VkImage> swapChainImages;

VkSwapchainKHR swapchain;
void createSwapChain(){
    VkSwapchainCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.pNext = nullptr;
    createInfo.flags = 0;
    createInfo.surface = surface;

    VkSurfaceCapabilitiesKHR capabilities = {};
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(phyDevice, surface, &capabilities);

    uint32_t formatCount;
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, nullptr);
    std::vector<VkSurfaceFormatKHR> formats(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(phyDevice, surface, &formatCount, formats.data());

    VkSurfaceFormatKHR surfaceFormat = formats[0];
    for (const auto& availableFormat : formats) {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            surfaceFormat = availableFormat;
            break;
        }
    }

    if (capabilities.currentExtent.width == UINT32_MAX) {
        createInfo.imageExtent = VkExtent2D{ std::clamp(800u, capabilities.minImageExtent.width, capabilities.maxImageExtent.width),
                                            std::clamp(600u, capabilities.minImageExtent.height, capabilities.maxImageExtent.height) };
    } else {
        createInfo.imageExtent = capabilities.currentExtent;
    }

    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    uint32_t imageCount = capabilities.minImageCount + 1;
    if (capabilities.maxImageCount > 0 && imageCount > capabilities.maxImageCount) {
        imageCount = capabilities.maxImageCount;
    }
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    swapChainImageFormat = surfaceFormat.format;
    swapchainExtent = capabilities.currentExtent;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
    createInfo.preTransform = capabilities.currentTransform;
    createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    createInfo.imageArrayLayers = 1;
    vkCreateSwapchainKHR(device, &createInfo, &allocator, &swapchain);

    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapchain, &imageCount, swapChainImages.data());
}

std::vector<VkImageView> swapChainImageViews;
void createImageViews(){
    swapChainImageViews.resize(swapChainImages.size());

    for (uint32_t i = 0; i < swapChainImages.size(); i++) {
        swapChainImageViews[i] = createImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
    }
}

std::vector<VkFramebuffer> swapChainFramebuffers;
void createFramebuffers(){
    swapChainFramebuffers.resize(swapChainImageViews.size());
    for (size_t i = 0; i < swapChainImageViews.size(); i++) {
        std::array<VkImageView, 2> attachments = {
            swapChainImageViews[i],
            depthImageView
        };

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = renderPass;
        framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        framebufferInfo.pAttachments = attachments.data();
        framebufferInfo.width = swapchainExtent.width;
        framebufferInfo.height = swapchainExtent.height;
        framebufferInfo.layers = 1;

        vkCreateFramebuffer(device, &framebufferInfo, &allocator, &swapChainFramebuffers[i]);
    }
}

VkRenderPass renderPass;
void createRenderPass(){

    VkAttachmentDescription colorAttachment = {};
    colorAttachment.format = swapChainImageFormat; // Swapchain format
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT; // Sample count
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR; // Load operation for the attachment
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE; // Store operation for the attachment
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE; // Stencil load operation
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE; // Stencil store operation
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED; // Initial layout of the attachment
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR; // Final layout of the attachment

    VkAttachmentDescription depthAttachment{};
    depthAttachment.format = findDepthFormat();
    depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkAttachmentReference colorAttachmentRef = {};
    colorAttachmentRef.attachment = 0; // Index of the color attachment in the attachments array
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // Layout used in this subpass

    VkAttachmentReference depthAttachmentRef{};
    depthAttachmentRef.attachment = 1;
    depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;
    subpass.pDepthStencilAttachment = &depthAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

    std::array<VkAttachmentDescription, 2> attachments = {colorAttachment, depthAttachment};

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
    renderPassInfo.pAttachments = attachments.data();
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    vkCreateRenderPass(
        device,
        &renderPassInfo,
        &allocator,
        &renderPass
    );
}

VkSemaphore imageAvailableSemaphore;
VkSemaphore renderFinishedSemaphore;
VkFence inFlightFence;

void createSyncObjects() {
    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    vkCreateSemaphore(device, &semaphoreInfo, &allocator, &imageAvailableSemaphore);
    vkCreateSemaphore(device, &semaphoreInfo, &allocator, &renderFinishedSemaphore);
    vkCreateFence(device, &fenceInfo, &allocator, &inFlightFence);
}

void recordCommandBuffer(uint32_t imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = 0; // Optional
    beginInfo.pInheritanceInfo = nullptr; // Optional

    vkBeginCommandBuffer(buffer, &beginInfo);


    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = renderPass;
    renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = swapchainExtent;

    std::array<VkClearValue, 2> clearValues{};
    clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
    clearValues[1].depthStencil = {1.0f, 0};

    renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
    renderPassInfo.pClearValues = clearValues.data();
    vkCmdBeginRenderPass(buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    vkCmdBindPipeline(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
    
    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = static_cast<float>(swapchainExtent.width);
    viewport.height = static_cast<float>(swapchainExtent.height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(buffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;
    vkCmdSetScissor(buffer, 0, 1, &scissor);

    VkDeviceSize offsets[] = {0};
    for(auto& [meshView, RendDescriptor]: renderDescriptorSets){
        vkCmdBindDescriptorSets(buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &RendDescriptor.sets[imageIndex], 0, nullptr);
        vkCmdBindVertexBuffers(buffer, 0, 1, &meshView.first->vertexBuffer, offsets);
        vkCmdBindIndexBuffer(buffer, meshView.first->indexBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(buffer, meshView.first->size,RendDescriptor.instance_count,0,0,0);
    }

    vkCmdEndRenderPass(buffer);
    vkEndCommandBuffer(buffer);
}


UniformBufferObject ubo{};

void updateUniformBuffer(uint32_t currentImage) {
    memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
}

void drawFrame() {
    vkWaitForFences(device, 1, &inFlightFence, VK_TRUE, UINT64_MAX);
    vkResetFences(device, 1, &inFlightFence);

    uint32_t imageIndex;
    vkAcquireNextImageKHR(device, swapchain, UINT64_MAX, imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);

    vkResetCommandBuffer(buffer, 0);
    updateUniformBuffer(imageIndex);
    for(auto& [key, pair] : modelsToRender){
        VkDeviceSize bufferSize = sizeof(InstanceBufferObject);
        if(renderDescriptorSets[key].initialized == false){
            std::vector<VkBuffer> instanceBuffers;
            std::vector<VkDeviceMemory> instanceBuffersMemory;
            instanceBuffers.resize(swapChainFramebuffers.size());
            instanceBuffersMemory.resize(swapChainFramebuffers.size());

            for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
                createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, instanceBuffers[i], instanceBuffersMemory[i]);
            }

            auto pool = createDescriptorPool();
            auto sets = createDescriptorSets(pool,descriptorSetLayout,key.second->view,instanceBuffers);
            renderDescriptorSets[key].pool = pool;
            renderDescriptorSets[key].sets = sets;
            renderDescriptorSets[key].instanceBuffers = instanceBuffers;
            renderDescriptorSets[key].instanceBuffersMemory = instanceBuffersMemory;
            renderDescriptorSets[key].initialized = true;
        }

        void* mapped;
        size_t count = pair.size();
        vkMapMemory(device, renderDescriptorSets[key].instanceBuffersMemory[imageIndex], 0, bufferSize, 0, &mapped);
        memcpy(mapped,pair.data(),count*sizeof(InstanceData));
        vkUnmapMemory(device, renderDescriptorSets[key].instanceBuffersMemory[imageIndex]);
        renderDescriptorSets[key].instance_count = count;
    }

    recordCommandBuffer(imageIndex);

    for(auto& [key, pair] : renderDescriptorSets){
        pair.instance_count = 0;
    }

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {imageAvailableSemaphore};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &buffer;

    VkSemaphore signalSemaphores[] = {renderFinishedSemaphore};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFence);

    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapchain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    presentInfo.pResults = nullptr; // Optiona
    vkQueuePresentKHR(presentQueue, &presentInfo);
}

VkPipeline graphicsPipeline;
VkPipelineLayout pipelineLayout;
void createGraphicsPipeline(){
    VkShaderModule vertShaderModule = createShaderModule(SRC_ASSETS_VERTEX);
    VkShaderModule fragShaderModule = createShaderModule(SRC_ASSETS_FRAGMENT);

    VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
    vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
    vertShaderStageInfo.module = vertShaderModule;
    vertShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
    fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    fragShaderStageInfo.module = fragShaderModule;
    fragShaderStageInfo.pName = "main";

    VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

    ////////////////////////////////////////////////////////////// vertex structure shinanigans /////////////////////////////////////////////

    VkVertexInputBindingDescription bindingDescription{};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    const size_t n = 2;

    std::array<VkVertexInputAttributeDescription, n> attributeDescriptions{};
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(Vertex, pos);

    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(Vertex, uv);

    VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInputInfo.vertexBindingDescriptionCount = 1;
    vertexInputInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
    vertexInputInfo.vertexAttributeDescriptionCount = n;
    vertexInputInfo.pVertexAttributeDescriptions = &attributeDescriptions[0]; // Optional

    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float) swapchainExtent.width;
    viewport.height = (float) swapchainExtent.height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = swapchainExtent;

    std::vector<VkDynamicState> dynamicStates = {
        VK_DYNAMIC_STATE_VIEWPORT,
        VK_DYNAMIC_STATE_SCISSOR
    };

    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
    dynamicState.pDynamicStates = dynamicStates.data();

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;
    rasterizer.depthBiasConstantFactor = 0.0f; // Optional
    rasterizer.depthBiasClamp = 0.0f; // Optional
    rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    multisampling.minSampleShading = 1.0f; // Optional
    multisampling.pSampleMask = nullptr; // Optional
    multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
    multisampling.alphaToOneEnable = VK_FALSE; // Optional

    VkPipelineColorBlendAttachmentState colorBlendAttachment{};
    colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    colorBlendAttachment.blendEnable = VK_TRUE;
    colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

    VkPipelineColorBlendStateCreateInfo colorBlending{};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &colorBlendAttachment;
    colorBlending.blendConstants[0] = 0.0f; // Optional
    colorBlending.blendConstants[1] = 0.0f; // Optional
    colorBlending.blendConstants[2] = 0.0f; // Optional
    colorBlending.blendConstants[3] = 0.0f; // Optional

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
    pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

    vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout);

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.depthBoundsTestEnable = VK_FALSE;
    depthStencil.minDepthBounds = 0.0f; // Optional
    depthStencil.maxDepthBounds = 1.0f; // Optional
    depthStencil.stencilTestEnable = VK_FALSE;
    depthStencil.front = {}; // Optional
    depthStencil.back = {}; // Optional

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = 2;
    pipelineInfo.pStages = shaderStages;
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = pipelineLayout;
    pipelineInfo.renderPass = renderPass;
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
    pipelineInfo.basePipelineIndex = -1; // Optional
    vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline);
}

#ifdef DEBUG
void printInstanceEtensions(){
    uint32_t extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    for (const auto& extension : availableExtensions) {
        DEBUG_MSG("Available extension: %s\n" , extension.extensionName);
    }
}

void printDeviceExtensions(){
    uint32_t extensionCount2 = 0;
    vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &extensionCount2, nullptr);
    std::vector<VkExtensionProperties> availableExtensions2(extensionCount2);
    vkEnumerateDeviceExtensionProperties(phyDevice, nullptr, &extensionCount2, availableExtensions2.data());
    for (const auto& extension : availableExtensions2) {
        DEBUG_MSG("Available device extension: %s\n", extension.extensionName);
    }
}
#else
void printInstanceEtensions(){}
void printDeviceExtensions(){}
#endif

uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties, VkPhysicalDeviceMemoryProperties& memoryProperties) {
    for (uint32_t i = 0; i < memoryProperties.memoryTypeCount; i++) {
        if ((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties) == properties) {
            return i;
        }
    }
    exit(-1);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
    VkBufferCreateInfo bufferInfo{};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    vkCreateBuffer(device, &bufferInfo, &allocator, &buffer);

    VkMemoryRequirements memRequirements;
    vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, memoryProperties);

    vkAllocateMemory(device, &allocInfo, &allocator, &bufferMemory);

    vkBindBufferMemory(device, buffer, bufferMemory, 0);
}

void destroyBuffer(VkBuffer buffer, VkDeviceMemory bufferMemory) {
    vkFreeMemory(device, bufferMemory, &allocator);
    vkDestroyBuffer(device, buffer, &allocator);
}

void createVertexBuffer(VkBuffer& vertexBuffer, VkDeviceMemory& vertexBufferMemory, VkDeviceSize count) {
    VkDeviceSize bufferSize = count * sizeof(Vertex);
    createBuffer(
        bufferSize, 
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        vertexBuffer, 
        vertexBufferMemory
    );
}

void sendDataToVertexBuffer(VkBuffer vertexBuffer, VkDeviceMemory vertexBufferMemory, const std::vector<Vertex>& vertices, VkDevice device) {
    VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();
    void* data;
    vkMapMemory(device, vertexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, vertexBufferMemory);
}

void createIndexBuffer(VkBuffer& indexBuffer, VkDeviceMemory& indexBufferMemory, VkDeviceSize count) {
    VkDeviceSize bufferSize = count * sizeof(uint32_t);
    createBuffer(
        bufferSize, 
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT, 
        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, 
        indexBuffer, 
        indexBufferMemory
    );
}

void sendDataToIndexBuffer(VkBuffer indexBuffer, VkDeviceMemory indexBufferMemory, const std::vector<uint32_t>& indices) {
    VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();
    void* data;
    vkMapMemory(device, indexBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(device, indexBufferMemory);
}

struct RenderDescriptor{
    VkDescriptorPool pool;
    std::vector<VkDescriptorSet> sets;
    std::vector<VkBuffer> instanceBuffers;
    std::vector<VkDeviceMemory> instanceBuffersMemory;
    size_t instance_count = 0;
    bool initialized = false;

    RenderDescriptor(){}

    RenderDescriptor(VkDescriptorPool& pool, std::vector<VkDescriptorSet>& sets, std::vector<VkBuffer>& instanceBuffers, std::vector<VkDeviceMemory>& instanceBuffersMemory): pool(pool), sets(sets), instanceBuffers(instanceBuffers), instanceBuffersMemory(instanceBuffersMemory) {
        this->initialized = true;
    }

    ~RenderDescriptor(){
        if(initialized == true){
            vkDestroyDescriptorPool(app->device,pool,&app->allocator);
            for(auto buffer: instanceBuffers){
                vkDestroyBuffer(app->device,buffer,&app->allocator);
            }
        }
    }
};

VkDescriptorSetLayout descriptorSetLayout;
VkDescriptorSetLayout createDescriptorSetLayout() {
    VkDescriptorSetLayout descriptorSetLayout;
    VkDescriptorSetLayoutBinding uboLayoutBinding{};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    VkDescriptorSetLayoutBinding samplerLayoutBinding{};
    samplerLayoutBinding.binding = 1;
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding iboLayoutBinding{};
    iboLayoutBinding.binding = 2;
    iboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    iboLayoutBinding.descriptorCount = 1;
    iboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    iboLayoutBinding.pImmutableSamplers = nullptr; // Optional

    std::array<VkDescriptorSetLayoutBinding, 3> bindings = {uboLayoutBinding, samplerLayoutBinding, iboLayoutBinding};

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = bindings.size();
    layoutInfo.pBindings = bindings.data();

    vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &descriptorSetLayout);
    return descriptorSetLayout;
}

VkBuffer indexBuffer;
VkDeviceMemory indexBufferMemory;

std::vector<VkBuffer> uniformBuffers;
std::vector<VkDeviceMemory> uniformBuffersMemory;
std::vector<void*> uniformBuffersMapped;

void createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(UniformBufferObject);

    uniformBuffers.resize(swapChainFramebuffers.size());
    uniformBuffersMemory.resize(swapChainFramebuffers.size());
    uniformBuffersMapped.resize(swapChainFramebuffers.size());

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

        vkMapMemory(device, uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
    }
}

VkDescriptorPool createDescriptorPool() {
    VkDescriptorPool descriptorPool;
    std::array<VkDescriptorPoolSize, 3> poolSizes{};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainFramebuffers.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainFramebuffers.size());
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainFramebuffers.size());

    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainFramebuffers.size());
    vkCreateDescriptorPool(device, &poolInfo, nullptr, &descriptorPool);

    return descriptorPool;
}

std::vector<VkDescriptorSet> createDescriptorSets(VkDescriptorPool descriptorPool, VkDescriptorSetLayout descriptorSetLayout, VkImageView textureView, std::vector<VkBuffer>& instanceBuffers) {
    std::vector<VkDescriptorSet> descriptorSets;
    std::vector<VkDescriptorSetLayout> layouts(swapChainFramebuffers.size(), descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(swapChainFramebuffers.size());
    allocInfo.pSetLayouts = layouts.data();

    descriptorSets.resize(swapChainFramebuffers.size());
    vkAllocateDescriptorSets(device, &allocInfo, descriptorSets.data());

    for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
        VkDescriptorBufferInfo bufferInfo{};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);

        VkDescriptorImageInfo imageInfo{};
        imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        imageInfo.imageView = textureView;
        imageInfo.sampler = textureSampler;

        VkDescriptorBufferInfo instancebufferInfo{};
        instancebufferInfo.buffer = instanceBuffers[i];
        instancebufferInfo.offset = 0;
        instancebufferInfo.range = sizeof(InstanceBufferObject);

        std::array<VkWriteDescriptorSet, 3> descriptorWrites{};

        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[1].descriptorCount = 1;
        descriptorWrites[1].pImageInfo = &imageInfo;
        
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].pBufferInfo = &instancebufferInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
    return descriptorSets;
}

VkCommandBuffer beginSingleTimeCommands() {
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandPool = pool;
    allocInfo.commandBufferCount = 1;

    VkCommandBuffer commandBuffer;
    vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    vkBeginCommandBuffer(commandBuffer, &beginInfo);

    return commandBuffer;
}

void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
    vkEndCommandBuffer(commandBuffer);

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;

    vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
    vkQueueWaitIdle(graphicsQueue);

    vkFreeCommandBuffers(device, pool, 1, &commandBuffer);
}

void createImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = 1;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    if (vkCreateImage(device, &imageInfo, &allocator, &image) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image!");
    }

    VkMemoryRequirements memRequirements;
    vkGetImageMemoryRequirements(device, image, &memRequirements);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties, memoryProperties);

    if (vkAllocateMemory(device, &allocInfo, &allocator, &imageMemory) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate image memory!");
    }

    vkBindImageMemory(device, image, imageMemory, 0);
}

VkImage nullTextureImage;
VkDeviceMemory nullTextureImageMemory;
std::pair<VkImage,VkDeviceMemory> createTextureImage(Image test) {
    VkImage textureImage;
    VkDeviceMemory textureImageMemory;
    VkDeviceSize imageSize = test.pixels->size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

    void* data;
    vkMapMemory(device, stagingBufferMemory, 0, imageSize, 0, &data);
    memcpy(data, test.pixels->data(), static_cast<size_t>(imageSize));
    vkUnmapMemory(device, stagingBufferMemory);

    createImage(test.width, test.height, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, textureImage, textureImageMemory);
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
    copyBufferToImage(stagingBuffer, textureImage, static_cast<uint32_t>(test.width), static_cast<uint32_t>(test.height));
    transitionImageLayout(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
    vkDestroyBuffer(device, stagingBuffer, &allocator);
    vkFreeMemory(device, stagingBufferMemory, &allocator);
    return std::pair(textureImage, textureImageMemory);
}

void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferCopy copyRegion{};
    copyRegion.size = size;
    vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

    endSingleTimeCommands(commandBuffer);
}

void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

        if (hasStencilComponent(format)) {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    } else {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
    } else {
        DEBUG_MSG("unsupported layout transition!\n");
        exit(-1);
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );

    endSingleTimeCommands(commandBuffer);
}

void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
    VkCommandBuffer commandBuffer = beginSingleTimeCommands();

    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;

    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;

    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        width,
        height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );

    endSingleTimeCommands(commandBuffer);
}

VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) {
    VkImageViewCreateInfo viewInfo{};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    vkCreateImageView(device, &viewInfo, &allocator, &imageView);

    return imageView;
}

VkImageView nullTextureImageView;
VkSampler textureSampler;

VkImageView createTextureImageView(VkImage textureImage) {
    return createImageView(textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT);
}

void createTextureSampler() {
    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_NEAREST;
    samplerInfo.minFilter = VK_FILTER_NEAREST;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_FALSE;
    samplerInfo.maxAnisotropy = 1.0f;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    vkCreateSampler(device, &samplerInfo, &allocator, &textureSampler);
}

VkImage depthImage;
VkDeviceMemory depthImageMemory;
VkImageView depthImageView;

Image createNullTexture(size_t resolution){
    Image null;
    null.width = resolution;
    null.height = resolution;
    null.pixels = new std::vector<uint8_t>();
    null.pixels->resize(null.width*null.height*4);
    for(size_t x = 0; x < null.width; x++){
        for(size_t y = 0; y < null.height; y++){
            size_t i = y*null.width + x;
            (*null.pixels)[i * 4 + 3] = 0xFF;
            (*null.pixels)[i * 4 + 2] = (x+y)%2 ? 0x00 : 0xFF;
            (*null.pixels)[i * 4 + 1] = 0x00;
            (*null.pixels)[i * 4 + 0] = (x+y)%2 ? 0x00 : 0xFF;
        }
    }
    return null;
}

#ifdef DEBUG
std::unordered_map<std::string,MeshView> meshesHash;
std::unordered_map<std::string,Texture> texturesHash;
#else
std::unordered_map<unsigned char*,MeshView> meshesHash;
std::unordered_map<unsigned char*,Texture> texturesHash;
#endif

void vulkanInitializer(){
    createAllocator();
    initializeVulkan();
    printInstanceEtensions();
    chooseDevice();
    printDeviceExtensions();
    createSurface();
    getDevice();
    createCommandPool();
    allocateCommandBuffer();
    createSwapChain();
    createImageViews();
    createDepthResources();
    createRenderPass();
    createFramebuffers();
    createSyncObjects();
    createUniformBuffers();
    auto out = createTextureImage(createNullTexture(32));
    nullTextureImage = out.first;
    nullTextureImageMemory = out.second;
    nullTextureImageView = createTextureImageView(nullTextureImage);

    Texture nullImageTexture = {
        out,
        nullTextureImageView
    };

#ifdef DEBUG
    texturesHash["null"] = nullImageTexture;
#else
    texturesHash[nullptr] = nullImageTexture;
#endif

    createTextureSampler();
    descriptorSetLayout = createDescriptorSetLayout();
    createGraphicsPipeline();
}

void createDepthResources() {
    VkFormat depthFormat = findDepthFormat();
    createImage(swapchainExtent.width, swapchainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, depthImage, depthImageMemory);
    depthImageView = createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
    transitionImageLayout(depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

bool hasStencilComponent(VkFormat format) {
    return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat findDepthFormat() {
    return findSupportedFormat(
        {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT},
        VK_IMAGE_TILING_OPTIMAL,
        VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
    );
}

static void resizeWindowInner(void* app){
    App* appO = static_cast<App*>(app);
    appO->resizeWindow();
}

void resizeWindow(){
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createDepthResources();
    createFramebuffers();

    ubo.proj = mat4::Perspective(90.0, (float)window->width/window->height, 0.001f, 100.0f);
}

void cleanupSwapChain() {
    vkDestroyImageView(device, depthImageView, &allocator);
    vkDestroyImage(device, depthImage, &allocator);
    vkFreeMemory(device, depthImageMemory, &allocator);
    for (auto framebuffer : swapChainFramebuffers) {
        vkDestroyFramebuffer(device, framebuffer, &allocator);
    }
    for (auto imageView : swapChainImageViews) {
        vkDestroyImageView(device, imageView, &allocator);
    }
    vkDestroySwapchainKHR(device, swapchain, &allocator);
}

VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
    for (VkFormat format : candidates) {
        VkFormatProperties props;
        vkGetPhysicalDeviceFormatProperties(phyDevice, format, &props);

        if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
            return format;
        } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
            return format;
        }
    }

    DEBUG_MSG("failed to find supported format!\n");
    exit(-1);
}

const float smoothingFactor = 0.75f;  // The lower the value, the smoother the response

void EnableMouseCapture() {
    ShowCursor(FALSE); // Hide the cursor

    RECT rect;
    GetClientRect(window->handle, &rect);
    MapWindowPoints(window->handle, NULL, (POINT*)&rect, 2);

    ClipCursor(&rect); // Restrict the cursor to the window
}

void DisableMouseCapture() {
    ShowCursor(TRUE); // Show the cursor
    ClipCursor(NULL); // Release cursor restriction
}

struct PreVertex{
    vec3 pos;
    vec3 normal;
};

struct FaceDescriptor{
    uint16_t pos_nor_id;
    uint16_t uv_id;
};

#ifdef DEBUG
#define loadMesh(data) loadMeshInner(data)
#else
#define loadMesh(data) loadMeshInner(data, data##_LEN)
#endif

Mesh loadMeshInner
#ifndef DEBUG
(unsigned char* data, unsigned int len){
    auto file_data = getBinaryInner(data, len);
#else
(const char* filename){
    auto file_data = getBinaryInner(filename);
#endif
    uint16_t pos_nor_count = *((uint16_t*)file_data.data());
    size_t offset = sizeof(uint16_t);
    float* pos_nor_ptr = (float*)(file_data.data() + offset);
    std::vector<PreVertex> pos_nor;
    pos_nor.resize(pos_nor_count);
    memcpy(pos_nor.data(), pos_nor_ptr, pos_nor_count * sizeof(PreVertex));
    offset += pos_nor_count * sizeof(PreVertex);

    uint16_t uv_count = *((uint16_t*)(file_data.data() + offset));
    offset += sizeof(uint16_t);
    vec2* uv_ptr = (vec2*)(file_data.data() + offset);
    std::vector<vec2> uvs;
    uvs.resize(uv_count);
    memcpy(uvs.data(), uv_ptr, uv_count * sizeof(vec2));
    offset += uv_count * sizeof(vec2);

    uint16_t triangle_count = *((uint16_t*)(file_data.data() + offset));
    offset += sizeof(uint16_t);
    FaceDescriptor* pre_vertex_ptr = (FaceDescriptor*)(file_data.data() + offset);
    std::vector<FaceDescriptor> face_descriptors;
    face_descriptors.resize(triangle_count * 3);
    memcpy(face_descriptors.data(), pre_vertex_ptr, triangle_count * 3 * sizeof(FaceDescriptor));
    offset += triangle_count * 3 * sizeof(FaceDescriptor);

    std::map<std::pair<uint16_t, uint16_t>, uint32_t> vertex_map;
    std::vector<Vertex> vertices;
    std::vector<uint32_t> indices;

    for (const auto& fd : face_descriptors) {
        auto key = std::make_pair(fd.pos_nor_id, fd.uv_id);

        if (vertex_map.find(key) == vertex_map.end()) {
            const PreVertex& pn = pos_nor[fd.pos_nor_id];
            const vec2& uv = uvs[fd.uv_id];

            Vertex vertex{ {pn.pos.x, -pn.pos.z, pn.pos.y}, {-uv.x, -uv.y} };
            uint32_t new_index = static_cast<uint32_t>(vertices.size());
            vertices.push_back(vertex);
            vertex_map[key] = new_index;
        }

        indices.push_back(vertex_map[key]);
    }

    return { vertices, indices };
}

int GetMonitorRefreshRate(HWND hwnd) {
    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (!hMonitor) {
        DEBUG_MSG("Failed to get monitor handle\n");
        return -1;
    }

    MONITORINFOEX monitorInfoEx = { 0 };
    monitorInfoEx.cbSize = sizeof(MONITORINFOEX);

    if (!GetMonitorInfo(hMonitor, &monitorInfoEx)) {
        DEBUG_MSG("Failed to get monitor info\n");
        return -1;
    }

    DEBUG_MSG("Monitor Device Name: %s\n",monitorInfoEx.szDevice);

    DEVMODE devMode = {};
    devMode.dmSize = sizeof(DEVMODE);

    if (!EnumDisplaySettings(monitorInfoEx.szDevice, ENUM_CURRENT_SETTINGS, &devMode)) {
        DEBUG_MSG("Failed to enumerate display settings for device: %s\n",monitorInfoEx.szDevice);
        return -1;
    }

    return devMode.dmDisplayFrequency;
}

void DisableWindowDecoration(HWND hwnd) {
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    style &= ~(WS_OVERLAPPEDWINDOW);

    SetWindowLong(hwnd, GWL_STYLE, style);
}

void EnableWindowDecoration(HWND hwnd) {
    LONG style = GetWindowLong(hwnd, GWL_STYLE);

    style |= (WS_OVERLAPPEDWINDOW);

    SetWindowLong(hwnd, GWL_STYLE, style);
}

void GetMonitorPositionAndSizeFromWindow(HWND hwnd, Ivec2 &position, Ivec2 &size) {
    position = { 0, 0 };
    size = { 0, 0 };

    HMONITOR hMonitor = MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
    if (hMonitor == NULL) {
       DEBUG_MSG("Failed to find monitor!\n");
        return;
    }

    MONITORINFO monitorInfo;
    monitorInfo.cbSize = sizeof(MONITORINFO);
    if (GetMonitorInfo(hMonitor, &monitorInfo)) {
        position.x = monitorInfo.rcMonitor.left;
        position.y = monitorInfo.rcMonitor.top;
        size.x = monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left;
        size.y = monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top;
    } else {
        DEBUG_MSG("Failed to get monitor info!\n");
    }
}

Ivec2 platform_get_cursor_pos(){
    POINT p;
    GetCursorPos(&p);
    ScreenToClient(window->handle,&p);
    return {p.x, p.y};
}

void platform_set_cursor_pos(Ivec2 pos){
    POINT p = {pos.x, pos.y};
    ClientToScreen(window->handle, &p);
    SetCursorPos(p.x, p.y);
}

// Custom hash function for std::pair<MeshView*, Texture*>
struct PairPointerHash {
    std::size_t operator()(const std::pair<MeshView*, Texture*>& pair) const {
        // Check if the pointers are good (non-null)
        if (!pair.first || !pair.second) {
            throw std::invalid_argument("Invalid pointer detected in the pair!");
        }

        // Hash the pointers
        std::size_t h1 = std::hash<MeshView*>()(pair.first);
        std::size_t h2 = std::hash<Texture*>()(pair.second);

        // Combine the two hash values
        return h1 ^ (h2 << 1);  // Combine hashes using XOR and left-shift
    }
};

// Custom equality comparison function for std::pair<MeshView*, Texture*>
struct PairPointerEqual {
    bool operator()(const std::pair<MeshView*, Texture*>& lhs, const std::pair<MeshView*, Texture*>& rhs) const {
        // Compare pointers' addresses for equality
        return lhs.first == rhs.first && lhs.second == rhs.second;
    }
};

std::unordered_map<std::pair<MeshView*, Texture*>, std::vector<InstanceData>, PairPointerHash,PairPointerEqual> modelsToRender;
std::unordered_map<std::pair<MeshView*, Texture*>, RenderDescriptor, PairPointerHash, PairPointerEqual> renderDescriptorSets;

#ifdef DEBUG
    MeshView* (*getModel)(const char* filename);
    Texture* (*getTexture)(const char* filename);
#else
    MeshView* (*getModel)(unsigned char* filename);
    Texture* (*getTexture)(unsigned char* filename);
#endif

#ifdef DEBUG
static MeshView* getModelInner(const char* filename){
#else
static MeshView* getModelInner(unsigned char* filename, unsigned int len){
#endif

#ifdef DEBUG
    std::string input = std::string(filename);
#else
    unsigned char* input = filename;
#endif
    if(app->meshesHash[input].size == 0){
        MeshView mesh(
            app, 
#ifdef DEBUG
            app->loadMeshInner(filename)
#else
            app->loadMeshInner(filename, len)
#endif
        );

        app->meshesHash[input] = mesh;
    }
    return &app->meshesHash[input];
}


#ifdef DEBUG
static Texture* getTextureInner(const char* filename){
#else
static Texture* getTextureInner(unsigned char* filename, unsigned int len){
#endif

#ifdef DEBUG
    std::string input = std::string(filename);
#else
    unsigned char* input = filename;
#endif
    if(app->texturesHash[input].view == nullptr){

        Image image =
#ifdef DEBUG
        getImagePixelsInner(filename);
#else
        getImagePixelsInner(filename,len);
#endif
        ;

        auto myTexture = app->createTextureImage(image);
        VkImageView myTextureView = app->createTextureImageView(myTexture.first);

        Texture texture = {
            myTexture,
            myTextureView
        };

        app->texturesHash[input] = texture;
    }
    return &app->texturesHash[input];
}

static void drawMesh(MeshView* mesh, Texture* texture, InstanceData data){
    if(texture == nullptr){
#ifdef DEBUG
    texture =  &app->texturesHash["null"];
#else
    texture =  &app->texturesHash[nullptr];
#endif
    }
    if(mesh == nullptr) return;
    app->modelsToRender[std::pair(mesh,texture)].push_back(data);
}

int run(){
    float deltaTime = 0.0f;
    vulkanInitializer();

    auto oldDeltaTimer = std::chrono::high_resolution_clock::now();

    vec2 pre_fullScreenPos;
    vec2 pre_fullScreenSize;

    // int monitorHZ = GetMonitorRefreshRate(window->handle);
    // if(monitorHZ == -1){
    //     monitorHZ = 420;
    // }
    int monitorHZ = 420;
    float targetFps = 1.0f / monitorHZ;

    auto myTexture = createTextureImage(getImagePixels(SRC_ASSETS_TEST));
    VkImageView myTextureView = createTextureImageView(myTexture.first);

    // Texture testerTexture = createTexture();

    game = new Game;
    game->window = window;
    game->ubo = &ubo;
    game->callbacks.drawMesh = drawMesh;
    game->callbacks.getModelInner = getModelInner;
    game->callbacks.getTextureInner = getTextureInner;
// #ifndef DEBUG
//     game->fullscreen = true;
//     window->locked_mouse = true;
//     window->hidden_mouse = true;
//     bool firstTime = true;
// #endif

    ubo.proj = mat4::Perspective(90.0, (float)window->width/window->height, 0.001f, 100.0f);

    bool prev_fullscreen = false;
    bool flop = true;

#ifndef DEBUG
    init_game(game);
#endif

    while(opened_windows > 0){
    #ifdef DEBUG
        reload_game_dll(game);
    #endif

        auto timer = std::chrono::high_resolution_clock::now();
        long long elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(timer - oldDeltaTimer).count();
        deltaTime = static_cast<float>(elapsedNanoseconds) / 1e9f; // deltaTime in seconds
        oldDeltaTimer = timer;  // Reset the old timer to the current time

        HandleWindowEvents();
        if(window->prev_hidden_mouse != window->hidden_mouse){
            window->prev_hidden_mouse = window->hidden_mouse;
            if(window->hidden_mouse){
                EnableMouseCapture();
            }else{
                DisableMouseCapture();
            }
        }

        if(window->locked_mouse){
            window->mouse_pos = platform_get_cursor_pos() - (Ivec2(window->width, window->height)/2);
            if(window->mouse_pos.x != 0 || window->mouse_pos.y != 0) platform_set_cursor_pos({(int)window->width/2, (int)window->height/2});
        }


        update_game(deltaTime, game);

        if(game->fullscreen != prev_fullscreen){
            prev_fullscreen = game->fullscreen;
            if(game->fullscreen){
                RECT rect;
                if(GetWindowRect(window->handle, &rect)){
                    pre_fullScreenSize = vec2(rect.right - rect.left, rect.bottom - rect.top);
                    pre_fullScreenPos = vec2(rect.left, rect.top);
                }
                DisableWindowDecoration(window->handle);
                Ivec2 pos;
                Ivec2 size;
                GetMonitorPositionAndSizeFromWindow(window->handle, pos, size);
                SetWindowPos(window->handle, nullptr, pos.x, pos.y, size.x, size.y, SWP_FRAMECHANGED);
            }else{
                EnableWindowDecoration(window->handle);
                SetWindowPos(window->handle, nullptr, pre_fullScreenPos.x, pre_fullScreenPos.y, pre_fullScreenSize.x, pre_fullScreenSize.y, SWP_FRAMECHANGED);
            }
// #ifndef DEBUG
//             if(firstTime){
//                 Sleep(10);
//                 window->mouse_pos = platform_get_cursor_pos() - (Ivec2(window->width, window->height)/2);
//                 if(window->mouse_pos.x != 0 || window->mouse_pos.y != 0) platform_set_cursor_pos({(int)window->width/2, (int)window->height/2});
//                 firstTime = false;
//             }
// #endif

        }

        drawFrame();

        memset(window->just_pressed_keys,0,sizeof(window->just_pressed_keys));
        memset(window->just_unpressed_keys,0,sizeof(window->just_unpressed_keys));
        memset(window->just_pressed_mouseKeys,0,sizeof(window->just_pressed_mouseKeys));
        memset(window->just_unpressed_mouseKeys,0,sizeof(window->just_unpressed_mouseKeys));

        modelsToRender.clear();

        auto endtimer = std::chrono::high_resolution_clock::now();
        elapsedNanoseconds = std::chrono::duration_cast<std::chrono::nanoseconds>(endtimer - timer).count();
        float took = static_cast<float>(elapsedNanoseconds) / 1e9f;
        if(took < targetFps){
            Sleep((targetFps - took)*1000);
        }
    }
    return 0;
}

};

MeshView::MeshView(App* app, Mesh mesh){
    size = mesh.indices.size();
    app->createVertexBuffer(vertexBuffer, vertexBufferMemory, mesh.vertices.size());
    app->sendDataToVertexBuffer(vertexBuffer, vertexBufferMemory, mesh.vertices, app->device);
    app->createIndexBuffer(indexBuffer,indexBufferMemory,mesh.indices.size());
    app->sendDataToIndexBuffer(indexBuffer,indexBufferMemory,mesh.indices);
}

#ifndef DEBUG
#include "Game.cpp"
#endif

int main(){

#ifdef DEBUG
    gameLoadLibName = "game_load"+std::to_string((int)(platform_getTime()*1000.0))+".dll";
#endif

    WindowClass windowClass("GigachadInc", Window::HandleMsgSetup);
    app = new App(new Window("cool window",windowClass, 800,600));
    return app->run();
}