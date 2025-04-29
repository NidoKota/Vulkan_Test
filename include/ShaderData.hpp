#pragma once

#include <iostream>
#include <memory>
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

struct Mat4x4 {
    float v[4][4];
};

struct Vertex {
    Vec3 pos;
    Vec3 color;
};

struct SceneData {
    Vec2 rectCenter;
};

struct CameraData {
    Mat4x4 mvpMatrix;
};

Mat4x4 operator*(const Mat4x4 &a, const Mat4x4 &b) {
    Mat4x4 c = {};
    for(int i = 0; i < 4; i++)
        for(int j = 0; j < 4; j++)
            for(int k = 0; k < 4; k++)
                c.v[i][j] += a.v[k][j] * b.v[i][k];
    return c;
}

std::vector<Vertex> vertices = {
    Vertex{Vec3{-0.5f, -0.5f, 0.5f}, Vec3{0.0, 0.0, 1.0}},
    Vertex{Vec3{0.5f, 0.5f, 0.5f}, Vec3{0.0, 1.0, 0.0}},
    Vertex{Vec3{0.5f, -0.5f, 0.5f}, Vec3{1.0, 1.0, 1.0}},
    Vertex{Vec3{-0.5f, 0.5f, 0.5f}, Vec3{1.0, 0.0, 0.0}},
    
    Vertex{Vec3{-0.5f, -0.5f, -0.5f}, Vec3{0.0, 0.0, 1.0}},
    Vertex{Vec3{0.5f, 0.5f, -0.5f}, Vec3{0.0, 1.0, 0.0}},
    Vertex{Vec3{0.5f, -0.5f, -0.5f}, Vec3{1.0, 1.0, 1.0}},
    Vertex{Vec3{-0.5f, 0.5f, -0.5f}, Vec3{1.0, 0.0, 0.0}},
};

std::vector<uint16_t> indices = {
    0, 1, 2, 1, 0, 3, 5, 4, 6, 4, 5, 7, 
    4, 3, 0, 3, 4, 7, 1, 6, 2, 6, 1, 5,
    7, 1, 3, 1, 7, 5, 6, 0, 2, 0, 6, 4
};

SceneData sceneData;
CameraData cameraData;

std::shared_ptr<vk::UniqueBuffer> getVertexBuffer(vk::UniqueDevice& device)
{
    // バッファというのはデバイスメモリ上のデータ列を表すオブジェクト
    // 何度も言うようにGPUから普通のメモリの内容は参照できない
    // シェーダで使いたいデータがある場合、まずデバイスメモリに移す必要がある
    // そしてそれはプログラムの上では「バッファに書き込む」「バッファに書き込んで転送する」という形になる
    // 頂点座標のデータをシェーダに送るためのバッファを作成する
    // いわゆる頂点バッファ
    // size はバッファの大きさをバイト数で示すもの
    // ここに例えば100という値を指定すれば、100バイトの大きさのバッファが作成できる
    // ここでは前節で定義した構造体のバイト数をsizeof演算子で取得し、それにデータの数をかけている
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    // 次は実際に使われる頂点バッファの作成
    // 今までと違い、メモリの確保時にvk::MemoryPropertyFlagBits::eDeviceLocalフラグを持ったメモリを使うようにする
    // 逆にeHostVisibleは要らない
    // また、usageにvk::BufferUsageFlagBits::eTransferDstを追加で指定する
    // データの転送先という意味
    // あとでステージングバッファからデータを転送してくる
    vk::BufferCreateInfo vertexBufferCreateInfo;
    vertexBufferCreateInfo.size = sizeof(Vertex) * vertices.size();
    // usage は作成するバッファの使い道を示すためのもの
    // 今回のように頂点バッファを作る場合、上記のようにvk::BufferUsageFlagBits::eVertexBufferフラグを指定しなければならない
    // 他にも場合によって様々なフラグを指定する必要があり、複数のフラグを指定することもある
    vertexBufferCreateInfo.usage = vk::BufferUsageFlagBits::eVertexBuffer | vk::BufferUsageFlagBits::eTransferDst; // eTransferDstを追加
    // sharingModeについては今のところは無視
    // ここではvk::SharingMode::eExclusiveを指定
    vertexBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;
    
    *result = device.get().createBufferUnique(vertexBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getVertexBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& vertexBuf)
{
    // 実際に使われる頂点バッファのデバイスメモリを確保する

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
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    // デバイスメモリが確保出来たら bindBufferMemoryで結び付ける
    // 第1引数は結びつけるバッファ、第2引数は結びつけるデバイスメモリ
    // 第3引数は、確保したデバイスメモリのどこを(先頭から何バイト目以降を)使用するかを指定するもの
    device.get().bindBufferMemory(vertexBuf.get(), result->get(), 0);
    return result;
}

std::shared_ptr<vk::UniqueBuffer> getStagingVertexBuffer(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();

    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

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
        if (vertexBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible))
        {
            vertexBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }
    
    *result = device.get().allocateMemoryUnique(vertexBufMemAllocInfo);
    device.get().bindBufferMemory(stagingVertexBuf.get(), result->get(), 0);
    return result;
}

void writeStagingVertexBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& stagingVertexBufMem)
{
    // デバイスメモリに書き込むために、メモリマッピングというものをする
    // これは操作したい対象のデバイスメモリを仮想的にアプリケーションのメモリ空間に対応付けることで操作出来るようにするもの
    // 対象のデバイスメモリを直接操作するわけにはいかないのでこういう形になっている
    void* pStagingVertexBufMem = device.get().mapMemory(stagingVertexBufMem.get(), 0, sizeof(Vertex) * vertices.size());

    std::memcpy(pStagingVertexBufMem, vertices.data(), sizeof(Vertex) * vertices.size());

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = stagingVertexBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(Vertex) * vertices.size();
    
    // 書き込んだら flushMappedMemoryRangesメソッドを呼ぶことで書き込んだ内容がデバイスメモリに反映される
    // マッピングされたメモリはあくまで仮想的にデバイスメモリと対応付けられているだけ
    // 「同期しておけよ」と念をおさないとデータが同期されない可能性がある
    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    // 作業が終わった後はunmapMemoryできちんと後片付けをする
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
    // vk::CommandPoolCreateFlagBits::eTransientフラグを指定
    // これは比較的すぐに使ってすぐに役目を終えるコマンドバッファ用であることを意味するフラグ
    // 必須ではないが指定しておくと内部的に最適化が起きる可能性がある
    tmpCmdPoolCreateInfo.flags = vk::CommandPoolCreateFlagBits::eTransient;
    vk::UniqueCommandPool tmpCmdPool = device->createCommandPoolUnique(tmpCmdPoolCreateInfo);
    
    vk::CommandBufferAllocateInfo tmpCmdBufAllocInfo;
    tmpCmdBufAllocInfo.commandPool = tmpCmdPool.get();
    tmpCmdBufAllocInfo.commandBufferCount = 1;
    tmpCmdBufAllocInfo.level = vk::CommandBufferLevel::ePrimary;
    std::vector<vk::UniqueCommandBuffer> tmpCmdBufs = device->allocateCommandBuffersUnique(tmpCmdBufAllocInfo);

    // バッファ間でデータをコピーするには copyBuffer を使う
    // そしてコピーするバッファやコピーする領域を指定するために vk::BufferCopy 構造体を使う
    // srcOffsetは転送元バッファの先頭から何バイト目を読み込むというデータ位置、dstOffsetは転送先バッファの先頭から何バイト目に書き込むというデータ位置、sizeはデータサイズを表す
    // memcpyの引数と似たような感じだと理解すると分かりやすい
    // あとはキューに投げるだけ
    vk::BufferCopy bufCopy;
    bufCopy.srcOffset = 0;
    bufCopy.dstOffset = 0;
    bufCopy.size = sizeof(Vertex) * vertices.size();

    vk::CommandBufferBeginInfo cmdBeginInfo;
    cmdBeginInfo.flags = vk::CommandBufferUsageFlagBits::eOneTimeSubmit;

    tmpCmdBufs[0]->begin(cmdBeginInfo);
    tmpCmdBufs[0]->copyBuffer(stagingBuf.get(), vertexBuf.get(), {bufCopy});
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
    (*result)[0].format = vk::Format::eR32G32B32Sfloat;
    (*result)[0].offset = offsetof(Vertex, pos);
    
    (*result)[1].binding = 0;
    (*result)[1].location = 1;
    (*result)[1].format = vk::Format::eR32G32B32Sfloat;
    (*result)[1].offset = offsetof(Vertex, color);

    return result;
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
        LOGERR("Suitable memory type not found. ");
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
        LOGERR("Suitable memory type not found. ");
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

std::shared_ptr<vk::UniqueBuffer> getUniformBuffer(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueBuffer> result = std::make_shared<vk::UniqueBuffer>();
    
    vk::BufferCreateInfo uniformBufferCreateInfo;
    uniformBufferCreateInfo.size = sizeof(SceneData);
    // usageはvk::BufferUsageFlagBits::eUniformBufferを指定
    uniformBufferCreateInfo.usage = vk::BufferUsageFlagBits::eUniformBuffer;
    uniformBufferCreateInfo.sharingMode = vk::SharingMode::eExclusive;

    *result = device.get().createBufferUnique(uniformBufferCreateInfo);
    return result;
}

std::shared_ptr<vk::UniqueDeviceMemory> getUniformBufferMemory(vk::UniqueDevice& device, vk::PhysicalDevice& physicalDevice, vk::UniqueBuffer& uniformBuf)
{    
    std::shared_ptr<vk::UniqueDeviceMemory> result = std::make_shared<vk::UniqueDeviceMemory>();
    vk::PhysicalDeviceMemoryProperties memProps = physicalDevice.getMemoryProperties();

    vk::MemoryRequirements uniformBufMemReq = device->getBufferMemoryRequirements(uniformBuf.get());

    vk::MemoryAllocateInfo uniformBufMemAllocInfo;
    uniformBufMemAllocInfo.allocationSize = uniformBufMemReq.size;

    bool suitableMemoryTypeFound = false;
    for (uint32_t i = 0; i < memProps.memoryTypeCount; i++) 
    {
        if (uniformBufMemReq.memoryTypeBits & (1 << i) && (memProps.memoryTypes[i].propertyFlags & vk::MemoryPropertyFlagBits::eHostVisible)) 
        {
            uniformBufMemAllocInfo.memoryTypeIndex = i;
            suitableMemoryTypeFound = true;
            break;
        }
    }
    if (!suitableMemoryTypeFound) 
    {
        LOGERR("Suitable memory type not found. ");
        exit(EXIT_FAILURE);
    }

    *result = device.get().allocateMemoryUnique(uniformBufMemAllocInfo);
    device->bindBufferMemory(uniformBuf.get(), result->get(), 0);
    return result;
}

void* mapUniformBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem)
{
    return device.get().mapMemory(uniformBufMem.get(), 0, sizeof(SceneData));
}

