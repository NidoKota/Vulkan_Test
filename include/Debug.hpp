#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>

namespace Vulkan_Test
{
    void debugApplicationInfo(vk::ApplicationInfo &applicationInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug ApplicationInfo");

        LOG("apiVersion: " << 
            VK_VERSION_MAJOR(applicationInfo.apiVersion) << "." << 
            VK_VERSION_MINOR(applicationInfo.apiVersion) << "." <<
            VK_VERSION_PATCH(applicationInfo.apiVersion));
    }

    void debugInstanceCreateInfo(vk::InstanceCreateInfo &instanceCreateInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug Instance Extensions");
        LOG("enabledExtensionCount: " << instanceCreateInfo.enabledExtensionCount);
        for (int i = 0; i < instanceCreateInfo.enabledExtensionCount; i++)
        {
            LOG("----------------------------------------", 1);
            LOG("    " << instanceCreateInfo.ppEnabledExtensionNames[i], 1);
        }
    }

    void debugGlfwWindow(GLFWwindow &window)
    {
        LOG("----------------------------------------");
        LOG("Debug GLFW Window");
        int width;
        int height;
        glfwGetWindowSize(&window, &width, &height);
        LOG("WindowSize: width " << width << ", height " << height);
        glfwGetFramebufferSize(&window, &width, &height);
        LOG("FramebufferSize: width " << width << ", height " << height);
    }

    // UUIDを16進数で表示する関数
    std::string getUUID(const uint8_t* uuid, size_t size)
    {
        std::stringstream ss;
        for (size_t i = 0; i < size; ++i) 
        {
            ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(uuid[i]);
            if (i < size - 1) 
            {
                ss << ":";
            }
        }
        return ss.str();
    }

    void debugPhysicalDevices(std::vector<vk::PhysicalDevice> &physicalDevices)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Devices");
        LOG("physicalDevicesCount: " << physicalDevices.size());
        for (vk::PhysicalDevice physicalDevice : physicalDevices)
        {
            LOG("----------------------------------------", 1);
            vk::PhysicalDeviceProperties2 props;
            vk::PhysicalDeviceVulkan12Properties props12;
            vk::PhysicalDeviceVulkan11Properties props11;
            props.pNext = &props12;
            props12.pNext = &props11;
            physicalDevice.getProperties2(&props);

            LOG(props.properties.deviceName, 1);
            LOG("deviceID: " << props.properties.deviceID, 1);
            LOG("vendorID: " << props.properties.vendorID, 1);
            LOG("deviceUUID: " << getUUID(props11.deviceUUID, VK_UUID_SIZE), 1);
            LOG("driverName: " << props12.driverName, 1);
            LOG("driverInfo: " << props12.driverInfo, 1);
            LOG("maxMemoryAllocationSize: " << static_cast<unsigned long long>(props11.maxMemoryAllocationSize), 1);
        }
    }

    void debugPhysicalDevice(vk::PhysicalDevice &physicalDevice, uint32_t queueFamilyIndex)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Devices");

        vk::PhysicalDeviceProperties2 props = physicalDevice.getProperties2();
        LOG("Found device and queue");
        LOG("physicalDevice: " << props.properties.deviceName);
        LOG("queueFamilyIndex: " << queueFamilyIndex);
    }

    void debugPhysicalMemory(vk::PhysicalDevice &physicalDevice)
    {
        LOG("----------------------------------------");
        LOG("Debug Physical Memory");
        vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();
        LOG("memory type count: " << memProps.memoryTypeCount);
        LOG("memory heap count: " << memProps.memoryHeapCount);
        for (size_t i = 0; i < memProps.memoryTypeCount; i++)
        {
            LOG("----------------------------------------", 1);
            LOG("memory index: " << i, 1);
            LOG(to_string(memProps.memoryTypes[i].propertyFlags), 1);
        }
    }

    void debugQueueFamilyProperties(std::vector<vk::QueueFamilyProperties> &queueFamilyProperties)
    {
        LOG("----------------------------------------");
        LOG("Debug Queue Family Properties");
        LOG("queue family count: " << queueFamilyProperties.size());
        for (size_t i = 0; i < queueFamilyProperties.size(); i++)
        {
            LOG("----------------------------------------", 1);
            LOG("queue family index: " << i, 1);
            LOG("queue count: " << queueFamilyProperties[i].queueCount, 1);
            LOG(to_string(queueFamilyProperties[i].queueFlags), 1);
        }
    }

    void debugSwapchainCreateInfo(vk::SwapchainCreateInfoKHR& swapchainCreateInfo)
    {
        LOG("----------------------------------------");
        LOG("Debug Swapchain Create Info");
        LOG("swapchain minImageCount " << swapchainCreateInfo.minImageCount);
        LOG("swapchainCreateInfo " << swapchainCreateInfo.imageExtent.width << ", " << swapchainCreateInfo.imageExtent.height);
    }
}
