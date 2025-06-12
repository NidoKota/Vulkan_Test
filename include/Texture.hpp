#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb/stb_image.h"
#include "Utility.hpp"
#include "Debug.hpp"

void* getImageData(int* imgWidth, int* imgHeight, int* imgCh)
{
    std::filesystem::path currentPath = std::filesystem::current_path();
    std::filesystem::path imagePath = currentPath / ".." / "assets" / "image.jpg";
    std::string imagePathStr = imagePath.string();

    LOG("load image : " << imagePathStr);
    
    // Vulkanに画像データを渡すには圧縮されていない生データでなければない
    stbi_uc* pImgData = stbi_load(imagePathStr.c_str(), imgWidth, imgHeight, nullptr, STBI_rgb_alpha);
    *imgCh = 4;

    if (pImgData == nullptr) 
    {
       LOG("Failed to load image file." << imagePathStr);
       exit(EXIT_FAILURE);
    }

    return pImgData;
}

void releaseImageData(void* pImgData)
{
    stbi_image_free(pImgData);
}
