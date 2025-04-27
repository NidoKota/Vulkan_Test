#pragma once

#include <iostream>
#include <memory>
#include <vulkan/vulkan.hpp>
#include <fstream>
#include <filesystem>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

std::shared_ptr<vk::UniquePipeline> getPipeline(
    vk::UniqueDevice& device, 
    vk::UniqueRenderPass& renderpass, 
    vk::SurfaceCapabilitiesKHR& surfaceCapabilities, 
    std::vector<vk::VertexInputBindingDescription>& vertexBindingDescription, 
    std::vector<vk::VertexInputAttributeDescription>& vertexInputDescription,
    vk::UniquePipelineLayout& pipelineLayout)
{
    std::shared_ptr<vk::UniquePipeline> result = std::make_shared<vk::UniquePipeline>();

    // 2種類の頂点入力デスクリプションを作成したら、それをパイプラインに設定する
    // 頂点入力デスクリプションはvk::PipelineVertexInputStateCreateInfo構造体に設定する
    vk::PipelineVertexInputStateCreateInfo vertexInputInfo;
    vertexInputInfo.vertexBindingDescriptionCount = vertexBindingDescription.size();
    vertexInputInfo.pVertexBindingDescriptions = vertexBindingDescription.data();
    vertexInputInfo.vertexAttributeDescriptionCount = vertexInputDescription.size();
    vertexInputInfo.pVertexAttributeDescriptions = vertexInputDescription.data();
    
    // パイプラインとは、3DCGの基本的な描画処理をひとつながりにまとめたもの
    // パイプラインは「点の集まりで出来た図形を色のついたピクセルの集合に変換するもの」
    // ほぼ全ての3DCGは三角形の集まりであり、私たちが最初に持っているものは三角形の各点の色や座標であるが、
    // 最終的に欲しいものは画面のどのピクセルがどんな色なのかという情報である
    // この間を繋ぐ演算処理は大体お決まりのパターンになっており、まとめてグラフィックスパイプラインとなっている

    // この処理は全ての部分が固定されているものではなく、プログラマ側で色々指定する部分があり、
    // それらの情報をまとめたものがパイプラインオブジェクト(vk::Pipeline)である
    // 実際に使用して描画処理を行う際はコマンドでパイプラインをバインドし、ドローコールを呼ぶ

    // Vulkanにおけるパイプラインには「グラフィックスパイプライン」と「コンピュートパイプライン」の2種類がある
    // コンピュートパイプラインはGPGPUなどに使うもの
    // 今回は普通に描画が目的なのでグラフィックスパイプラインを作成する
    // グラフィックスパイプラインはvk::DeviceのcreateGraphicsPipelineメソッドで作成できる
    
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

    vk::UniqueShaderModule vertShader;
    vk::UniqueShaderModule fragShader;
    {
        // 頂点シェーダーを読み込む
        size_t vertSpvFileSz = std::filesystem::file_size("../src/shader.vert.spv");
        std::ifstream vertSpvFile = std::ifstream("../src/shader.vert.spv", std::ios_base::binary);
        std::vector<char> vertSpvFileData = std::vector<char>(vertSpvFileSz);
        vertSpvFile.read(vertSpvFileData.data(), vertSpvFileSz);

        vk::ShaderModuleCreateInfo vertShaderCreateInfo;
        vertShaderCreateInfo.codeSize = vertSpvFileSz;
        vertShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(vertSpvFileData.data());
        vertShader = device.get().createShaderModuleUnique(vertShaderCreateInfo);

        // フラグメントシェーダーを読み込む
        size_t fragSpvFileSz = std::filesystem::file_size("../src/shader.frag.spv");
        std::ifstream fragSpvFile = std::ifstream("../src/shader.frag.spv", std::ios_base::binary);
        std::vector<char> fragSpvFileData = std::vector<char>(fragSpvFileSz);
        fragSpvFile.read(fragSpvFileData.data(), fragSpvFileSz);

        vk::ShaderModuleCreateInfo fragShaderCreateInfo;
        fragShaderCreateInfo.codeSize = fragSpvFileSz;
        fragShaderCreateInfo.pCode = reinterpret_cast<const uint32_t*>(fragSpvFileData.data());
        fragShader = device.get().createShaderModuleUnique(fragShaderCreateInfo);
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

    *result = device.get().createGraphicsPipelineUnique(nullptr, pipelineCreateInfo).value;
    return result;
}