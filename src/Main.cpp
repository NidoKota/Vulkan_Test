#include <vulkan/vulkan.hpp>
#include "Utility.hpp"

using namespace Vulkan_Test;

const char *requiredLayers[] = {"VK_LAYER_KHRONOS_validation"};

int main()
{
    vk::InstanceCreateInfo instCreateInfo;
    instCreateInfo.enabledLayerCount = std::size(requiredLayers);
    instCreateInfo.ppEnabledLayerNames = requiredLayers;

    vk::UniqueInstance instancePtr = vk::createInstanceUnique(instCreateInfo);
    vk::Instance instance = instancePtr.get();

    std::vector<vk::PhysicalDevice> physicalDevices = instance.enumeratePhysicalDevices();
    for (vk::PhysicalDevice physicalDevice : physicalDevices)
    {
        vk::PhysicalDeviceProperties props = physicalDevice.getProperties();
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

    std::vector<vk::QueueFamilyProperties> queueProps = physicalDevice.getQueueFamilyProperties();

    // LOG("queue family count: " << queueProps.size());
    // for (size_t i = 0; i < queueProps.size(); i++)
    // {
    //     LOG("----------------------------------------");
    //     LOG("queue family index: " << i);
    //     LOG("queue count: " << queueProps[i].queueCount);
    //     LOG("graphic support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eGraphics ? "True" : "False"));
    //     LOG("compute support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eCompute ? "True" : "False"));
    //     LOG("transfer support: " << (queueProps[i].queueFlags & vk::QueueFlagBits::eTransfer ? "True" : "False"));
    // }

    float queuePriorities[1] = {1.0f};

    vk::DeviceQueueCreateInfo queueCreateInfo[1];
    queueCreateInfo[0].queueFamilyIndex = graphicsQueueFamilyIndex;
    queueCreateInfo[0].pQueuePriorities = queuePriorities;
    queueCreateInfo[0].queueCount = 1;

    vk::DeviceCreateInfo devCreateInfo;
    devCreateInfo.enabledLayerCount = std::size(requiredLayers);
    devCreateInfo.ppEnabledLayerNames = requiredLayers;

    devCreateInfo.pQueueCreateInfos = queueCreateInfo;
    devCreateInfo.queueCreateInfoCount = 1;

    vk::UniqueDevice device = physicalDevice.createDeviceUnique(devCreateInfo);

    vk::Queue graphicsQueue = device->getQueue(graphicsQueueFamilyIndex, 0);

    return EXIT_SUCCESS;
}