void writeUniformBuffer(void* pUniformBufMem, vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem, int deltaTime)
{
    static float time = 0;

    sceneData.rectCenter = Vec2{ 0.3f * std::cos(time), 0.3f * std::sin(time) };
    time += deltaTime / 1000.0f;

    std::memcpy(pUniformBufMem, &sceneData, sizeof(SceneData));

    vk::MappedMemoryRange flushMemoryRange;
    flushMemoryRange.memory = uniformBufMem.get();
    flushMemoryRange.offset = 0;
    flushMemoryRange.size = sizeof(SceneData);

    device.get().flushMappedMemoryRanges({ flushMemoryRange });
    // device.get().unmapMemory(uniformBufMem.get());
}

void unmapUniformBuffer(vk::UniqueDevice& device, vk::UniqueDeviceMemory& uniformBufMem)
{
    device.get().unmapMemory(uniformBufMem.get());
}

std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> getDiscriptorSetLayouts(vk::UniqueDevice& device)
{
    std::shared_ptr<std::vector<vk::UniqueDescriptorSetLayout>> result = std::make_shared<std::vector<vk::UniqueDescriptorSetLayout>>();

    // vk::DescriptorSetLayoutBindingがデスクリプタ1つの情報を表す
    // これの配列からデスクリプタセットレイアウトを作成
    //
    // binding はシェーダと結びつけるための番号を表す = バインディング番号
    //    頂点シェーダにlayout(set = 0, binding = 0)などと指定したが、このときのbindingの数字と揃えてあることに注意
    // descriptorType はデスクリプタの種別を示す
    //    今回シェーダに渡すものはバッファなのでvk::DescriptorType::eUniformBufferを指定
    // descriptorCount はデスクリプタの個数を表す
    //    デスクリプタは配列として複数のデータを持てるが、ここにその要素数を指定する 今回は1個だけなので1を指定
    // stageFlags はデータを渡す対象となるシェーダを示す 
    //    今回は頂点シェーダだけに渡すのでvk::ShaderStageFlagBits::eVertexを指定 フラグメントシェーダに渡したい場合はvk::ShaderStageFlagBits::eFragmentを指定します。ビットマスクなので、ORで重ねれば両方に渡すことも可能です。

    vk::DescriptorSetLayoutBinding descSetLayoutBinding[1];
    descSetLayoutBinding[0].binding = 0;
    descSetLayoutBinding[0].descriptorType = vk::DescriptorType::eUniformBuffer;
    descSetLayoutBinding[0].descriptorCount = 1;
    descSetLayoutBinding[0].stageFlags = vk::ShaderStageFlagBits::eVertex;
    
    vk::DescriptorSetLayoutCreateInfo descSetLayoutCreateInfo{};
    descSetLayoutCreateInfo.bindingCount = 1;
    descSetLayoutCreateInfo.pBindings = descSetLayoutBinding;
    
    // デスクリプタセットレイアウトを作成したあとはそれをパイプラインレイアウトに設定する必要がある
    // パイプラインは描画の手順を表すオブジェクト
    // 頂点入力デスクリプションなどと同様、シェーダへのデータの読み込ませ方はここで設定する
    (*result).push_back(device->createDescriptorSetLayoutUnique(descSetLayoutCreateInfo));
    return result;
}

