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

vk::Instance instance = nullptr;
vk::SurfaceKHR surface = nullptr;
vk::DispatchLoaderDynamic dldi;

void initVulkan(android_app* pApp) {
    if (!pApp->window) {
        LOG("ウィンドウがないのでVulkanを初期化できません");
        return;
    }

    std::shared_ptr<vk::ApplicationInfo> appInfo = getAppInfo();
    debugApplicationInfo(*appInfo);
    LOG("ApplicationInfoの作成");

    std::shared_ptr<vk::UniqueInstance> instance = getInstance(*appInfo);
    dldi.init((*instance).get(), vkGetInstanceProcAddr);
    std::shared_ptr<vk::UniqueSurfaceKHR> surface = getSurface(*instance, pApp->window);
    LOG("Instanceの作成");

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

    LOG("Vulkanのセットアップ完了");
}

void terminateVulkan() {
    if (instance) {
        if (surface) {
            // destroySurfaceKHRもExtension関数
            instance.destroySurfaceKHR(surface, nullptr, dldi);
            LOG("Vulkanサーフェスを破棄しました。");
        }
        instance.destroy();
        LOG("Vulkanインスタンスを破棄しました。");
    }
    surface = nullptr;
    instance = nullptr;
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
    LOG("android_main 起動");

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
        if (instance && surface)
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
