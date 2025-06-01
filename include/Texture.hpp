#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <GLFW/glfw3.h>
#include "Utility.hpp"
#include "Debug.hpp"

#define STB_IMAGE_IMPLEMENTATION
#include "../include/stb/stb_image.h"

void* getImageData(int* imgWidth, int* imgHeight, int* imgCh)
{
    stbi_uc* pImgData = stbi_load("image.jpg", imgWidth, imgHeight, imgCh, STBI_rgb_alpha);

    if (pImgData == nullptr) 
    {
       std::cerr << "画像ファイルの読み込みに失敗しました。" << std::endl;
       exit(EXIT_FAILURE);
    }

    return pImgData;
}

void releaseImageData(void* pImgData)
{
    stbi_image_free(pImgData);
}