std::shared_ptr<vk::UniqueDescriptorPool> getDescriptorPool(vk::UniqueDevice& device)
{
    std::shared_ptr<vk::UniqueDescriptorPool> result = std::make_shared<vk::UniqueDescriptorPool>();

    // 次にデスクリプタプールを作成
    // 重要なこととしてデスクリプタには種類がある
    // そのためデスクリプタプールも、「この種類のデスクリプタをこの数」と指定して作成する必要がある
    // 必要な種類と数をきちんと指定する
    vk::DescriptorPoolSize descPoolSize[1];
    descPoolSize[0].type = vk::DescriptorType::eUniformBuffer;
    descPoolSize[0].descriptorCount = 1;

    // vk::DescriptorPoolSizeの配列を poolSizeCountとpPoolSizes に指定
    // vk::DescriptorPoolSizeはtypeがデスクリプタの種類でdescriptorCountがデスクリプタの数
    // vk::DescriptorPoolCreateInfoに maxSets というメンバがあるが、これはデスクリプタプールから作成するデスクリプタセットの数の上限を指定
    vk::DescriptorPoolCreateInfo descPoolCreateInfo;
    descPoolCreateInfo.poolSizeCount = std::size(descPoolSize);
    descPoolCreateInfo.pPoolSizes = descPoolSize;
    descPoolCreateInfo.maxSets = 1;

    *result = device->createDescriptorPoolUnique(descPoolCreateInfo);
    return result;
}

