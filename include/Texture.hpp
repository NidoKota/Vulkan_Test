#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb/stb_image.h"
#include "Utility.hpp"
#include "Debug.hpp"

void* getImageData(int* imgWidth, int* imgHeight, int* imgCh)
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path imagePath = currentPath / ".." / "assets" / "image.jpg";
    
    // Vulkanに画像データを渡すには圧縮されていない生データでなければない
    stbi_uc* pImgData = stbi_load(imagePath.c_str(), imgWidth, imgHeight, nullptr, STBI_rgb_alpha);
    *imgCh = 4;

    if (pImgData == nullptr) 
    {
       std::cerr << "Failed to load image file." << std::endl;
       exit(EXIT_FAILURE);
    }

    return pImgData;
}

void releaseImageData(void* pImgData)
{
    stbi_image_free(pImgData);
}
