#ifdef __ANDROID__
#include <jni.h>
#endif

extern "C"
{
#include <game-activity/native_app_glue/android_native_app_glue.c>
};

#include <game-activity/GameActivity.cpp>
#include <game-text-input/gametextinput.cpp>

#include <memory>
#include "vulkan/vulkan.hpp"
#include "AndroidOut.h"
#include "Utility.hpp"
#include "Debug.hpp"
#include "Device.hpp"
#include "PhysicalDevice.hpp"
#include "Surface.hpp"
#include "Swapchain.hpp"
#include "FrameBuffer.hpp"
#include "Pipeline.hpp"
#include "RenderPass.hpp"
#include "Subpass.hpp"
#include "Command.hpp"
#include "Instance.hpp"
#include "ShaderData.hpp"
#include "Texture.hpp"
#include "Depth.hpp"

void initVulkan(android_app* pApp) {
    if (!pApp->window) {
        LOG("ウィンドウがないのでVulkanを初期化できません");
        return;
    }

    std::shared_ptr<vk::ApplicationInfo> appInfo = getAppInfo();
    debugApplicationInfo(*appInfo);

    std::shared_ptr<vk::UniqueInstance> instance = getInstance(*appInfo);
    std::shared_ptr<vk::UniqueSurfaceKHR> surface = getSurface(*instance, pApp->window);

    std::shared_ptr<std::vector<vk::PhysicalDevice>> physicalDevices = getPhysicalDevices(*instance);
    debugPhysicalDevices(*physicalDevices);

    std::shared_ptr<std::pair<vk::PhysicalDevice, uint32_t>> physicalDeviceAndQueueFamilyIndex = selectPhysicalDeviceAndQueueFamilyIndex(*physicalDevices, *surface);
    vk::PhysicalDevice physicalDevice;
    uint32_t queueFamilyIndex;
    std::tie(physicalDevice, queueFamilyIndex) = *physicalDeviceAndQueueFamilyIndex;
    debugPhysicalDevice(physicalDevice, queueFamilyIndex);
    debugPhysicalMemory(physicalDevice);

    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    debugQueueFamilyProperties(queueProps);

    std::shared_ptr<vk::UniqueDevice> device = getDevice(physicalDevice, queueFamilyIndex);

    vk::Queue graphicsQueue = device->get().getQueue(queueFamilyIndex, 0);

    std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> vertexBindingDescription = getVertexBindingDescription();
    std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> vertexInputDescription = getVertexInputDescription();

    std::shared_ptr<vk::UniqueBuffer> vertexBuf = getVertexBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> vertexBufMem = getVertexBufferMemory(*device, physicalDevice, *vertexBuf);
    std::shared_ptr<vk::UniqueBuffer> stagingVertexBuf = getStagingVertexBuffer(*device, physicalDevice);
    std::shared_ptr<vk::UniqueDeviceMemory> stagingVertexBufMem = getStagingVertexBufferMemory(*device, physicalDevice, *stagingVertexBuf);
    writeStagingVertexBuffer(*device, *stagingVertexBufMem);
    sendVertexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingVertexBuf, *vertexBuf);

    std::shared_ptr<vk::UniqueBuffer> indexBuf = getIndexBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> indexBufMem = getIndexBufferMemory(*device, physicalDevice, *indexBuf);
    std::shared_ptr<vk::UniqueBuffer> stagingIndexBuf = getStagingIndexBuffer(*device, physicalDevice);
    std::shared_ptr<vk::UniqueDeviceMemory> stagingIndexBufMem = getStagingIndexBufferMemory(*device, physicalDevice, *stagingIndexBuf);
    writeStagingIndexBuffer(*device, *stagingIndexBufMem);
    sendIndexBuffer(*device, queueFamilyIndex, graphicsQueue, *stagingIndexBuf, *indexBuf);

    int imgWidth, imgHeight, imgCh;
    void* imgData = getImageData(&imgWidth, &imgHeight, &imgCh);
    std::shared_ptr<vk::UniqueImage> texImage = getImage(*device, imgWidth, imgHeight, imgCh);
    std::shared_ptr<vk::UniqueDeviceMemory> imgBufMemory = getImageMemory(*device, physicalDevice, *texImage);
    std::shared_ptr<vk::UniqueBuffer> imgStagingBuf = getStagingImageBuffer(*device, imgWidth, imgHeight, imgCh);
    std::shared_ptr<vk::UniqueDeviceMemory> imgStagingBufMemory = getStagingImageMemory(*device, physicalDevice, *imgStagingBuf);
    writeImageBuffer(*device, *imgStagingBufMemory, imgData, imgWidth, imgHeight, imgCh);
    sendImageBuffer(*device, queueFamilyIndex, graphicsQueue, *imgStagingBuf, *texImage, imgWidth, imgHeight);
    std::shared_ptr<vk::UniqueSampler> texSampler = getSampler(*device);
    std::shared_ptr<vk::UniqueImageView> texImageView = getImageView(*device, *texImage);
    releaseImageData(imgData);

    std::shared_ptr<vk::UniqueBuffer> uniformBuf = getUniformBuffer(*device);
    std::shared_ptr<vk::UniqueDeviceMemory> uniformBufMem = getUniformBufferMemory(*device, physicalDevice, *uniformBuf);
    void* pUniformBufMem = mapUniformBuffer(*device, *uniformBufMem);
    std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> descSetLayouts = getDiscriptorSetLayouts(*device);
    std::shared_ptr<std::vector<vk::DescriptorSetLayout>> unwrapedDescSetLayouts = unwrapHandles<vk::DescriptorSetLayout, vk::UniqueDescriptorSetLayout>(*descSetLayouts);
    std::shared_ptr<vk::UniqueDescriptorPool> descPool = getDescriptorPool(*device);
    std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> descSets = getDescprotorSets(*device, *descPool, *unwrapedDescSetLayouts);
    writeDescriptorSets(*device, *descSets, *uniformBuf, *texImageView, *texSampler);
    std::shared_ptr<std::vector<vk::PushConstantRange>> pushConstantRanges = getPushConstantRanges();

    std::shared_ptr<vk::UniquePipelineLayout> descpriptorPipelineLayout = getDescpriptorPipelineLayout(*device, *unwrapedDescSetLayouts, *pushConstantRanges);
    std::shared_ptr<vk::SurfaceCapabilitiesKHR> surfaceCapabilities = getSurfaceCapabilities(physicalDevice, *surface);
    std::shared_ptr<std::vector<vk::SurfaceFormatKHR>> surfaceFormats = getSurfaceFormats(physicalDevice, *surface);
    std::shared_ptr<std::vector<vk::PresentModeKHR>> surfacePresentModes = getSurfacePresentModes(physicalDevice, *surface);
    vk::SurfaceFormatKHR& surfaceFormat = (*surfaceFormats)[0];
    vk::PresentModeKHR& surfacePresentMode = (*surfacePresentModes)[0];

    std::shared_ptr<std::vector<vk::AttachmentReference>> subpass0_attachmentRefs = getAttachmentReferences();
    std::shared_ptr<vk::AttachmentReference> subpass0_depthStencilAttachmentRef = getDepthStencilAttachmentReference();
    std::shared_ptr<std::vector<vk::SubpassDescription>> subpasses = getSubpassDescription(*subpass0_attachmentRefs, *subpass0_depthStencilAttachmentRef);

    std::shared_ptr<vk::UniqueRenderPass> renderPass = getRenderPass(*device, surfaceFormat, *subpasses);
    std::shared_ptr<vk::UniquePipeline> pipeline = getPipeline(*device, *renderPass, *surfaceCapabilities, *vertexBindingDescription, *vertexInputDescription, *descpriptorPipelineLayout);

    std::shared_ptr<vk::UniqueSwapchainKHR> swapchain;
    std::shared_ptr<std::vector<vk::Image>> swapchainImages;
    std::shared_ptr<std::vector<vk::UniqueImageView>> swapchainImageViews;
    std::shared_ptr<vk::UniqueImage> depthImage;
    std::shared_ptr<vk::UniqueDeviceMemory> depthImageMemory;
    std::shared_ptr<vk::UniqueImageView> depthImageView;
    std::shared_ptr<std::vector<vk::UniqueFramebuffer>> swapchainFramebufs;

    auto recreateSwapchain = [&]()
    {
        if (swapchainFramebufs)
        {
            swapchainFramebufs->clear();
        }
        if (depthImageView)
        {
            depthImageView->reset();
        }
        if (depthImageMemory)
        {
            depthImageMemory->reset();
        }
        if (depthImage)
        {
            depthImage->reset();
        }
        if (swapchainImageViews)
        {
            swapchainImageViews->clear();
        }
        if (swapchainImages)
        {
            swapchainImages->clear();
        }
        if (swapchain)
        {
            swapchain->reset();
        }

        swapchain = getSwapchain(*device, physicalDevice, *surface, *surfaceCapabilities, surfaceFormat, surfacePresentMode);
        swapchainImages = getSwapchainImages(*device, *swapchain);
        swapchainImageViews = getSwapchainImageViews(*device, *swapchain, *swapchainImages, surfaceFormat);
        depthImage = getDepthImage(*device, physicalDevice, *surfaceCapabilities);
        depthImageMemory = getDepthImageMemory(*device, physicalDevice, *depthImage);
        depthImageView = getDepthImageView(*device, *renderPass, *depthImage);
        swapchainFramebufs = getFramebuffers(*device, *renderPass, *swapchainImageViews, *surfaceCapabilities, *depthImageView);
    };

    recreateSwapchain();

    std::shared_ptr<vk::UniqueCommandPool> cmdPool = getCommandPool(*device, queueFamilyIndex);
    std::shared_ptr<std::vector<vk::UniqueCommandBuffer>> cmdBufs = getCommandBuffer(*device, *cmdPool);

    vk::SemaphoreCreateInfo semaphoreCreateInfo;

    vk::UniqueSemaphore swapchainImgSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);
    vk::UniqueSemaphore imgRenderedSemaphore = device->get().createSemaphoreUnique(semaphoreCreateInfo);

    vk::FenceCreateInfo fenceCreateInfo;
    fenceCreateInfo.flags = vk::FenceCreateFlagBits::eSignaled;
    vk::UniqueFence imgRenderedFence = device->get().createFenceUnique(fenceCreateInfo);

    int deltaTime = 0;
    std::chrono::system_clock::time_point sT;

    graphicsQueue.waitIdle();
}

