#define VK_ENABLE_BETA_EXTENSIONS
#define STB_IMAGE_WRITE_IMPLEMENTATION

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include <fstream>
#include <filesystem>
#include "../include/Utility.hpp"

using namespace Vulkan_Test;

int main()
{
    vk::ApplicationInfo appInfo;
    appInfo.pApplicationName = "YourApp";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "NoEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);

    if (!glfwInit())
    {
        return EXIT_FAILURE;
    }

    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.pApplicationInfo = &appInfo;

    uint32_t glfwExtensionsCount;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionsCount);
    std::vector<const char*> insRequiredExtensions = std::vector<const char*>(glfwExtensionsCount);
    std::memcpy(&*insRequiredExtensions.begin(), glfwExtensions, sizeof(const char*) * glfwExtensionsCount);

#ifdef __APPLE__
    std::vector<const char*> appleRrequiredExtentions = 
    { 
        vk::KHRPortabilityEnumerationExtensionName,
    };
    std::copy(appleRrequiredExtentions.begin(), appleRrequiredExtentions.end(), std::back_inserter(insRequiredExtensions));
#endif

    instCreateInfo.enabledExtensionCount = insRequiredExtensions.size();
    instCreateInfo.ppEnabledExtensionNames = &*insRequiredExtensions.begin();

    LOG("Instance Extensions");
    for (int i = 0; i < instCreateInfo.enabledExtensionCount; i++) 
    {
        LOG("----------------------------------------");
        LOG(instCreateInfo.ppEnabledExtensionNames[i]);
    }

#ifdef __APPLE__
    instCreateInfo.flags |= vk::InstanceCreateFlagBits::eEnumeratePortabilityKHR;