std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> getDescprotorSets(vk::UniqueDevice& device, vk::UniqueDescriptorPool& descPool, std::vector<vk::DescriptorSetLayout>& descSetLayouts)
{
    std::shared_ptr<std::vector<vk::UniqueDescriptorSet>> result = std::make_shared<std::vector<vk::UniqueDescriptorSet>>();

    // 最後にデスクリプタセットを作成
    // デスクリプタセットはデバイスの allocateDescriptorSetsメソッドを用いて作成する
    // createではなくallocateなあたりで「デスクリプタプールから割り当てる」という気持ちが読み取れる

    // 構造を示すデスクリプタセットレイアウト、実体を記録するデスクリプタプールを指定して作成 
    // またSetsと複数形になっているところから分かる通り、一度に複数のデスクリプタセットを作成できる
    // デスクリプタセットレイアウトが配列によって複数渡せるようになっているのはそのため

    // デスクリプタ関係でややこしいのは、デスクリプタセットを表すvk::DescriptorSet、デスクリプタプールを表すvk::DescriptorPoolなどは存在するのに、
    // 1つのデスクリプタを表すvk::Descriptorなどというオブジェクトは存在しないということ
    // デスクリプタは常に「あるデスクリプタセットの何番目のデスクリプタ」という形でしか触ることができない
    
    vk::DescriptorSetAllocateInfo descSetAllocInfo;
    
    descSetAllocInfo.descriptorPool = descPool.get();
    descSetAllocInfo.descriptorSetCount = descSetLayouts.size();
    descSetAllocInfo.pSetLayouts = descSetLayouts.data();
    
    *result = device->allocateDescriptorSetsUnique(descSetAllocInfo);
    return result;
}

void writeDescriptorSets(vk::UniqueDevice& device, std::vector<vk::UniqueDescriptorSet>& descSets, vk::UniqueBuffer& uniformBuf)
{
    // 今はまだデスクリプタセットを作っただけでその中身は何もないので、updateDescriptorSetsで中身を設定する必要がある
    // デスクリプタへの書き込み情報はvk::WriteDescriptorSet構造体で表される

    // 一度に複数のvk::WriteDescriptorSetをupdateDescriptorSetsに渡せば複数のデスクリプタの更新を行うことも可能
    // dstSet は書き込みの対象とするデスクリプタセット

    // dstBinding は書き込みの対象とするデスクリプタのバインディング番号
    // dstArrayElement は書き込みの対象とするデスクリプタの配列要素の番号
    // デスクリプタは配列であって複数のデータが持てる、という話をしたが、その配列上の何番からの要素に書き込むかをここに指定する
    
    // descriptorType は書き込みの対象となるデスクリプタの種別
    // ここではvk::DescriptorType::eUniformBufferを指定

    // バッファタイプのデスクリプタに書き込む場合、vk::DescriptorBufferInfo の配列を pBufferInfoとdescriptorCount に指定
    // vk::DescriptorBufferInfoのメンバについて解説すると、buffer が用いるバッファ、offset がバッファ上の先頭から何バイト目からをデータとして用いるか、range がデータの大きさ(バイト数)になる

    // 混乱を避けるために書いておくと、デスクリプタが持っている情報は「0.3, -0.2」といった数値ではなく、あくまで「このバッファのこの位置のデータをシェーダに渡す」という情報
    // デスクリプタ=ポインタみたいなものと考えた方が良い
    // 従って、データの変更のたびにupdateDescriptorSetsを呼ばなくてもバッファのメモリ内容を書き換えればシェーダに渡すデータも変えられる
    // 途中で別のバッファや別の領域を使うといったことをしない限り、updateDescriptorSetsによる設定は最初の1回だけで十分です。

    vk::WriteDescriptorSet writeDescSet;
    writeDescSet.dstSet = descSets[0].get();
    writeDescSet.dstBinding = 0;
    writeDescSet.dstArrayElement = 0;
    writeDescSet.descriptorType = vk::DescriptorType::eUniformBuffer;
    
    vk::DescriptorBufferInfo descBufInfo[1];
    descBufInfo[0].buffer = uniformBuf.get();
    descBufInfo[0].offset = 0;
    descBufInfo[0].range = sizeof(SceneData);
    
    writeDescSet.descriptorCount = 1;
    writeDescSet.pBufferInfo = descBufInfo;
    
    device->updateDescriptorSets({ writeDescSet }, {});
}

