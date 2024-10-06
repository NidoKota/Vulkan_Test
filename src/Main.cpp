#include <vulkan/vulkan.hpp>
#include "Utility.hpp"

using namespace Vulkan_Test;

const char *requiredLayers[] = { "VK_LAYER_KHRONOS_validation" };

// https://chaosplant.tech/do/vulkan/

// cd build
// ."C:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake\bin\cmake.exe" .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake
// VisualStudioでビルド

int main()
{
    // これに色々情報を乗っけることで
    // 例えばVulkanの拡張機能をオンにしたりだとかデバッグ情報を表示させるといったことができる
    // これはオプション的なもの
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.enabledLayerCount = std::size(requiredLayers);
    instCreateInfo.ppEnabledLayerNames = requiredLayers;

    // Vulkanインスタンスを作ってから壊すまでの間に色々な処理を書いていく
    // Vulkanを使う場合、全ての機能はこのインスタンスを介して利用する
    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    // GPUが複数あるなら頼む相手をまず選ぶ必要がある
    // GPUの型式などによってサポートしている機能とサポートしていない機能があったりするため、
    // だいたい「インスタンスを介して物理デバイスを列挙する」→「それぞれの物理デバイスの情報を取得する」→「一番いいのを頼む」という流れになる
    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
        LOG("----------------------------------------");
        LOG(props.deviceName);
    }

    vk::PhysicalDevice physicalDevice;
    bool existsSuitablePhysicalDevice = false;
    uint32_t graphicsQueueFamilyIndex;
    for (size_t i = 0; i < physicalDevices.size(); i++)
    {
        std::vector<vk::QueueFamilyProperties> queueProps = physicalDevices[i].getQueueFamilyProperties();
        bool existsGraphicsQueue = false;

        for (size_t j = 0; j < queueProps.size(); j++)
        {
            if (queueProps[j].queueFlags & vk::QueueFlagBits::eGraphics)
            {
                existsGraphicsQueue = true;
                graphicsQueueFamilyIndex = j;
                break;
            }
        }

        if (existsGraphicsQueue)
        {
            physicalDevice = physicalDevices[i];
            existsSuitablePhysicalDevice = true;
            break;
        }
    }

    if (!existsSuitablePhysicalDevice)
    {
        std::cerr << "No physical devices are available." << std::endl;
        return EXIT_FAILURE;
    }

    // Vulkanにおけるキューとは、GPUの実行するコマンドを保持する待ち行列
    // GPUのやることリスト
    // GPUにコマンドを送るときは、このキューにコマンドを詰め込むことになる
    // ところで、1つのGPUが持っているキューは1つだけとは限らない
    // いくつも持っている場合がある
    // また、それらキューによってサポートしている機能とサポートしていない機能がある
    // キューにコマンドを送るときは、そのキューが何の機能をサポートしているかを事前に把握しておく必要がある
    // ある物理デバイスの持っているキューの情報は getQueueFamilyProperties メソッドで取得できます。
    // メソッド名にある「キューファミリ」というのは、同じ能力を持っているキューをひとまとめにしたものです。
    // 1つの物理デバイスには1個以上のキューファミリがあり、1つのキューファミリには1個以上の同等の機能を持ったキューが所属しています。
    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();
    LOG("----------------------------------------");
    LOG("queue family count: " << queueProps.size());
    for (size_t i = 0; i < queueProps.size(); i++)
    {
        LOG("----------------------------------------");
        LOG("queue family index: " << i);
        LOG("queue count: " << queueProps[i].queueCount);
        LOG("graphic support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics ? "True" : "False"));
        LOG("compute support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute ? "True" : "False"));
        LOG("transfer support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer ? "True" : "False"));
    }

    float queuePriorities[1] = {1.0f};
    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].pQueuePriorities = queuePriorities;
    queueCreateInfo[0].queueCount = 1;

    // 物理デバイスを選んだら次は論理デバイスを作成する
    // VulkanにおいてGPUの能力を使うような機能は、全てこの論理デバイスを通して利用する
    // GPUの能力を使う際、物理デバイスを直接いじることは出来ない
    // コンピュータの仕組みについて多少知識のある人であれば、
    // この「物理」「論理」の区別はメモリアドレスの「物理アドレス」「論理アドレス」と同じ話であることが想像できる
    // マルチタスクOS上で、1つのプロセスが特定のGPU(=物理デバイス)を独占して管理するような状態はよろしくない
    // 仮想化されたデバイスが論理デバイス
    // これならあるプロセスが他のプロセスの存在を意識することなくGPUの能力を使うことができる
    
    // インスタンスを作成するときにvk::InstanceCreateInfo構造体を使ったのと同じように、
    // 論理デバイス作成時にもvk::DeviceCreateInfo構造体の中に色々な情報を含めることができる
    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = std::size(requiredLayers);
    devCreateInfo.ppEnabledLayerNames = requiredLayers;

    devCreateInfo.pQueueCreateInfos = queueCreateInfo;
    devCreateInfo.queueCreateInfoCount = 1;

    // なお、ここで言う「選ぶ」とは、特定のGPUを完全に占有してしまうとかそういう話ではない
    // ちなみにvk::Instanceにはそれに対応するvk::UniqueInstanceが存在しましたが、
    // vk::PhysicalDeviceに対応するvk::UniquePhysicalDevice は存在しない
    // destroyなどを呼ぶ必要もない
    // vk::PhysicalDeviceは単に物理的なデバイスの情報を表しているに過ぎないので、構築したり破棄したりする必要がある類のオブジェクトではない
    vk::UniqueDevice devicePtr = physicalDevice.createDeviceUnique(devCreateInfo);
    vk::Device device = devicePtr.get();


    vk::Queue graphicsQueue = device.getQueue(graphicsQueueFamilyIndex, 0);

    //
    const uint32_t screenWidth = 640;
    const uint32_t screenHeight = 480;

    vk::ImageCreateInfo imgCreateInfo;
    imgCreateInfo.imageType = vk::ImageType::e2D;
    imgCreateInfo.extent = vk::Extent3D(screenWidth, screenHeight, 1);
    imgCreateInfo.mipLevels = 1;
    imgCreateInfo.arrayLayers = 1;
    imgCreateInfo.format = vk::Format::eR8G8B8A8Unorm;
    imgCreateInfo.tiling = vk::ImageTiling::eLinear;
    imgCreateInfo.initialLayout = vk::ImageLayout::eUndefined;
    imgCreateInfo.usage = vk::ImageUsageFlagBits::eColorAttachment;
    imgCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    imgCreateInfo.samples = vk::SampleCountFlagBits::e1;

    vk::UniqueImage image = device.createImageUnique(imgCreateInfo);
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
    //

    vk::CommandPoolCreateInfo cmdPoolCreateInfo;
    cmdPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

    vk::UniqueCommandPool cmdPool = device.createCommandPoolUnique(cmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo cmdBufAllocInfo;
    cmdBufAllocInfo.commandPool = cmdPool.get();
    cmdBufAllocInfo.commandBufferCount = 1;
    cmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;

    std::vector<vk::UniqueCommandBuffer> cmdBufs = device.allocateCommandBuffersUnique(cmdBufAllocInfo);
    vk::MemoryRequirements imgMemReq = device.getImageMemoryRequirements(image.get());

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBufs[0]->begin(cmdBeginInfo);

    // コマンドを記録

    cmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = { cmdBufs[0].get() };
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({ submitInfo }, nullptr);

    graphicsQueue.waitIdle();

    return EXIT_SUCCESS;
}