void terminateVulkan() {

}

void handle_cmd(android_app *pApp, int32_t cmd)
{
    // auto& window = GetAppWindow();

    switch (cmd) {
        case APP_CMD_INIT_WINDOW:
            initVulkan(pApp);
            // if(theApp) {
            //     if (theApp->IsInitialized())
            //     {
            //         theApp->Shutdown();
            //     }
            //     theApp->Initialize();
            // }
            break;

        case APP_CMD_TERM_WINDOW:
            terminateVulkan();
            // theApp->Shutdown();
            break;

        case APP_CMD_WINDOW_RESIZED:
            // if (theApp)
            // {
                // auto width = ANativeWindow_getWidth(pApp->window);
                // auto height = ANativeWindow_getHeight(pApp->window);
            //     theApp->SurfaceSizeChanged();
            // }
            break;
        default:
            break;
    }
}

int32_t handle_input(struct android_app* app)
{
    auto inputBuffer = android_app_swap_input_buffers(app);
    if ( inputBuffer == nullptr )
    {
        return 0;
    }

    int32_t processed = 0;
    for (int i = 0; i < inputBuffer->motionEventsCount; ++i)
    {
        auto& event = inputBuffer->motionEvents[i];
        auto action = event.action;
        auto pointerIndex = (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
        auto& pointer = event.pointers[pointerIndex];
        auto x= GameActivityPointerAxes_getX(&pointer);
        auto y = GameActivityPointerAxes_getY(&pointer);
        auto scrollH = GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_HSCROLL);
        auto scrollV = GameActivityPointerAxes_getAxisValue(&pointer, AMOTION_EVENT_AXIS_VSCROLL);

        switch(action & AMOTION_EVENT_ACTION_MASK)
        {
            case AMOTION_EVENT_ACTION_DOWN:
            case AMOTION_EVENT_ACTION_UP:
                // io.AddMousePosEvent(x, y);
                // io.AddMouseButtonEvent(0, action == AMOTION_EVENT_ACTION_DOWN);
                processed = 1;
                break;

            case AMOTION_EVENT_ACTION_HOVER_MOVE:
            case AMOTION_EVENT_ACTION_MOVE:
                // io.AddMousePosEvent(x, y);
                processed = 1;
                break;

            default:
                break;
        }
    }
    android_app_clear_motion_events(inputBuffer);
    return processed;
}

bool motion_event_filter_func(const GameActivityMotionEvent *motionEvent) {
    auto sourceClass = motionEvent->source & AINPUT_SOURCE_CLASS_MASK;
    return (sourceClass == AINPUT_SOURCE_CLASS_POINTER ||
            sourceClass == AINPUT_SOURCE_CLASS_JOYSTICK);
}

extern "C"
{
void android_main(struct android_app *pApp)
{
    pApp->onAppCmd = handle_cmd;
    android_app_set_motion_event_filter(pApp, motion_event_filter_func);

    int events;
    android_poll_source *pSource;

    while(!pApp->destroyRequested)
    {

        if (ALooper_pollOnce(0, nullptr, &events, (void **) &pSource) >= 0)
        {
            if (pSource) {
                pSource->process(pApp, pSource);
            }
        }
        // if (instance && surface)
        {
            // handle_input(pApp); // 入力処理
            // aApp->Process();    // 描画処理
        }
        // if(theApp->IsInitialized())
        {
            handle_input(pApp);
            // theApp->Process();
        }
    }
}
} // extern "C"
