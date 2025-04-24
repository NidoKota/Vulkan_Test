#pragma once

#include <iostream>
#include <vulkan/vulkan.hpp>
#include "Utility.hpp"
#include "Debug.hpp"

using namespace Vulkan_Test;

struct Vec2 {
    float x, y;
};

struct Vec3 {
    float x, y, z;
};

struct Vertex {
    Vec2 pos;
    Vec3 color;
};

std::vector<Vertex> vertices = {
    Vertex{ Vec2{-0.5f, -0.5f }, Vec3{ 0.0, 0.0, 1.0 } },
    Vertex{ Vec2{ 0.5f,  0.5f }, Vec3{ 0.0, 1.0, 0.0 } },
    Vertex{ Vec2{-0.5f,  0.5f }, Vec3{ 1.0, 0.0, 0.0 } },
    Vertex{ Vec2{ 0.5f, -0.5f }, Vec3{ 1.0, 1.0, 1.0 } },
};

std::vector<uint16_t> indices = { 0, 1, 2, 1, 0, 3 };

std::shared_ptr<vk::UniqueBuffer> getVertexBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    // 次は実際に使われる頂点バッファの作成
    // 今までと違い、メモリの確保時にvk::MemoryPropertyFlagBits::eDeviceLocalフラグを持ったメモリを使うようにする
    // 逆にeHostVisibleは要らない
    // また、usageにvk::BufferUsageFlagBits::eTransferDstを追加で指定する
    // データの転送先という意味
    // あとでステージングバッファからデータを転送してくる
    vk::BufferCreateInfo vertexBufferCreateInfo;
    vertexBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    vertexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst; // eTransferDstを追加
    vertexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(vertexBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getVertexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& vertexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements vertexBufMemReq = device.get().getBufferMemoryRequirements(vertexBuf.get());
    
    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    device.get().bindBufferMemory(vertexBuf.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingVertexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    // バッファというのはデバイスメモリ上のデータ列を表すオブジェクト
    // 何度も言うようにGPUから普通のメモリの内容は参照できない
    // シェーダで使いたいデータがある場合、まずデバイスメモリに移す必要がある
    // そしてそれはプログラムの上では「バッファに書き込む」「バッファに書き込んで転送する」という形になる
    // 頂点座標のデータをシェーダに送るためのバッファを作成する
    // いわゆる頂点バッファ
    // size はバッファの大きさをバイト数で示すもの
    // ここに例えば100という値を指定すれば、100バイトの大きさのバッファが作成できる
    // ここでは前節で定義した構造体のバイト数をsizeof演算子で取得し、それにデータの数をかけている
    // usage は作成するバッファの使い道を示すためのもの
    // 今回のように頂点バッファを作る場合、上記のようにvk::BufferUsageFlagBits::eVertexBufferフラグを指定しなければならない
    // 他にも場合によって様々なフラグを指定する必要があり、複数のフラグを指定することもある
    // sharingModeについては今のところは無視
    // ここではvk::SharingMode::eExclusiveを指定
    vk::BufferCreateInfo stagingBufferCreateInfo;
    stagingBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    stagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(stagingBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getStagingVertexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& stagingVertexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements vertexBufMemReq = device.get().getBufferMemoryRequirements(stagingVertexBuf.get());
    
    vk::MemoryAllocateInfo vertexBufMemAllocInfo;
    vertexBufMemAllocInfo.allocationSize = vertexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) // デバイスローカルに変更
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    device.get().bindBufferMemory(stagingVertexBuf.get(), result->get(), 0);
    return result;
}

void writeStagingVertexBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& stagingVertexBufMem)
{
    void* pStagingVertexBufMem = device.get().mapMemory(stagingVertexBufMem.get(), 0, sizeof(Vertex) * vertices.size());

    std::memcpy(pStagingVertexBufMem, vertices.data(), sizeof(Vertex) * vertices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = stagingVertexBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(Vertex) * vertices.size();
    
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    device.get().unmapMemory(stagingVertexBufMem.get());
}

void sendVertexBuffer(vk::UniqueDevice& device, uint32_t queueFamilyIndex, vk::Queue& graphicsQueue, vk::UniqueBuffer& stagingBuf, vk::UniqueBuffer& vertexBuf)
{
    // こちらはメモリマッピングではデータを入れられない ホスト可視でないため
    // ホスト可視でないメモリはCPUからは触れない
    // GPUにコピーさせる訳だが、GPUに命令するということは例によってコマンドバッファとキューを使う
    // データの転送用に、コマンドバッファを作成するコードを別途追加する
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    // 今までのコードと違う点として、コマンドプールにvk::CommandPoolCreateFlagBits::eTransientフラグを指定する
    // これは比較的すぐに使ってすぐに役目を終えるコマンドバッファ用であることを意味するフラグ
    // 必須ではないですが指定しておくと内部的に最適化が起きる可能性がある
    // このコマンドバッファを使ってデータ転送の命令を飛ばす
    vk::BufferCopy bufCopy;
    bufCopy.srcOffset = 0;
    bufCopy.dstOffset = 0;
    bufCopy.size = sizeof(Vertex) * vertices.size();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    tmpCmdBufs[0]->copyBuffer(stagingBuf.get(), vertexBuf.get(), {bufCopy});
    tmpCmdBufs[0]->end();

    // バッファ間でデータをコピーするには copyBuffer を使います。そしてコピーするバッファやコピーする領域を指定するために vk::BufferCopy 構造体を使います。
    // srcOffsetは転送元バッファの先頭から何バイト目を読み込むというデータ位置、dstOffsetは転送先バッファの先頭から何バイト目に書き込むというデータ位置、sizeはデータサイズを表します。memcpyの引数と似たような感じだと理解すると分かりやすいかもしれません。
    // あとはキューに投げるだけです。
    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({submitInfo});
    graphicsQueue.waitIdle();
}

std::shared_ptr<vk::UniqueBuffer> getIndexBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::BufferCreateInfo indexBufferCreateInfo;
    indexBufferCreateInfo.size = sizeof(uint16_t) * indices.size();
    indexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eIndexBuffer | vk::BufferUsageFlagBits::eTransferDst; 
    indexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(indexBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getIndexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& indexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements indexBufMemReq = device.get().getBufferMemoryRequirements(indexBuf.get());
    
    vk::MemoryAllocateInfo indexBufMemAllocInfo;
    indexBufMemAllocInfo.allocationSize = indexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (indexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eDeviceLocal))
        {
            indexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(indexBufMemAllocInfo);
    device.get().bindBufferMemory(indexBuf.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingIndexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::BufferCreateInfo stagingBufferCreateInfo;
    stagingBufferCreateInfo.size = sizeof(uint16_t) * indices.size();
    stagingBufferCreateInfo.usage = vk::BufferUsageFlagBits::eTransferSrc;
    stagingBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(stagingBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getStagingIndexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& stagingIndexBuf)
{
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements indexBufMemReq = device.get().getBufferMemoryRequirements(stagingIndexBuf.get());
    
    vk::MemoryAllocateInfo indexBufMemAllocInfo;
    indexBufMemAllocInfo.allocationSize = indexBufMemReq.size;
    
    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (indexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
        {
            indexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        std::cerr << "適切なメモリタイプが存在しません。" << std::endl;
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(indexBufMemAllocInfo);
    device.get().bindBufferMemory(stagingIndexBuf.get(), result->get(), 0);
    return result;
}

void writeStagingIndexBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& stagingIndexBufMem)
{
    void* pStagingIndexBufMem = device.get().mapMemory(stagingIndexBufMem.get(), 0, sizeof(uint16_t) * indices.size());

    std::memcpy(pStagingIndexBufMem, indices.data(), sizeof(uint16_t) * indices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = stagingIndexBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(uint16_t) * indices.size();
    
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    device.get().unmapMemory(stagingIndexBufMem.get());
}

void sendIndexBuffer(vk::UniqueDevice& device, uint32_t queueFamilyIndex, vk::Queue& graphicsQueue, vk::UniqueBuffer& stagingBuf, vk::UniqueBuffer& indexBuf)
{
    vk::CommandPoolCreateInfo tmpCmdPoolCreateInfo;
    tmpCmdPoolCreateInfo.queueFamilyIndex = queueFamilyIndex;
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    vk::BufferCopy bufCopy;
    bufCopy.srcOffset = 0;
    bufCopy.dstOffset = 0;
    bufCopy.size = sizeof(uint16_t) * indices.size();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    tmpCmdBufs[0]->copyBuffer(stagingBuf.get(), indexBuf.get(), {bufCopy});
    tmpCmdBufs[0]->end();

    vk::CommandBuffer submitCmdBuf[1] = {tmpCmdBufs[0].get()};
    vk::SubmitInfo submitInfo;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = submitCmdBuf;

    graphicsQueue.submit({submitInfo});
    graphicsQueue.waitIdle();
}

std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> getVertexBindingDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputBindingDescription>> result = std::make_shared<std::vector<vk::VertexInputBindingDescription>>();

    // このままではバッファがどういう形式で頂点のデータを持っているのか、Vulkanのシステム側には分からない
    // Vertex構造体というこちらが勝手に定義してみただけの構造体のことは知りようがない
    // このままではシェーダに読み込ませられない
    // そこで頂点入力デスクリプションというものを用意する
    // 適切な説明(デスクリプション)を与えることで「こういう形式でこういうデータが入っている」という情報を与えることができる
    // データだけでなく説明を与えて初めてシェーダからデータを読み込める

    // 頂点入力デスクリプションには2種類ある
    // アトリビュートデスクリプション(vk::VertexInputAttributeDescription)
    // バインディングデスクリプション(vk::VertexInputBindingDescription)
    // これはひとえに、シェーダに与えるデータを分ける単位が「バインディング」と「アトリビュート」の2段階あるため

    // バインディングが一番上の単位
    // 基本的に1つのバインディングに対して1つのバッファ(の範囲)が結び付けられる
    // あるバインディングに含まれるデータは一定の大きさごとに切り分けられ、各頂点のデータとして解釈される
    // アトリビュートはより細かい単位になる
    // 1つの頂点のデータは、1つまたは複数のアトリビュートとして分けられる
    // 一つの頂点データは「座標」とか「色」みたいな複数のデータを内包していますが、この1つ1つのデータが1つのアトリビュートに相当することになる
    // (注意点として、1つのアトリビュートは「多次元ベクトル」や「行列」だったりできるので、アトリビュートは「x座標」とか「y座標」みたいな単位よりはもう少し大きい単位になる)

    // バインディングデスクリプションは、1つのバインディングのデータをどうやって各頂点のデータに分割するかという情報を設定する。
    // 具体的には、1つの頂点データのバイト数などを設定する。
    // そしてアトリビュートデスクリプションは、1つの頂点データのどの位置からどういう形式でアトリビュートのデータを取り出すかという情報を設定する。
    // アトリビュートデスクリプションはアトリビュートの数だけ作成する。
    // 1つの頂点データが3種類のデータを含んでいるならば、3つのアトリビュートデスクリプションを作らなければならない。
    // 今回のデータだと含んでいるデータは「座標」だけなので1つだけになるが、複数のアトリビュートを含む場合はその数だけ作成する。

    // bindingは説明の対象となるバインディングの番号である。各頂点入力バインディングには0から始まる番号が振ってある。ここでは0番を使うことにする。
    // strideは1つのデータを取り出す際の刻み幅である。つまり上の図でいう所の「1つの頂点データ」の大きさである。ここではVertex構造体のバイト数を指定している。
    // 前節までで用意したデータはVertex構造体が並んでいるわけなので、各データを取り出すには当然Vertex構造体のバイト数ずつずらして読み取ることになる。
    // inputRateにはvk::VertexInputRate::eVertexを指定する。1頂点ごとにデータを読み込む、というだけの意味である。インスタンス化というものを行う場合に別の値を指定する。
    vk::VertexInputBindingDescription vertexBindingDescription;
    vertexBindingDescription.binding = 0;
    vertexBindingDescription.stride = sizeof(Vertex);
    vertexBindingDescription.inputRate = vk::VertexInputRate::eVertex;

    (*result).push_back(vertexBindingDescription);
    return result;
}

std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> getVertexInputDescription()
{
    std::shared_ptr<std::vector<vk::VertexInputAttributeDescription>> result = std::make_shared<std::vector<vk::VertexInputAttributeDescription>>();

    // アトリビュートの読み込み方の説明情報
    // 今度は vk::VertexInputAttributeDescription を用意する
    // 今回用意した頂点データが含む情報は「座標」だけなので、用意するアトリビュートデスクリプションは1つだけ
    //
    // bindingはデータの読み込み元のバインディングの番号を指定。上で0番のバインディングを使うことにしたので、ここでも0にする。
    // locationはシェーダにデータを渡す際のデータ位置。
    //
    // 頂点データの用意に書いたシェーダのコードを見てみる
    // layout(location = 0) in vec2 inPos;
    // location = 0という部分がある。シェーダがアトリビュートのデータを受け取る際はこのようにデータ位置を指定する。
    // このlayout(location = xx)の指定とアトリビュートデスクリプションのlocationの位置は対応付けて書かなければならない。
    // locationの対応付けによって、シェーダで読み込む変数とVulkanが用意したアトリビュートの対応が付くのである。
    //
    // formatはデータ形式である。今回渡すのは32bitのfloat型が2つある2次元ベクトルなので、それを表すeR32G32Sfloatを指定する。
    // ここで使われているvk::Formatは本来ピクセルの色データのフォーマットを表すものなのでRとかGとか入っているが、それらはここでは無視してほしい。
    // ここでは 「32bit, 32bit, それぞれSigned(符号付)floatである」という意味を表すためだけにこれが指定されている。
    // ちなみにfloat型の3次元ベクトルを渡す際にはeR32G32B32Sfloatとか指定する。ここでもRGBの文字に深い意味はない。
    // 色のデータを渡すにしろ座標データを渡すにしろこういう名前の値を指定する。違和感があるかもしれないが、こういうものなので仕方ない。
    // offsetは頂点データのどの位置からデータを取り出すかを示す値。今回は1つしかアトリビュートが無いので0を指定しているが、複数のアトリビュートがある場合にはとても重要なものである。
    (*result).push_back(vk::VertexInputAttributeDescription());
    (*result).push_back(vk::VertexInputAttributeDescription());

    (*result)[0].binding = 0;
    (*result)[0].location = 0;
    (*result)[0].format = vk::Format::eR32G32Sfloat;
    (*result)[0].offset = offsetof(Vertex, pos);
    
    (*result)[1].binding = 0;
    (*result)[1].location = 1;
    (*result)[1].format = vk::Format::eR32G32B32Sfloat;
    (*result)[1].offset = offsetof(Vertex, color);

    return result;
}