#endif

    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

    const uint32_t screenWidth = 640;
    const uint32_t screenHeight = 480;

    GLFWwindow* window;
    window = glfwCreateWindow(screenWidth, screenHeight, "GLFW Test Window", NULL, NULL);
    if (!window) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    int width;
    int height;
    glfwGetWindowSize(window, &width, &height);

    LOG("width " << width << ", height " << height);

    glfwGetFramebufferSize(window, &width, &height);
    LOG("width " << width << ", height " << height);

    VkSurfaceKHR c_surface;
    VkResult result = glfwCreateWindowSurface(instance, window, nullptr, &c_surface);
    if (result != VK_SUCCESS) 
    {
        const char* err;
        glfwGetError(&err);
        LOG(err);
        glfwTerminate();
        return EXIT_FAILURE;
    }

    vk::UniqueSurfaceKHR surfacePtr = vk::UniqueSurfaceKHR(c_surface, instance);
    vk::SurfaceKHR surface = surfacePtr.get();

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("----------------------------------------");
        LOG(props.properties.deviceName);
        LOG("deviceID: " << props.properties.deviceID);
        LOG("vendorID: " << props.properties.vendorID);
        LOG("deviceType: " << to_string(props.properties.deviceType));
    }

    uint32_t graphicsQueueFamilyIndex;
    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();

        bool foundGraphicsQueue = false;
        for (size_t j = 0; j < queueProps.size(); j++)
        {
            if (!(queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics) || !physicalDevices[i].getSurfaceSupportKHR(j, surface)) continue;
            
            foundGraphicsQueue = true;
            graphicsQueueFamilyIndex = j;
            physicalDevice = physicalDevices[i];
            break;
        }

        std::vector<vk::ExtensionProperties> extProps = physicalDevices[i].enumerateDeviceExtensionProperties();
        bool supportsSwapchainExtension = false;

        for (size_t j = 0; j < extProps.size(); j++) 
        {
            if (std::string_view(extProps[j].extensionName.data()) == VK_KHR_SWAPCHAIN_EXTENSION_NAME) 
            {
                supportsSwapchainExtension = true;
                break;
            }
        }

        bool supportsSurface =
            !physicalDevices[i].getSurfaceFormatsKHR(surface).empty() ||
            !physicalDevices[i].getSurfacePresentModesKHR(surface).empty();

        if (foundGraphicsQueue && supportsSwapchainExtension && supportsSurface) 
        {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (existsSuitablePhysicalDevice)
    {
        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("----------------------------------------");
        LOG("Found device and queue");
        LOG("physicalDevice: " << props.properties.deviceName);
        LOG("graphicsQueueFamilyIndex: " << graphicsQueueFamilyIndex);
    }
    else
    {
        LOGERR("No physical devices are available");
        return EXIT_FAILURE;
    }

    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    LOG("----------------------------------------");
    LOG("queue family count: " << queueProps.size());
    for (size_t i = 0; i < queueProps.size(); i++)
    {
        LOG("----------------------------------------");
        LOG("queue family index: " << i);
        LOG("queue count: " << queueProps[i].queueCount);
        LOG(to_string(queueProps[i].queueFlags));
    }

    float queuePriorities[1] = { 1.0f };
    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].pQueuePriorities = queuePriorities;
    queueCreateInfo[0].queueCount = std::size(queueCreateInfo);

    std::vector<const char*> requiredLayers = 
    { 
        "VK_LAYER_KHRONOS_validation" 
    };
    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = requiredLayers.size();
    devCreateInfo.ppEnabledLayerNames = &*requiredLayers.begin();
    devCreateInfo.queueCreateInfoCount = std::size(queueCreateInfo);
    devCreateInfo.pQueueCreateInfos = queueCreateInfo;

    std::vector<const char*> devRequiredExtensions = 
    {
        VK_KHR_SWAPCHAIN_EXTENSION_NAME 
    };
    devCreateInfo.enabledExtensionCount = devRequiredExtensions.size();
    devCreateInfo.ppEnabledExtensionNames = &*devRequiredExtensions.begin();

    vk::UniqueDevice devicePtr = physicalDevice.createDeviceUnique(devCreateInfo);
    vk::Device device = devicePtr.get();

    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    vk::SurfaceCapabilitiesKHR surfaceCapabilities = physicalDevice.getSurfaceCapabilitiesKHR(surface);
    std::vector<vk::SurfaceFormatKHR> surfaceFormats = physicalDevice.getSurfaceFormatsKHR(surface);
    std::vector<vk::PresentModeKHR> surfacePresentModes = physicalDevice.getSurfacePresentModesKHR(surface);

    vk::SurfaceFormatKHR swapchainFormat = surfaceFormats[0];
    vk::PresentModeKHR swapchainPresentMode = surfacePresentModes[0];

    vk::SwapchainCreateInfoKHR swapchainCreateInfo;
    swapchainCreateInfo.surface = surface;
    swapchainCreateInfo.minImageCount = surfaceCapabilities.minImageCount + 1;
    LOG("swapchain minImageCount" << swapchainCreateInfo.minImageCount);
    swapchainCreateInfo.imageFormat = swapchainFormat.format;
    swapchainCreateInfo.imageColorSpace = swapchainFormat.colorSpace;
    swapchainCreateInfo.imageExtent = surfaceCapabilities.currentExtent;
    LOG("swapchainCreateInfo " << swapchainCreateInfo.imageExtent.width << ", " << swapchainCreateInfo.imageExtent.height);
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = vk::ImageUsageFlagBits::eColorAttachment;
    swapchainCreateInfo.imageSharingMode = vk::SharingMode::eExclusive;
    swapchainCreateInfo.preTransform = surfaceCapabilities.currentTransform;
    swapchainCreateInfo.presentMode = swapchainPresentMode;
    swapchainCreateInfo.clipped = VK_TRUE;

    vk::UniqueSwapchainKHR swapchain = device.createSwapchainKHRUnique(swapchainCreateInfo);

    std::vector<vk::Image> swapchainImages = device.getSwapchainImagesKHR(swapchain.get());
    std::vector<vk::UniqueImageView> swapchainImageViews(swapchainImages.size());
    for (size_t i = 0; i < swapchainImages.size(); i++) 
    {
        vk::ImageViewCreateInfo imgViewCreateInfo;
        imgViewCreateInfo.image = swapchainImages[i];
        imgViewCreateInfo.viewType = vk::ImageViewType::e2D;
        imgViewCreateInfo.format = swapchainFormat.format;
        imgViewCreateInfo.components.r = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.g = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.b = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.components.a = vk::ComponentSwizzle::eIdentity;
        imgViewCreateInfo.subresourceRange.aspectMask = vk::ImageAspectFlagBits::eColor;
        imgViewCreateInfo.subresourceRange.baseMipLevel = 0;
        imgViewCreateInfo.subresourceRange.levelCount = 1;
        imgViewCreateInfo.subresourceRange.baseArrayLayer = 0;
        imgViewCreateInfo.subresourceRange.layerCount = 1;

        swapchainImageViews[i] = device.createImageViewUnique(imgViewCreateInfo);
    }

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    LOG("----------------------------------------");
    LOG("memory type count: " << memProps.memoryTypeCount);
    LOG("memory heap count: " << memProps.memoryHeapCount);
    for (size_t i = 0; i < memProps.memoryTypeCount; i++)
    {
        LOG("----------------------------------------");
        LOG("memory type index: " << i);
        LOG(to_string(memProps.memoryTypes[i].propertyFlags));
    }

    vk::AttachmentDescription attachments[1];
    attachments[0].format = swapchainFormat.format;
    attachments[0].samples = vk::SampleCountFlagBits::e1;
    attachments[0].loadOp = vk::AttachmentLoadOp::eClear;
    attachments[0].storeOp = vk::AttachmentStoreOp::eStore;
    attachments[0].stencilLoadOp = vk::AttachmentLoadOp::eDontCare;
    attachments[0].stencilStoreOp = vk::AttachmentStoreOp::eDontCare;
    attachments[0].initialLayout = vk::ImageLayout::eUndefined;
    attachments[0].finalLayout = vk::ImageLayout::ePresentSrcKHR;

    vk::AttachmentReference subpass0_attachmentRefs[1];
    subpass0_attachmentRefs[0].attachment = 0;
    subpass0_attachmentRefs[0].layout = vk::ImageLayout::eColorAttachmentOptimal;

    vk::SubpassDescription subpasses[1];
    subpasses[0].pipelineBindPoint = vk::PipelineBindPoint::eGraphics;
    subpasses[0].colorAttachmentCount = 1;
    subpasses[0].pColorAttachments = subpass0_attachmentRefs;

    vk::RenderPassCreateInfo renderpassCreateInfo;
    renderpassCreateInfo.attachmentCount = 1;
    renderpassCreateInfo.pAttachments = attachments;
    renderpassCreateInfo.subpassCount = 1;
    renderpassCreateInfo.pSubpasses = subpasses;
    renderpassCreateInfo.dependencyCount = 0;
    renderpassCreateInfo.pDependencies = nullptr;

    vk::UniqueRenderPass renderpass = device.createRenderPassUnique(renderpassCreateInfo);

    vk::Viewport viewports[1];
    viewports[0].x = 0.0;
    viewports[0].y = 0.0;
    viewports[0].minDepth = 0.0;
    viewports[0].maxDepth = 1.0;
    viewports[0].width = surfaceCapabilities.currentExtent.width;
    viewports[0].height = surfaceCapabilities.currentExtent.height;

    vk::Rect2D scissors[1];
    scissors[0].offset = vk::Offset2D(0, 0);
    scissors[0].extent = surfaceCapabilities.currentExtent;

    vk::PipelineViewportStateCreateInfo viewportState;
    viewportState.viewportCount = 1;
    viewportState.pViewports = viewports;
    viewportState.scissorCount = 1;
    viewportState.pScissors = scissors;

    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexAttributeDescriptionCount = 0;
    vertexInputInfo.pVertexAttributeDescriptions = nullptr;
    vertexInputInfo.vertexBindingDescriptionCount = 0;
    vertexInputInfo.pVertexBindingDescriptions = nullptr;

    vk::PipelineInputAssemblyStateCreateInfo inputAssembly;
    inputAssembly.topology = vk::PrimitiveTopology::eTriangleList;
    inputAssembly.primitiveRestartEnable = false;

    vk::PipelineRasterizationStateCreateInfo rasterizer;
    rasterizer.depthClampEnable = false;
    rasterizer.rasterizerDiscardEnable = false;
    rasterizer.polygonMode = vk::PolygonMode::eFill;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = vk::CullModeFlagBits::eBack;
    rasterizer.frontFace = vk::FrontFace::eClockwise;
    rasterizer.depthBiasEnable = false;

    vk::PipelineMultisampleStateCreateInfo multisample;
    multisample.sampleShadingEnable = false;
    multisample.rasterizationSamples = vk::SampleCountFlagBits::e1;

    vk::PipelineColorBlendAttachmentState blendattachment[1];
    blendattachment[0].colorWriteMask =
        vk::ColorComponentFlagBits::eA |
        vk::ColorComponentFlagBits::eR |
        vk::ColorComponentFlagBits::eG |
        vk::ColorComponentFlagBits::eB;
    blendattachment[0].blendEnable = false;

    vk::PipelineColorBlendStateCreateInfo blend;
    blend.logicOpEnable = false;
    blend.attachmentCount = 1;
    blend.pAttachments = blendattachment;

    vk::PipelineLayoutCreateInfo layoutCreateInfo;
    layoutCreateInfo.setLayoutCount = 0;
    layoutCreateInfo.pSetLayouts = nullptr;

    vk::UniquePipelineLayout pipelineLayout = device.createPipelineLayoutUnique(layoutCreateInfo);

    vk::UniqueShaderModule vertShader;
    vk::UniqueShaderModule fragShader;
    {
        size_t vertSpvFileSz = std::filesystem::file_size("../src/shader.vert.spv");
        std::ifstream vertSpvFile = std::ifstream("../src/shader.vert.spv", std::ios_base::binary);
        std::vector<char> vertSpvFileData = std::vector<char>(vertSpvFileSz);
        vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);

        vk::ShaderModuleCreateInfo vertShaderCreateInfo;
        vertShaderCreateInfo.codeSize = vertSpvFileSz;
        vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());
        vertShader = device.createShaderModuleUnique(vertShaderCreateInfo);

        size_t fragSpvFileSz = std::filesystem::file_size("../src/shader.frag.spv");
        std::ifstream fragSpvFile = std::ifstream("../src/shader.frag.spv", std::ios_base::binary);
        std::vector<char> fragSpvFileData = std::vector<char>(fragSpvFileSz);
        fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);

        vk::ShaderModuleCreateInfo fragShaderCreateInfo;
        fragShaderCreateInfo.codeSize = fragSpvFileSz;
        fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());
        fragShader = device.createShaderModuleUnique(fragShaderCreateInfo);
    }

    vk::PipelineShaderStageCreateInfo shaderStage[2];
    shaderStage[0].stage = vk::ShaderStageFlagBits::eVertex;
    shaderStage[0].module = vertShader.get();
    shaderStage[0].pName = "main";
    shaderStage[1].stage = vk::ShaderStageFlagBits::eFragment;
    shaderStage[1].module = fragShader.get();
    shaderStage[1].pName = "main";
    
    vk::GraphicsPipelineCreateInfo pipelineCreateInfo;
    pipelineCreateInfo.pViewportState = &viewportState;
    pipelineCreateInfo.pVertexInputState = &vertexInputInfo;
    pipelineCreateInfo.pInputAssemblyState = &inputAssembly;
    pipelineCreateInfo.pRasterizationState = &rasterizer;
    pipelineCreateInfo.pMultisampleState = &multisample;
    pipelineCreateInfo.pColorBlendState = &blend;
    pipelineCreateInfo.layout = pipelineLayout.get();
    pipelineCreateInfo.renderPass = renderpass.get();
    pipelineCreateInfo.subpass = 0;
    pipelineCreateInfo.stageCount = std::size(shaderStage);
    pipelineCreateInfo.pStages = shaderStage;

    vk::UniquePipeline pipeline = device.createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;

    std::vector<vk::UniqueFramebuffer> swapchainFramebufs(swapchainImages.size());

    for (size_t i = 0; i < swapchainImages.size(); i++) 
    {
        vk::ImageView frameBufAttachments[1];
        frameBufAttachments[0] = swapchainImageViews[i].get();

        vk::FramebufferCreateInfo frameBufCreateInfo;
        frameBufCreateInfo.width = surfaceCapabilities.currentExtent.width;
        frameBufCreateInfo.height = surfaceCapabilities.currentExtent.height;
        frameBufCreateInfo.layers = 1;
        frameBufCreateInfo.renderPass = renderpass.get();
        frameBufCreateInfo.attachmentCount = 1;
        frameBufCreateInfo.pAttachments = frameBufAttachments;

        swapchainFramebufs[i] = device.createFramebufferUnique(frameBufCreateInfo);
    }

    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
    cmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eResetCommandBuffer;
    vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);


    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);


    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence swapchainImgFence = device.createFenceUnique(fenceCreateInfo);

    while (!glfwWindowShouldClose(window)) 
    {
        glfwPollEvents();
        
        device.resetFences({ swapchainImgFence.get() });

        vk::ResultValue acquireImgResult = device.acquireNextImageKHR(swapchain.get(), 1'000'000'000, {}, swapchainImgFence.get());
        if (acquireImgResult.result != vk::Result::eSuccess) 
        {
            LOG("Failed to get next frame");
            return EXIT_FAILURE;
        }
        uint32_t imgIndex = acquireImgResult.value;
    
        if (device.waitForFences({ swapchainImgFence.get() }, VK_TRUE, 1'000'000'000) != vk::Result::eSuccess)
        {
            LOG("Failed to get next frame");
            return EXIT_FAILURE;
        }
    
        cmdBufs[0]->reset();
    
        vk::CommandBufferBeginInfo cmdBeginInfo;
        cmdBufs[0]->begin(cmdBeginInfo);
        
        vk::ClearValue clearVal[1];
        clearVal[0].color.float32[0] = 0.0f;
        clearVal[0].color.float32[1] = 0.0f;
        clearVal[0].color.float32[2] = 0.0f;
        clearVal[0].color.float32[3] = 1.0f;

        vk::RenderPassBeginInfo renderpassBeginInfo;
        renderpassBeginInfo.renderPass = renderpass.get();
        renderpassBeginInfo.framebuffer = swapchainFramebufs[imgIndex].get();
        renderpassBeginInfo.renderArea = vk::Rect2D({ 0,0 }, surfaceCapabilities.currentExtent);
        renderpassBeginInfo.clearValueCount = 1;
        renderpassBeginInfo.pClearValues = clearVal;

        cmdBufs[0]->beginRenderPass(renderpassBeginInfo, vk::SubpassContents::eInline);

        cmdBufs[0]->bindPipeline(vk::PipelineBindPoint::eGraphics, pipeline.get());
        cmdBufs[0]->draw(3, 1, 0, 0);
    
        cmdBufs[0]->endRenderPass();

        cmdBufs[0]->end();
        
        vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
        vk::SubmitInfo submitInfo;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = submitCmdBuf;
        graphicsQueue.submit({ submitInfo }, nullptr);
        graphicsQueue.waitIdle();
    
        vk::PresentInfoKHR presentInfo;

        auto presentSwapchains = { swapchain.get() };
        auto imgIndices = { imgIndex };
        
        presentInfo.swapchainCount = presentSwapchains.size();
        presentInfo.pSwapchains = presentSwapchains.begin();
        presentInfo.pImageIndices = imgIndices.begin();
        
        graphicsQueue.presentKHR(presentInfo);
        
        graphicsQueue.waitIdle();
    }

    glfwTerminate();

    return EXIT_SUCCESS;
}

void WaitPrograms(vk::Device device, vk::Queue graphicsQueue, std::vector<vk::SubmitInfo> submits, vk::SubmitInfo submitInfo)
{
    vk::FenceCreateInfo fenceCreateInfo;
    vk::UniqueFence fence = device.createFenceUnique(fenceCreateInfo);

    graphicsQueue.submit(submits, fence.get());
    std::vector<vk::Fence> fences;
    fences.push_back(fence.get());

    vk::Result result = device.waitForFences(fences, VK_TRUE, 1'000'000'000);

    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    fence = device.createFenceUnique(fenceCreateInfo);

    vk::SemaphoreCreateInfo semaphoreCreateInfo;
    vk::UniqueSemaphore semaphore = device.createSemaphoreUnique(semaphoreCreateInfo);

    vk::Semaphore signalSemaphores[] = { semaphore.get() };
    submitInfo.signalSemaphoreCount = std::size(signalSemaphores);
    submitInfo.pSignalSemaphores = signalSemaphores;
    submits.push_back(submitInfo);
    graphicsQueue.submit(submits);

    vk::Semaphore waitSemaphores[] = { semaphore.get() };
    vk::PipelineStageFlags waitStages[] = { vk::PipelineStageFlagBits::eAllCommands };

    submitInfo.waitSemaphoreCount = std::size(waitSemaphores);
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;

}