Mat4x4 scaleMatrix(float scale) 
{
    return Mat4x4{{
        {scale, 0, 0, 0},
        {0, scale, 0, 0},
        {0, 0, scale, 0},
        {0, 0, 0, 1},
    }};
}

Mat4x4 rotationMatrix(Vec3 n, float theta) 
{
    float c = cos(theta);
    float s = sin(theta);
    float nc = 1 - c;

    return Mat4x4{{
        {n.x * n.x * nc + c,       n.x * n.y * nc + n.z * s, n.x * n.z * nc - n.y * s, 0},
        {n.y * n.x * nc - n.z * s, n.y * n.y * nc + c,       n.y * n.z * nc + n.x * s, 0},
        {n.z * n.x * nc + n.y * s, n.z * n.y * nc - n.x * s, n.z * n.z * nc + c,       0},
        {0, 0, 0, 1},
    }};
}

Mat4x4 translationMatrix(Vec3 v) 
{
    return Mat4x4{{
        {1, 0, 0, 0},
        {0, 1, 0, 0},
        {0, 0, 1, 0},
        {v.x, v.y, v.z, 1},
    }};
}

Mat4x4 viewMatrix(Vec3 cameraPos, Vec3 dir, Vec3 up) 
{
    const auto cameraShift = 
        Mat4x4{{
            {1, 0, 0, 0},
            {0, 1, 0, 0},
            {0, 0, 1, 0},
            {-cameraPos.x, -cameraPos.y, -cameraPos.z, 1},
        }};
    const auto cameraRotation = 
        Mat4x4{{
            {up.z * dir.y - up.y * dir.z, -up.x, dir.x, 0},
            {up.x * dir.z - up.z * dir.x, -up.y, dir.y, 0},
            {up.y * dir.x - up.x * dir.y, -up.z, dir.z, 0},
            {0, 0, 0, 1},
        }};

    return cameraRotation * cameraShift;
}

Mat4x4 projectionMatrix(float angle_y, float ratio, float near, float far) 
{
    float ky = tan(angle_y);
    float kx = ky * ratio;

    return Mat4x4{{
        {kx, 0, 0, 0},
        {0, ky, 0, 0},
        {0, 0, far/(far-near), 1},
        {0, 0, -near*far/(far-near), 0}
    }};
}

std::shared_ptr<std::vector<vk::PushConstantRange>> getPushConstantRanges()
{
    // 小さい数値データはプッシュ定数、大きなバッファやテクスチャ画像などのデータはデスクリプタを使う
    // プッシュ定数は128バイトまでしか容量を保証されていない(最低保証)
    std::shared_ptr<std::vector<vk::PushConstantRange>> result = std::make_shared<std::vector<vk::PushConstantRange>>();

    vk::PushConstantRange pushConstantRange;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(CameraData);
    pushConstantRange.stageFlags = vk::ShaderStageFlagBits::eVertex;

    (*result).push_back(pushConstantRange);
    return result;
}

void writePushConstant(uint32_t screenWidth, uint32_t screenHeight, int deltaTime)
{
    static float time = 0;
    time += deltaTime / 1000.0f;

    float rotation = (time / 10) * 2 * 3.14f;

    Mat4x4 model = translationMatrix({0.0f, 0.0f, 0.0f}) * rotationMatrix({0.0f, 0.0f, 1.0f}, rotation) * scaleMatrix(1.0f);
    Mat4x4 view = viewMatrix({0.0f, 2.0f, 2.0f}, {0.0f, -0.707f, -0.707f}, {0.0f, -0.707f, +0.707f});
    Mat4x4 proj = projectionMatrix(3.14f / 3, float(screenHeight) / float(screenWidth), 0.1f, 100.0f);
    
    cameraData.mvpMatrix = proj * view * model;
}